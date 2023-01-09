#include "script/first_include.hpp"

#include "server_app_config.hpp"

#include "cstdmf/timestamp.hpp"
#include "cstdmf/profiler.hpp"
#include "network/keepalive_channels.hpp"
#include "network/udp_channel.hpp"
#include "pyscript/personality.hpp"

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS ServerAppConfig
#define BW_CONFIG_PREFIX ""
#include "server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE
/*   Server modes   */
static const BW::string DEFAULT_SERVER_MODE   = "standalone";
static const BW::string CENTER_SERVER_MODE    = "center";
static const BW::string PERIPHERY_SERVER_MODE = "periphery";


// -----------------------------------------------------------------------------
// Section: ServerAppConfig
// -----------------------------------------------------------------------------

ServerAppOption< int > ServerAppConfig::updateHertz( DEFAULT_GAME_UPDATE_HERTZ,
			"gameUpdateHertz", "gameUpdateHertz", Watcher::WT_READ_ONLY );

BW_OPTION_RO( BW::string, personality, DEFAULT_PERSONALITY_NAME );
BW_OPTION_RO( BW::string, serverMode, DEFAULT_SERVER_MODE );

ServerAppOption< bool >
	ServerAppConfig::isProduction( true, "production", "isProduction" );

BW_OPTION_RO( float, timeSyncPeriod, 60.f );
DERIVED_BW_OPTION( int, timeSyncPeriodInTicks );

BW_OPTION_RO( bool, useDefaultSpace, false );

BW_OPTION_RO( long, maxOpenFileDescriptors, -1 );

BW_OPTION_SETTER( float, channelTimeoutPeriod,
		Mercury::KeepAliveChannels::timeoutPeriod,
		Mercury::KeepAliveChannels::setTimeoutPeriod );

BW_OPTION_SETTER( bool, allowInteractiveDebugging,
		Mercury::UDPChannel::allowInteractiveDebugging,
		Mercury::UDPChannel::allowInteractiveDebugging );

BW_OPTION_RO( uint, maxSharedDataValueSize, 10240 );

ServerAppOption< float > ServerAppConfig::maxMgrRegisterStagger(
		0.0f, "maxMgrRegisterStagger", "" );

ServerAppOption< int > ServerAppConfig::numStartupRetries(
		60, "numStartupRetries", "" );


#if ENABLE_PROFILER
ServerAppOption< bool > ServerAppConfig::hasHitchDetection(
		false, "hitchDetection/enabled", "", Watcher::WT_READ_ONLY );

ServerAppOption< float > ServerAppConfig::hitchDetectionThreshold(
		1.5f, "hitchDetection/threshold", "", Watcher::WT_READ_ONLY );

BW_OPTION_RO( BW::string, profilerJsonDumpDirectory, "" );
BW_OPTION_RO( int, profilerMaxThreads, (int)Profiler::MAXIMUM_NUMBER_OF_THREADS );
#endif

/**
 *	This method initialises the configuration options of a ServerApp derived
 *	App. This is called by the bwMainT< APP >() template method.
 */
bool ServerAppConfig::init( bool (*postInitFn)() )
{
	ServerAppOptionIniter::initAll();
	bool result = (*postInitFn)();
	ServerAppOptionIniter::printAll();
	ServerAppOptionIniter::deleteAll();

	return result;
}


//-----------------------------------------------------------------------------
// Section: Post initialisation
//-----------------------------------------------------------------------------

/**
 *	This method is called just after initialisation but before the options are
 *	printed or used.
 */
bool ServerAppConfig::postInit()
{
	timeSyncPeriodInTicks.set( secondsToTicks( timeSyncPeriod(), 1 ) );

	/* verify server_mode is valid */
	if (( serverMode.getRef() != DEFAULT_SERVER_MODE ) &&
		( serverMode.getRef() != CENTER_SERVER_MODE ) &&
		( serverMode.getRef() != PERIPHERY_SERVER_MODE ))
	{
		ERROR_MSG( "ServerAppConfig::init: server_mode attribute in server/bw.xml "
			"is not in range: \"standalone\" \"center\" \"periphery\"" );
		return false;
	}

	const float maxMgrRegisterStaggerPeriod = 1.0f/ServerAppConfig::updateHertz();

	if (maxMgrRegisterStagger() > maxMgrRegisterStaggerPeriod)
	{
		maxMgrRegisterStagger.set( maxMgrRegisterStaggerPeriod );
	}

	return true;
}

// -----------------------------------------------------------------------------
// Section: Helper functions
// -----------------------------------------------------------------------------

/**
 *	This method converts a time duration from seconds to ticks.
 *
 *	@param seconds The number of seconds to convert.
 *	@param lowerBound A lower bound on the resulting value.
 */
int ServerAppConfig::secondsToTicks( float seconds, int lowerBound )
{
	return std::max( lowerBound,
			int( floorf( seconds * updateHertz() + 0.5f ) ) );
}

BW_END_NAMESPACE

// server_app_config.cpp
