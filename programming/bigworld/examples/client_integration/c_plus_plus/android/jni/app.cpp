

#include "app.hpp"
//#include "loginapp_key.hpp"
//#include "loop_task.hpp"
//#include "timer.hpp"

#include "cstdmf/timestamp.hpp"
#include "cstdmf/bgtask_manager.hpp"


#include <stdlib.h>

#include <unistd.h>

#include <android/log.h>

BW_BEGIN_NAMESPACE
BW_SINGLETON_STORAGE( App );
BW_END_NAMESPACE

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

// -----------------------------------------------------------------------------
// Section: ConnectionTicker
// -----------------------------------------------------------------------------

namespace  // (anonymous)
{

const char DEFAULT_SERVER_ADDRESS_STRING[] = "10.40.1.102:20077";
const char DEFAULT_USERNAME[] = "AndroidClient";
const char DEFAULT_PASSWORD[] = "pass";

const char LOGINAPP_PUBLIC_KEY[] =
"-----BEGIN PUBLIC KEY-----\n"
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA7/MNyWDdFpXhpFTO9LHz\n"
"CUQPYv2YP5rqJjUoxAFa3uKiPKbRvVFjUQ9lGHyjCmtixBbBqCTvDWu6Zh9Imu3x\n"
"KgCJh6NPSkddH3l+C+51FNtu3dGntbSLWuwi6Au1ErNpySpdx+Le7YEcFviY/ClZ\n"
"ayvVdA0tcb5NVJ4Axu13NvsuOUMqHxzCZRXCe6nyp6phFP2dQQZj8QZp0VsMFvhh\n"
"MsZ4srdFLG0sd8qliYzSqIyEQkwO8TQleHzfYYZ90wPTCOvMnMe5+zCH0iPJMisP\n"
"YB60u6lK9cvDEeuhPH95TPpzLNUFgmQIu9FU8PkcKA53bj0LWZR7v86Oco6vFg6V\n"
"sQIDAQAB\n"
"-----END PUBLIC KEY-----\n";

} // end namespace (anonymous)


// -----------------------------------------------------------------------------
// Section: class App
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
App::App():
		running_( false ),
		stopRequested_( false ),
		lastTime_(0),
		pConnectionTicker_(),
		spaceDataStorage_( new BW::SpaceDataMappings() ),
		entityFactory_(),
		connectionHelper_( 
			entityFactory_,
			spaceDataStorage_,
			DefsDigest::constants() ),
		gameLogicFactory_(),
		pConnection_( connectionHelper_.createServerConnection( 0.0 ) ),
		serverAddressString_( DEFAULT_SERVER_ADDRESS_STRING ),
		username_( DEFAULT_USERNAME ),
		password_( DEFAULT_PASSWORD )
{
	pConnection_->setHandler( this );

	entityFactory_.addExtensionFactory( &gameLogicFactory_ );
}


/**
 *	Destructor.
 */
App::~App()
{
	this->fini();
}

/**
 *	This method initialises the application.
 *	It must be followed by regular calls to App::update().
 *
  *	@return True on success, false on initialisation failure.
 */
bool App::init()
{
	if (!pConnection_->setLoginAppPublicKeyString( LOGINAPP_PUBLIC_KEY ))
	{
		return false;
	}

	if (!pConnection_->logOnTo( serverAddressString_,
			username_,
			password_ ))
	{
		return false;
	}

	running_ = true;

	return true;
}

/**
 *	This method shuts down the application.
 */
void App::fini()
{
	if (!running_)
	{
		return;
	}
	running_ = false;
	pConnection_->logOff();
	pConnection_.reset( NULL );
}

/**
 *	This method shuts down the application.
 *
 *	@return True on success, false to request the shutdown.
 */
bool App::update()
{
	if ( stopRequested_ )
	{
		return false;
	}

/*
	float now = Timer::getTime();
	if (lastTime_ == 0.0f)
	{
		lastTime_ = now;
	}
	float dt = now - lastTime_;
	lastTime_ = now;
*/
	float dt = 1.0f/60.0f;

	connectionHelper_.update( pConnection_.get(), dt );
	connectionHelper_.updateServer( pConnection_.get() );

	return true;
}


/*
 *	Override of ServerConnectionHandler.
 */
void App::onLoggedOn()
{
	LOGI( "App::onLoggedOn:\n" );
}


/*
 *	Override of ServerConnectionHandler.
 */
void App::onLogOnFailure( const BW::LogOnStatus & status,
	const BW::string & errorMessage )
{
	LOGI( "App::onLogOnFailure: %s\n", errorMessage.c_str() );
	stopRequested_ = true;
}


/*
 *	Override of ServerConnectionHandler.
 */
void App::onLoggedOff()
{
	LOGI( "App::onLoggedOff\n" );
	stopRequested_ = true;
}

// app.cpp

