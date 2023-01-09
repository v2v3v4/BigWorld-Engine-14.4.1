#include "rate_limit_config.hpp"

#include "baseapp_config.hpp"

#undef BW_CONFIG_CLASS
#undef BW_CONFIG_PREFIX

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS RateLimitConfig
#define BW_CONFIG_PREFIX "baseApp/"
#define BW_COMMON_PREFIX "clientUpstreamLimits/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: RateLimitConfig
// -----------------------------------------------------------------------------

BW_OPTION( uint, warnMessagesPerSecond, 250 );
BW_OPTION( uint, maxMessagesPerSecond, 500 );
BW_OPTION( uint, warnBytesPerSecond, 20480 );
BW_OPTION( uint, maxBytesPerSecond, 40960 );

DERIVED_BW_OPTION( uint, warnMessagesPerTick );
DERIVED_BW_OPTION( uint, maxMessagesPerTick );
DERIVED_BW_OPTION( uint, warnBytesPerTick );
DERIVED_BW_OPTION( uint, maxBytesPerTick );

BW_OPTION( uint, warnMessagesBuffered, 500 );
BW_OPTION( uint, maxMessagesBuffered, 1000 );
BW_OPTION( uint, warnBytesBuffered, 262144 );
BW_OPTION( uint, maxBytesBuffered, 524288 );



// -----------------------------------------------------------------------------
// Section: Custom initialisation
// -----------------------------------------------------------------------------

/**
 *	This method is called after the configuration options have been read but
 *	before they have been printed.
 */
bool RateLimitConfig::postInit()
{
#define DERIVE_RATE_LIMIT( NAME ) 											\
	NAME##PerTick.set( NAME##PerSecond() / BaseAppConfig::updateHertz() );

	DERIVE_RATE_LIMIT( warnMessages );
	DERIVE_RATE_LIMIT( maxMessages );
	DERIVE_RATE_LIMIT( warnBytes );
	DERIVE_RATE_LIMIT( maxBytes );

	return true;
}

BW_END_NAMESPACE

// rate_limit_config.cpp
