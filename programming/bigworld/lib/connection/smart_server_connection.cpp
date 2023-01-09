#include "pch.hpp"

#include "login_handler.hpp"
#include "smart_server_connection.hpp"
#include "stream_encoder.hpp"

#include <sys/stat.h>

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
SmartServerConnection::SmartServerConnection(
		LoginChallengeFactories & challengeFactories,
		CondemnedInterfaces & condemnedInterfaces,
		const EntityDefConstants & entityDefConstants ) :
	ServerConnection( challengeFactories, condemnedInterfaces,
		entityDefConstants ),
	pConnectionHandler_( NULL ),
	pInProgressLogin_( NULL ),
	loggedOn_( false ),
	clientTime_( 0.f ),
	streamEncoder_( /* keyIsPrivate: */ false )
{
	this->pTime( &clientTime_ );
}


/**
 *	Destructor.
 */
SmartServerConnection::~SmartServerConnection()
{
	if (pInProgressLogin_)
	{
		pInProgressLogin_->finish();
		pInProgressLogin_ = NULL;
	}
}


/**
 *	This method associates a connection handler with this object. It receives
 *	calls on specific significant events.
 *
 *	@param pHandler 	The handler to register.
 */
void SmartServerConnection::setConnectionHandler(
		ServerConnectionHandler * pHandler )
{
	pConnectionHandler_ = pHandler;
}


/**
 *	This method sets the LoginApp public key to use for connection session
 *	handshake from a key string.
 *
 *	@param publicKeyString 	The string contents of the public key.
 */
bool SmartServerConnection::setLoginAppPublicKeyString(
		const BW::string & publicKeyString )
{
	if (!streamEncoder_.initFromKeyString( publicKeyString ))
	{
		return false;
	}
	this->pLogOnParamsEncoder( &streamEncoder_ );

	return true;
}


/**
 *	This method sets the LoginApp public key to use for connection session
 *	handshake from a file path.
 *
 *	@param publicKeyPath 	The path to a file containing the public key.
 */
bool SmartServerConnection::setLoginAppPublicKeyPath(
		const BW::string & publicKeyPath )
{
	struct stat publicKeyStat;
	if (0 != stat( publicKeyPath.c_str(), &publicKeyStat ))
	{
		return false;
	}

	char * buf = new char[publicKeyStat.st_size];

	FILE * publicKeyFile = fopen( publicKeyPath.c_str(), "r" );
	if (publicKeyFile == NULL)
	{
		delete [] buf;
		return false;
	}

	if (1 != fread( buf, publicKeyStat.st_size, 1, publicKeyFile ))
	{
		delete [] buf;
		fclose( publicKeyFile );
		return false;
	}
	fclose( publicKeyFile );

	if (!streamEncoder_.initFromKeyString( buf ))
	{
		delete [] buf;
		return false;
	}

	this->pLogOnParamsEncoder( &streamEncoder_ );

	delete [] buf;

	return true;
}


/**
 *	This method is used to log onto a server.
 *
 *	@param serverAddressString	The server address string, in the form
 *									&lt;hostname&gt;[:port]
 *								If not specified, port is assumed to be 20013.
 *	@param username				The username.
 *	@param password				The password.
 *	@param transport 			The connection transport to use.
 *
 *	@return 	true if the login attempt has started, false if failed.
 */
bool SmartServerConnection::logOnTo( const BW::string & serverAddressString,
	const BW::string & username,
	const BW::string & password,
	ConnectionTransport transport )
{
	if (serverAddressString.empty() ||
			username.empty())
	{
		ERROR_MSG( "Server address and username cannot be empty\n" );
		return false;
	}

	if (pInProgressLogin_)
	{
		ERROR_MSG( "SmartServerConnection::logOnTo: "
				"Logon already in progress.\n" );
		return false;
	}

	loggedOn_ = false;

	TRACE_MSG( "SmartServerConnection::logOnTo: transport = %s\n",
		ServerConnection::transportToCString( transport ) );

	pInProgressLogin_ = this->logOnBegin( serverAddressString.c_str(),
		username.c_str(), 
		password.c_str(),
		0,
		transport );

	return true;
}


/**
 *	This method disconnects the connection.
 */
void SmartServerConnection::logOff()
{
	if (pInProgressLogin_ != NULL)
	{
		pInProgressLogin_->finish();
		pInProgressLogin_ = NULL;
	}

	this->disconnect( true /*informServer*/, true /*shouldCondemnChannel*/ );
}


/**
 *	This method returns whether or not the connection is in the process of
 *	logging in.
 */
bool SmartServerConnection::isLoggingIn() const
{
	return pInProgressLogin_ != NULL;
}


/**
 *	This method ticks the connection. It needs to be called regularly to receive
 *	network data and also check for connection timeouts.
 *
 *	@param dt 	The time delta.
 *	@param pMessageHandler	The message handler to use for connection events.
 */
void SmartServerConnection::update( float dt,
		ServerMessageHandler * pMessageHandler )
{
	// ServerConnection::getMessageHandler() is NULL while we're connecting,
	// otherwise it's set from the ServerMessageHandler passed in to
	// logOnComplete.
	// This catches code which gives us a different pMessageHandler after we
	// have connected.
	MF_ASSERT( this->getMessageHandler() == NULL ||
		this->getMessageHandler() == pMessageHandler );

	clientTime_ += dt;

	this->processInput();

	if (pInProgressLogin_ && pInProgressLogin_->isDone())
	{
		if (!pInProgressLogin_->isBaseAppSwitch())
		{
			LogOnStatus status = 
				this->logOnComplete( pInProgressLogin_, pMessageHandler );

			// Release this first so that the callbacks don't see
			// isLoggingIn() == true
			pInProgressLogin_ = NULL;

			if (status.succeeded())
			{
				this->onLoggedOn();

				if (this->isOnline())
				{
					this->enableEntities();
				}
			}
			else
			{
				this->onLogOnFailure( status, this->errorMsg() );
			}
		}
		else
		{
			// BaseApp switch. No need to call logOn callback or enable
			// entities.
			if (!pInProgressLogin_->status().succeeded())
			{
				ERROR_MSG( "SmartServerConnection::update: "
						"Failed to switch BaseApps: %s\n",
					pInProgressLogin_->errorMsg().c_str() );
				// TODO: If this assertion holds, then onLoggedOff
				// will be called below anyway.
				MF_ASSERT( loggedOn_ && !this->isOnline() );
//				this->onLoggedOff();
			}
			else
			{
				DEBUG_MSG( "SmartServerConnection::update: "
						"Successfully switched BaseApps to %s\n", 
					pInProgressLogin_->baseAppAddr().c_str() );
			}

			pInProgressLogin_ = NULL;
		}
	}

	if (loggedOn_ && 
			!this->isSwitchingBaseApps() &&
			!this->isOnline())
	{
		this->onLoggedOff();
	}
}


/**
 *	This method sends any updated state to the server. It should be called
 *	paired with SmartServerConnection::update after any local state changes are
 *	made or game logic is run, to propagate those changes to the server.
 */
void SmartServerConnection::updateServer()
{
	this->sendIfNecessary();
}


/**
 *	This method returns the server time.
 */
double SmartServerConnection::serverTime() const
{
	return this->ServerConnection::serverTime( clientTime_ );
}


/**
 *	This method is called when the login has succeeded.
 */
void SmartServerConnection::onLoggedOn()
{
	if (pConnectionHandler_)
	{
		pConnectionHandler_->onLoggedOn();
	}

	loggedOn_ = true;
}


/**
 *	This method is called when the login has failed.
 *
 *	@param error 	The login error string.
 */
void SmartServerConnection::onLogOnFailure( const LogOnStatus & status,
	const BW::string & error )
{
	if (pConnectionHandler_)
	{
		pConnectionHandler_->onLogOnFailure( status, error );
	}

	loggedOn_ = false;
}


/**
 *	This method is called when the connection has logged off.
 */
void SmartServerConnection::onLoggedOff()
{
	if (pConnectionHandler_)
	{
		pConnectionHandler_->onLoggedOff();
	}

	loggedOn_ = false;
}


/**
 *	This method is overridden from ServerConnection, and is called when we are
 *	switching BaseApps to handle the new login handler.
 *
 *	@param loginHandler		The new login handler for connecting to the new 
 *							BaseApp.
 */
void SmartServerConnection::onSwitchBaseApp( LoginHandler & loginHandler )
{
	pInProgressLogin_ = &loginHandler;
}


/**
 *	This method returns whether we are currently switching between BaseApps.
 */
bool SmartServerConnection::isSwitchingBaseApps() const
{
	return (pInProgressLogin_ != NULL) && pInProgressLogin_->isBaseAppSwitch();
}

BW_END_NAMESPACE

// smart_server_connection.cpp

