#include "pch.hpp"

#include "login_handler.hpp"

#include "log_on_status.hpp"
#include "login_challenge.hpp"
#include "login_challenge_factory.hpp"
#include "login_challenge_task.hpp"
#include "login_request_protocol.hpp"
#include "login_request_transport.hpp"
#include "server_connection.hpp"

#include "cstdmf/bgtask_manager.hpp"

#include "network/encryption_filter.hpp"
#include "network/network_interface.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 *
 *	@param pServerConnection 	The server connection.
 *	@param loginNotSent			Initial login status.
 */
LoginHandler::LoginHandler(
		ServerConnection * pServerConnection, 
		LogOnStatus loginNotSent ) :
	Mercury::INoSuchPort(),
	TimerHandler(),
	SafeReferenceCount(),
	pServerConnection_( pServerConnection ),
	transport_( CONNECTION_TRANSPORT_UDP ),
	pProtocol_( NULL ),
	loginAppAddr_( Mercury::Address::NONE ),
	baseAppAddr_( Mercury::Address::NONE ),
	retryTimer_(),
	pChallengeTask_( NULL ),
	pParams_( NULL ),
	replyRecord_(),
	status_( loginNotSent ),
	serverMsg_(),
	isDone_( loginNotSent != LogOnStatus::NOT_SET ),
	numAttemptsMade_( 0 ),
	childRequests_(),
	hasReceivedNoSuchPort_( false ),
	pCompletionCallback_( NULL )
{
}


/**
 *	Destructor.
 */
LoginHandler::~LoginHandler()
{
	if (!isDone_)
	{
		WARNING_MSG( "LoginHandler::~LoginHandler: "
				"Destroying unfinished login (%" PRIzu " child requests)\n",
			childRequests_.size() );
		this->finish();
	}
}


/**
 *	This method starts the login process asynchronously.
 *
 *	@param loginAppAddr 	The address of the LoginApp to log in to.
 *	@param transport 		The transport to use for logging in.
 *	@param pParams 			The login credentials.
 */
void LoginHandler::start( const Mercury::Address & loginAppAddr,
		ConnectionTransport transport,
		LogOnParamsPtr pParams )
{
	BW_GUARD;

	if (isDone_)
	{
		ERROR_MSG( "LoginHandler::start: "
			"This handler has already been completed\n" );

		return;
	}

	IF_NOT_MF_ASSERT_DEV( loginAppAddr != Mercury::Address::NONE )
	{
		// Save the trouble of connecting nowhere.
		this->fail( LogOnStatus::CONNECTION_FAILED,
			"Invalid LoginApp address" );
		this->finish();
		return;
	}
	
	transport_ 					= transport;
	pProtocol_ 					= LoginRequestProtocol::getForLoginApp();
	loginAppAddr_ 				= loginAppAddr;
	baseAppAddr_ 				= Mercury::Address::NONE;
	pParams_ 					= pParams;
	numAttemptsMade_ 			= 0;

	IF_NOT_MF_ASSERT_DEV( pProtocol_->numRequests() > 0 )
	{
		ERROR_MSG( "LoginHandler::start: Protocol to LoginApp specified no "
				"attempts\n" );

		pProtocol_->onAttemptsExhausted( *this, hasReceivedNoSuchPort_ );
		return;
	}

	DEBUG_MSG( "LoginHandler::start: LoginApp@%s over %s "
			"(# requests: %u, retry interval: %.01fs, "
			"timeout interval: %.01fs)\n",
		loginAppAddr.c_str(),
		ServerConnection::transportToCString( transport ),
		pProtocol_->numRequests(),
		pProtocol_->retryInterval(),
		pProtocol_->timeoutInterval() );

	this->sendNextRequest();
}


/**
 *	This method starts a BaseApp login asynchronously.
 *
 *	@param baseAppAddr 		The address of the BaseApp expecting our login.
 *	@param transport 		The transport to use for logging in.
 *	@param sessionKey 		The session key.
 */
void LoginHandler::startWithBaseAddr( const Mercury::Address & baseAppAddr,
		ConnectionTransport transport, SessionKey sessionKey )
{
	BW_GUARD;

	if (isDone_)
	{
		ERROR_MSG( "LoginHandler::start: "
			"This handler has already been completed\n" );

		return;
	}

	transport_ 					= transport;
	pProtocol_					= LoginRequestProtocol::getForBaseApp();
	loginAppAddr_				= Mercury::Address::NONE;
	baseAppAddr_ 				= baseAppAddr;
	replyRecord_.sessionKey 	= sessionKey;
	replyRecord_.serverAddr 	= baseAppAddr;
	numAttemptsMade_ 			= 0;

	IF_NOT_MF_ASSERT_DEV( pProtocol_->numRequests() > 0 )
	{
		ERROR_MSG( "LoginHandler::startWithBaseAddr: "
			"Protocol to BaseApp specified no attempts\n" );

		pProtocol_->onAttemptsExhausted( *this, hasReceivedNoSuchPort_ );
		return;
	}

	DEBUG_MSG( "LoginHandler::startWithBaseAddr: BaseApp@%s over %s "
			"(# requests: %u, retry interval:%.01fs, "
			"timeout interval: %.01fs)\n",
		baseAppAddr.c_str(),
		ServerConnection::transportToCString( transport ),
		pProtocol_->numRequests(),
		pProtocol_->retryInterval(),
		pProtocol_->timeoutInterval() );

	this->sendNextRequest();
}


/**
 *	This method cancels any outstanding login requests.
 */
void LoginHandler::finish()
{
	BW_GUARD;

	if (isDone_)
	{
		WARNING_MSG( "LoginHandler::finish: Called twice\n" );
		return;
	}

	isDone_ = true;

	retryTimer_.cancel();

	this->finishChildRequests();

	pProtocol_ = NULL;

	pServerConnection_->dispatcher().breakProcessing();

	// Recreate the ServerConnection's network interface if necessary.
	if (!pServerConnection_->hasInterface())
	{
		Mercury::NetworkInterface * pInterface =
			new Mercury::NetworkInterface( 
				&pServerConnection_->dispatcher(),
				Mercury::NETWORK_INTERFACE_EXTERNAL );

		pServerConnection_->pInterface( pInterface );
	}

	if (pCompletionCallback_)
	{
		pCompletionCallback_->onLoginComplete( this );
	}
}


/**
 *  This method sends the login request to the server asynchronously.
 *
 *	@see LoginRequest
 *	@see LoginHandler::onLoginAppLoginChallengeIssued
 *	@see LoginHandler::onLoginAppLoginSuccess
 *	@see LoginHandler::onBaseAppLoginSuccess
 *	@see LoginHandler::onRequestFailed
 */
void LoginHandler::sendNextRequest()
{
	BW_GUARD;

	uint attemptNum = ++numAttemptsMade_;

	LoginRequestTransportPtr pTransport =
		LoginRequestTransport::createForTransport( transport_ );

	LoginRequestPtr pLoginRequest = new LoginRequest( *this,
		*pTransport, *pProtocol_, attemptNum );
	pTransport->init(*pLoginRequest);

	childRequests_.insert( pLoginRequest );

	if (numAttemptsMade_ < pProtocol_->numRequests())
	{
		retryTimer_ = pServerConnection_->dispatcher().addOnceOffTimer(
			int64( pProtocol_->retryInterval() * 1000000 ), this,
			/* arg */ NULL, "LoginHandler::sendNextRequest" );
	}

	TRACE_MSG( "LoginHandler::sendNextRequest: "
			"From %s, attempt #%u / %u to %s %s\n",
		pLoginRequest->sourceAddress().c_str(),
		attemptNum,
		pProtocol_->numRequests(),
		pProtocol_->appName(),
		pProtocol_->address( *this ).c_str() );
}


/**
 *  This method removes a LoginRequest from this LoginHandler.
 *
 *	@param request 	The finished request to be removed.
 */
void LoginHandler::removeChildRequest( LoginRequest & request )
{
	BW_GUARD;

	if (0 == childRequests_.erase( &request ))
	{
		WARNING_MSG( "LoginHandler::removeChildRequest: "
				"Request not present: (Attempt #%u)\n",
			request.attemptNum() );
	}
}


/**
 *	This method is called by a LoginRequest after it has failed.
 *
 *	@param request 			The request that failed.
 *	@param reason 			The reason for the failure.
 *	@param errorMessage 	A human-readable description of the error.
 */
void LoginHandler::onRequestFailed( LoginRequest & request,
		Mercury::Reason reason, const BW::string & errorMessage )
{
	BW_GUARD;

	WARNING_MSG( "LoginHandler::onRequestFailed: "
			"Request to %s %s, attempt #%u failed: \"%s\" (%s)\n",
		pProtocol_->appName(),
		pProtocol_->address( *this ).c_str(),
		request.attemptNum(),
		errorMessage.c_str(),
		Mercury::reasonToString( reason ) );

	MF_ASSERT( request.isFinished() );
	MF_ASSERT( childRequests_.count( LoginRequestPtr( &request ) ) == 0 );

	if ((numAttemptsMade_ >= pProtocol_->numRequests()) &&
			childRequests_.empty())
	{
		WARNING_MSG( "LoginHandler::onRequestFailed: "
				"Exhausted all %u attempts to %s %s\n",
			pProtocol_->numRequests(), pProtocol_->appName(),
			pProtocol_->address( *this ).c_str() );

		// Ask the protocol what we should do.
		pProtocol_->onAttemptsExhausted( *this, hasReceivedNoSuchPort_ );
	}
}


/**
 *	This method is called when the login process has terminated and cannot
 *	continue.
 *
 *	@param status 			The log on status.
 *	@param errorMessage 	A descriptive message about the failure.
 */
void LoginHandler::fail( LogOnStatus status, const BW::string & errorMessage )
{
	BW_GUARD;

	WARNING_MSG( "LoginHandler::fail: status = %d, errorMessage = \"%s\"\n",
		status.value(), errorMessage.c_str() );

	status_ = status;
	serverMsg_ = errorMessage;

	this->finish();
}


/**
 *	This method handles when the server sends us a challenge.
 */
void LoginHandler::onLoginAppLoginChallengeIssued(
		LoginChallengePtr pChallenge )
{
	BW_GUARD;

	if (pChallengeTask_)
	{
		ERROR_MSG( "LoginHandler::onLoginAppLoginChallengeIssued: "
			"Already have challenge\n" );

		this->fail( LogOnStatus::CONNECTION_FAILED,
			"Got duplicate challenges from server" );
		return;
	}

	retryTimer_.cancel();
	this->finishChildRequests();

	pChallengeTask_ = new LoginChallengeTask( *this, pChallenge );

	TaskManager * pTaskManager = pServerConnection_->pTaskManager();

	DEBUG_MSG( "LoginHandler::onLoginChallengeCompleted: "
			"Starting login challenge %s\n",
		pTaskManager ? "in background" : "in foreground" );

	if (pTaskManager)
	{
		pTaskManager->addBackgroundTask( pChallengeTask_.get() );
	}
	else
	{
		// We don't have a task manager, so do it here and now.
		pChallengeTask_->perform();
		this->onLoginChallengeCompleted();
	}
}


/**
 *	This method is called when a login challenge has been completed.
 */
void LoginHandler::onLoginChallengeCompleted()
{
	BW_GUARD;

	DEBUG_MSG( "LoginHandler::onLoginChallengeCompleted: "
			"Challenge took %.03fs\n",
		pChallengeTask_->calculationDuration() );

	numAttemptsMade_ = 0;
	this->sendNextRequest();
}


/**
 *	This method returns a challenge for the given challenge type string.
 *
 *	@param challengeType 	The challenge type.
 *
 *	@return 				The newly created challenge, or NULL if an error
 *							occurred.
 */
LoginChallengePtr LoginHandler::createChallenge(
		const BW::string & challengeType )
{
	return pServerConnection_->challengeFactories().createChallenge(
		challengeType );
}


/**
 *	This method returns the calculation duration, or -1 if there was no
 *	challenge receieved or the challenge has not finished.
 */
float LoginHandler::challengeCalculationDuration() const
{
	if (!pChallengeTask_ || !pChallengeTask_->isFinished())
	{
		return -1.f;
	}

	return pChallengeTask_->calculationDuration();
}


/**
 *	This method returns any available challenge response data, or NULL if no
 *	challenge has been received yet.
 */
BinaryIStream * LoginHandler::pChallengeData()
{
	if (!pChallengeTask_ || !pChallengeTask_->isFinished())
	{
		return NULL;
	}

	return &(pChallengeTask_->data());
}


/**
 *	This method handles the login reply message from the LoginApp.
 *
 *	@param replyRecord 		The reply from the LoginApp.
 *	@param serverMessage	A descriptive message about the login.
 */
void LoginHandler::onLoginAppLoginSuccess(
		const LoginReplyRecord & replyRecord,
		const BW::string & serverMessage )
{
	BW_GUARD;

	retryTimer_.cancel();

	this->finishChildRequests();


	if (isDone_)
	{
		TRACE_MSG( "LoginHandler::onLoginAppLoginSuccess: isDone_ = true, probably "
					"caused by a SHUTDOWN message. Ignoring." );
		return;
	}

	replyRecord_ 			= replyRecord;
	serverMsg_ 				= serverMessage;
	baseAppAddr_ 			= replyRecord_.serverAddr;

	pProtocol_ 				= LoginRequestProtocol::getForBaseApp();
	numAttemptsMade_ 		= 0;
	hasReceivedNoSuchPort_ 	= false;

	IF_NOT_MF_ASSERT_DEV( pProtocol_->numRequests() > 0 )
	{
		ERROR_MSG( "LoginHandler::onLoginAppLoginSuccess: "
			"Protocol to BaseApp specified no attempts\n" );

		pProtocol_->onAttemptsExhausted( *this, hasReceivedNoSuchPort_ );
		return;
	}

	DEBUG_MSG( "LoginHandler::onLoginAppLoginSuccess: BaseApp@%s over %s "
			"(# requests: %u, retry interval:%.01fs, "
			"timeout interval: %.01fs)\n",
		baseAppAddr_.c_str(),
		ServerConnection::transportToCString( transport_ ),
		pProtocol_->numRequests(),
		pProtocol_->retryInterval(),
		pProtocol_->timeoutInterval() );

	this->sendNextRequest();
}


/**
 *	This method is called when a reply to baseAppLogin is received from the
 *	BaseApp (or an exception occurred - likely a timeout).
 *
 *	The first successful reply wins and we do not care about the rest.
 *
 *	@param pChannel 	The winning channel. This is now owned by the
 *						LoginHandler, relinquished from the LoginRequest.
 *	@param sessionKey 	The session key of the connection.
 */
void LoginHandler::onBaseAppLoginSuccess( Mercury::Channel * pChannel,
		SessionKey sessionKey )
{
	BW_GUARD;

	DEBUG_MSG( "LoginHandler::onBaseAppLoginSuccess: "
			"Got reply from BaseApp channel %s\n",
		pChannel->c_str() );


	if (isDone_)
	{
		TRACE_MSG( "LoginHandler::onBaseAppLoginSuccess: isDone_ = true, probably "
					"caused by a SHUTDOWN message. Ignoring." );
		return;
	}

	// Necessary to set this here as when we are switching BaseApps, 
	// we don't go through the LoginApps to get the log on status.
	status_ = LogOnStatus::LOGGED_ON;

	// This is the session key that authenticate message should send.
	if (sessionKey == 0)
	{
		ERROR_MSG( "LoginHandler::onBaseAppLoginSuccess: "
				"Got zero session key\n" );
		this->fail( LogOnStatus::CONNECTION_FAILED,
				"Got zero session key" );
		return;
	}
	else if (sessionKey == replyRecord_.sessionKey)
	{
		ERROR_MSG( "LoginHandler::onBaseAppLoginSuccess: "
				"Got unchanged session key\n" );
		this->fail( LogOnStatus::CONNECTION_FAILED,
			"Got unchanged session key" );
		return;
	}

	replyRecord_.sessionKey = sessionKey;

	pServerConnection_->sessionKey( sessionKey );
	pServerConnection_->pInterface( &(pChannel->networkInterface()) );
	pServerConnection_->channel( *pChannel );

	this->finish();
}


/**
 *  This method adds a condemned network interface to this LoginHandler. 
 *
 *	This is used by BaseAppLoginRequests to clean up their network interfaces
 *	as they are unable to do it themselves, as they may be triggered while in
 *	the process of handling a network event from that same interface.
 *
 *	@param pInterface 	The network interface to condemn.
 */
void LoginHandler::addCondemnedInterface( 
		Mercury::NetworkInterface * pInterface )
{
	BW_GUARD;

	pServerConnection_->addCondemnedInterface( pInterface );
}


/*
 *	Override from INoSuchPort.
 */
void LoginHandler::onNoSuchPort( const Mercury::Address & addr )
{
	BW_GUARD;

	// Note that this only affects UDP connections, for TCP connections, ICMP
	// no such port notifications do not get sent to the NetworkInterface port
	// (because TCP connections open up their own sockets for connecting to the
	// server).

	if (isDone_ ||
			!pProtocol_.hasObject() ||
			(addr != pProtocol_->address( *this )))
	{
		TRACE_MSG( "LoginHandler::onNoSuchPort: "
				"Got notified for unrelated (possibly old) address: %s\n",
			addr.c_str() );
		return;
	}

	if (!hasReceivedNoSuchPort_)
	{
		WARNING_MSG( "LoginHandler::onNoSuchPort: "
				"%s %s is refusing connections\n",
			pProtocol_->appName(), addr.c_str() );

		hasReceivedNoSuchPort_ = true;
	}
}


/**
 *	This method finishes each outstanding child request.
 */
void LoginHandler::finishChildRequests()
{
	size_t childRequestSize = childRequests_.size();

	while (!childRequests_.empty())
	{
		LoginRequestPtr pChild = *(childRequests_.begin());

		pChild->finish();

		MF_ASSERT( pChild->isFinished() );

		MF_ASSERT( childRequestSize > 0 );

		// Check that they removed themselves.
		MF_ASSERT( childRequests_.count( pChild ) == 0 );
		--childRequestSize;

		IF_NOT_MF_ASSERT_DEV( childRequests_.size() == childRequestSize )
		{
			ERROR_MSG( "LoginHandler::finishChildRequests: "
					"A child request did not remove itself properly "
					"(while connecting to %s)\n",
				pProtocol_->appName() );

			this->fail( LogOnStatus::CONNECTION_FAILED,
				"Child request failed to remove itself" );
			childRequests_.clear();
			return;
		}
	}
}


/*
 *	Override from TimerHandler.
 */
void LoginHandler::handleTimeout( TimerHandle handle, void * arg )
{
	this->sendNextRequest();
}


BW_END_NAMESPACE


// login_handler.cpp
