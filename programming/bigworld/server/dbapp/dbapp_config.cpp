#include "dbapp_config.hpp"
#include "login_conditions_config.hpp"

#define BW_CONFIG_CLASS DBAppConfig
#define BW_CONFIG_PREFIX "dbApp/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: General settings
// -----------------------------------------------------------------------------

// Top-level options.
BW_OPTION_AT( uint32, desiredBaseApps, 1, "" );
BW_OPTION_AT( uint32, desiredCellApps, 1, "" );
BW_OPTION_AT( uint32, desiredServiceApps, 1, "" );

// <dbapp> options.
BW_OPTION_RO( int, dumpEntityDescription, 0 );
BW_OPTION_RO( int, maxConcurrentEntityLoaders, 5 );
BW_OPTION_RO( int, numDBLockRetries, 30 );
BW_OPTION_RO( bool, shouldCacheLogOnRecords, false );
BW_OPTION( bool, shouldDelayLookUpSend, false );


/**
 *
 */
bool DBAppConfig::postInit()
{
	if (!ServerAppConfig::postInit() ||
		!LoginConditionsConfig::postInit())
	{
		return false;
	}

	return true;
}

BW_END_NAMESPACE

// dbapp_config.cpp
