#include "baseapp_config.hpp"

#include "download_streamer_config.hpp"
#include "rate_limit_config.hpp"

#include "cstdmf/timestamp.hpp"
#include "server/util.hpp"

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS BaseAppConfig
#define BW_CONFIG_PREFIX "baseApp/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: General settings
// -----------------------------------------------------------------------------

BW_OPTION( float, createBaseElsewhereThreshold, 0.f );

BW_OPTION( float, backupPeriod, 10.f );
DERIVED_BW_OPTION( int, backupPeriodInTicks );
BW_OPTION( int, backupSizeWarningLevel, 102400 );
BW_OPTION( bool, shouldLogBackups, false );

BW_OPTION( float, archivePeriod, 100.f );
DERIVED_BW_OPTION( int, archivePeriodInTicks );
BW_OPTION( float, archiveEmergencyThreshold, 0.25f );

BW_OPTION_RO( bool, sendAuthToClient, false );

BW_OPTION_RO( bool, shouldShutDownIfPortUsed, true );

BW_OPTION( float, inactivityTimeout, 30.f );
BW_OPTION( int, clientOverflowLimit, 1000 );

BW_OPTION_RO_AT( int, bitsPerSecondToClient, 20000, "" );
DERIVED_BW_OPTION( int, bytesPerPacketToClient );

BW_OPTION( bool, backUpUndefinedProperties, false );
BW_OPTION( bool, shouldResolveMailBoxes, false );
BW_OPTION( bool, warnOnNoDef, true );

BW_OPTION( float, loadSmoothingBias, 0.01f );

BW_OPTION_RO( float, reservedTickFraction, 0.05f );
DERIVED_BW_OPTION( uint64, reservedTickTime );

BW_OPTION_RO( bool, verboseExternalInterface, false );
BW_OPTION_RO( float, maxExternalSocketProcessingTime, -1.f );

BW_OPTION_RO( float, sendWindowCallbackThreshold, 0.5f );

BW_OPTION( bool, shouldDelayLookUpSend, false );

// -----------------------------------------------------------------------------
// Section: Custom initialisation
// -----------------------------------------------------------------------------

/**
 *	This method is called after the configuration options have been read but
 *	before they have been printed.
 */
bool BaseAppConfig::postInit()
{
	if (!EntityAppConfig::postInit() ||
			!ExternalAppConfig::postInit() ||
			!RateLimitConfig::postInit() ||
			!DownloadStreamerConfig::postInit())
	{
		return false;
	}

	bool result = true;

	BWConfig::update( "baseApp/externalLatencyMin", externalLatencyMin.getRef() );
	BWConfig::update( "baseApp/externalLatencyMax", externalLatencyMax.getRef() );
	BWConfig::update( "baseApp/externalLossRatio", externalLossRatio.getRef() );

	BWConfig::update( "baseApp/externalInterface", externalInterface.getRef() );

	backupPeriodInTicks.set( secondsToTicks( backupPeriod(), 0 ) );
	archivePeriodInTicks.set( secondsToTicks( archivePeriod(), 0 ) );

	bytesPerPacketToClient.set(
			ServerUtil::bpsToPacketSize( bitsPerSecondToClient(), updateHertz() ) );

	reservedTickTime.set( 
		uint64( reservedTickFraction()/updateHertz()/stampsPerSecondD() ) );

	if (maxExternalSocketProcessingTime() < 0.f)
	{
		maxExternalSocketProcessingTime.set( expectedTickPeriod() );
	}

	return result;
}

BW_END_NAMESPACE

// baseapp_config.cpp
