#include "pch.hpp"

#include "login_request.hpp"

#include "login_handler.hpp"
#include "login_request_protocol.hpp"
#include "login_request_transport.hpp"
#include "server_connection.hpp"

#include "network/nub_exception.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 *
 *	@param parent 		The parent LoginHandler.
 *	@param transport 	The transport to the server.
 *	@param protocol		The protocol object used to speak to the server process
 *						we connect to.
 *	@param attemptNum	The attempt number.
 *
 */
LoginRequest::LoginRequest( LoginHandler & parent,
			LoginRequestTransport & transport,
			LoginRequestProtocol & protocol,
			uint attemptNum ) :
		SafeReferenceCount(),
		Mercury::ShutdownSafeReplyMessageHandler(),
		Mercury::ChannelListener(),
		pParent_( &parent ),
		pTransport_( &transport ),
		pProtocol_( &protocol ),
		isFinished_( false ),
		attemptNum_( attemptNum ),
		timeoutInterval_( pProtocol_->timeoutInterval() ),
		startTime_( timestamp() ),
		pInterface_( NULL ),
		pChannel_( NULL )
{
	pInterface_ = pProtocol_->setUpInterface( *this );
}


/**
 *	Destructor.
 */
LoginRequest::~LoginRequest()
{
	IF_NOT_MF_ASSERT_DEV( isFinished_ )
	{
		ERROR_MSG( "LoginRequest::~LoginRequest: "
			"Not finished before destructor called, finishing now\n" );
		this->finish();
	}
}


/**
 *	This method cleans up this request. Note that it should be called on
 *	requests both successful and failed.
 */
void LoginRequest::finish()
{
	if (isFinished_)
	{
		if (pParent_ == NULL)
		{
			// pParent_ is cleared as the last step. If we re-enter finish()
			// as part of cancelRequestsFor() below, this is expected 
			// and we should not warn.
			WARNING_MSG( "LoginRequest::finish: Already finished: "
					"Attempt #%u to %s %s\n",
				attemptNum_,
				this->appName(),
				this->address().c_str() );
		}
		return;
	}

	isFinished_ = true;

	LoginRequestPtr pThisHolder( this );

	pTransport_->finish();
	pTransport_ = NULL;

	// This needs to be before cancelRequestsFor() as we may re-enter finish().
	pParent_->removeChildRequest( *this );

	// If we still have our channel and interface when we are finishing, they
	// did not win, so these need to be cleaned up.
	if (pChannel_.hasObject())
	{
		pChannel_->pChannelListener( NULL );
		pChannel_->shutDown();

		// TODO: Calling destroy() here to fix a memory leak for TCP channels
		// from failed requests. Ideally NetworkInterface would have a mechanism 
		// for tracking condemned TCP channels waiting for FIN from the server.
		if (!pChannel_->isDestroyed())
		{
			pChannel_->destroy();
		}

		pChannel_ = NULL;
	}

	if (pInterface_ != NULL)
	{
		pInterface_->cancelRequestsFor( this, Mercury::REASON_SHUTTING_DOWN );
		pProtocol_->cleanUpInterface( *this, pInterface_ );
		pInterface_ = NULL;
	}

	pProtocol_ = NULL;
	pParent_ = NULL;
}


/**
 *	This method is called when the login transport reports a channel has been
 *	opened.
 */
void LoginRequest::onTransportConnect()
{
	BW_GUARD;

	const float timeLeft = this->timeLeft();

	DEBUG_MSG( "LoginRequest::onTransportConnect: Attempt #%u to %s %s; "
			" took %.01fs, time left = %.01fs\n",
		attemptNum_,
		this->appName(),
		this->address().c_str(),
		this->timeSinceStart(),
		timeLeft );

	if (timeLeft <= 0.f)
	{
		// Consider this request failed. We must have just missed it, otherwise
		// handleException would have been called.
		pProtocol_->onAttemptFailed( *this, Mercury::REASON_TIMER_EXPIRED,
			"Request timed out" );

		return;
	}

	pProtocol_->sendRequest( *pTransport_, *this, timeLeft );
}


/**
 *	This method releases any reference to the channel and interface that has
 *	been used in this request.
 */
void LoginRequest::relinquishChannelAndInterface()
{
	// Calling method so that we can deregister from the channel as the channel
	// listener.
	this->pChannel( NULL );
	pInterface_ = NULL;
}


/**
 *	This method returns the application name that we are connecting to.
 */
const char * LoginRequest::appName() const
{
	return pProtocol_->appName();
}


/**
 *	This method returns the destination server address for this request.
 */
const Mercury::Address & LoginRequest::address() const
{
	return pProtocol_->address( *pParent_ );
}


/**
 *	This method returns the time in seconds since this request was started.
 */
float LoginRequest::timeSinceStart() const
{
	uint64 now = timestamp();

	return static_cast< float >( (now - startTime_) / stampsPerSecondD() );
}


/**
 *	This method returns the time (in seconds) left before this request is
 *	considered timed out.
 */
float LoginRequest::timeLeft() const
{
	uint64 now = timestamp();

	return (timeoutInterval_ - 
		static_cast< float >( (now - startTime_) / stampsPerSecondD() ));
}


/**
 *	This method sets/clears the channel for this request.
 *
 *	@param pChannel 	The channel, or NULL to clear the channel.
 */
void LoginRequest::pChannel( Mercury::Channel * pChannel )
{ 
	BW_GUARD;

	if (pChannel_ != NULL)
	{
		pChannel_->pChannelListener( NULL );
	}

	pChannel_ = pChannel;

	if (pChannel_ != NULL)
	{
		pChannel_->pChannelListener( this );
	}
}


/**
 *	This method handles a failure reported from the transport.
 *
 *	@param reason 			The reason for the failure.
 *	@param errorMessage 	A human-readable description of the failure.
 */
void LoginRequest::onTransportFailed( Mercury::Reason reason,
		const BW::string & errorMessage )
{
	BW_GUARD;

	TRACE_MSG( "LoginRequest::onTransportFailed: Attempt #%u: "
			"%s \"%s\" (after %.01fs)\n",
		attemptNum_, Mercury::reasonToString( reason ), errorMessage.c_str(),
		this->timeSinceStart() );

	pProtocol_->onAttemptFailed( *this, reason, errorMessage );
}

/**
 *	This method returns the source address of this request.
 *
 *	@return the source addres.
 */
const Mercury::Address & LoginRequest::sourceAddress() const
{
	return pInterface_ ? pInterface_->address() : Mercury::Address::NONE;
}


/*
 *	Override from Mercury::ShutdownSafeReplyMessageHandler.
 */
void LoginRequest::handleMessage( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		void * arg )
{
	BW_GUARD;

	TRACE_MSG( "LoginRequest::handleMessage: Attempt #%u: "
			"Received reply from %s (after %.01fs)\n", 
		attemptNum_, source.c_str(), this->timeSinceStart() );

	pProtocol_->onReply( *this, data );
}


/*
 *	Override from Mercury::ShutdownSafeReplyMessageHandler.
 */
void LoginRequest::handleException( const Mercury::NubException & exception,
		void * arg )
{
	BW_GUARD;

	const bool wasShuttingDown = 
		(exception.reason() == Mercury::REASON_SHUTTING_DOWN);

	TRACE_MSG( "LoginRequest::handleException: Attempt #%u: %s: %s "
			"(time since start = %.01fs)\n",
		attemptNum_, 
		((pChannel_ != NULL) ? pChannel_->c_str() : "(no channel)"),
		Mercury::reasonToString( exception.reason() ),
		this->timeSinceStart() );

	BW::string errorMessage = BW::string(
			!wasShuttingDown ?
				"Failed to send request to " :
				"Cancelled request to ") +
		this->appName() + " " +
		this->address().c_str();

	LoginHandlerPtr pParent( pParent_ );
	LoginRequestPtr pThisHolder( this );

	this->finish();

	pParent->onRequestFailed( *this, exception.reason(), errorMessage );
}


/*
 *	Override from Mercury::ChannelListener.
 */
void LoginRequest::onChannelGone( Mercury::Channel & channel )
{
	BW_GUARD;

	MF_ASSERT( pChannel_ == &channel );

	TRACE_MSG( "LoginRequest::onChannelGone: Attempt #%u: %s "
			"(time since start = %.01fs)\n",
		attemptNum_, channel.c_str(),
		this->timeSinceStart() );

	LoginHandlerPtr pParent( pParent_ );
	LoginRequestPtr pThisHolder( this );

	this->finish();

	pParent->onRequestFailed( *this, Mercury::REASON_CHANNEL_LOST,
		"Channel was lost" );
}


BW_END_NAMESPACE

// login_request.cpp
