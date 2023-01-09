#include "loginapp_config.hpp"

#include "network/packet.hpp"

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS LoginAppConfig
#define BW_CONFIG_PREFIX "loginApp/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: General settings
// -----------------------------------------------------------------------------

BW_OPTION_RO( bool, shouldShutDownIfPortUsed, true );
BW_OPTION_RO( bool, verboseExternalInterface, false );
BW_OPTION_RO( float, maxExternalSocketProcessingTime,  1.0f );
BW_OPTION( float, maxLoginDelay, 10.f );

BW_OPTION_RO( BW::string, privateKey, "server/loginapp.privkey" );

BW_OPTION( bool, allowLogin, false );
BW_OPTION( bool, allowProbe, false );
BW_OPTION( bool, logProbes, true );

BW_OPTION_RO( bool, registerExternalInterface, false );
BW_OPTION( bool, allowUnencryptedLogins, false );

BW_OPTION( int, maxRepliesOnFailPerSecond, 100 );
BW_OPTION( bool, verboseLoginFailures, false);
BW_OPTION( int, loginRateLimit, 0 );
BW_OPTION( int, rateLimitDuration, 0 );

BW_OPTION_RO( uint, ipAddressRateLimit, 0 );
BW_OPTION_RO( uint, ipAddressPortRateLimit, 0 );

BW_OPTION( uint, maxUsernameLength, 256 );
BW_OPTION( uint, maxPasswordLength, 256 );
BW_OPTION( int, maxLoginMessageSize, PACKET_MAX_SIZE );

BW_OPTION_RO( bool, shouldOffsetExternalPortByUID, false );
BW_OPTION( bool, passwordlessLoginsOnly, false );
BW_OPTION( uint, ipBanListCleanupInterval, 10 );


// re-use numStartupRetries from general configuration
ServerAppOption< int > LoginAppConfig::numStartupRetries(
        60, "numStartupRetries", "" );

// suppress the option watcher registration
// the watcher will be registered as part of LoginApp initialization
ServerAppOption< BW::string > LoginAppConfig::challengeType(
		"", BW_OPTION_CONFIG_DIR "challengeType", "" );

// -----------------------------------------------------------------------------
// Section: Post initialisation
// -----------------------------------------------------------------------------

/**
 *
 */
bool LoginAppConfig::postInit()
{
	if (!ServerAppConfig::postInit() &&
			ExternalAppConfig::postInit())
	{
		return false;
	}

	BWConfig::update( "loginApp/externalLatencyMin", externalLatencyMin.getRef() );
	BWConfig::update( "loginApp/externalLatencyMax", externalLatencyMax.getRef() );
	BWConfig::update( "loginApp/externalLossRatio", externalLossRatio.getRef() );

	BWConfig::update( "loginApp/externalInterface", externalInterface.getRef() );

	return true;
}

BW_END_NAMESPACE

// loginapp_config.cpp
