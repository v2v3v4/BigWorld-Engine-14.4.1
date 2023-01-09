#include "dbappmgr_config.hpp"

#include "network/packet.hpp"

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS DBAppMgrConfig
#define BW_CONFIG_PREFIX "dbAppMgr/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: General settings
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Section: Post initialisation
// -----------------------------------------------------------------------------

/**
 *
 */
bool DBAppMgrConfig::postInit()
{
	if (!ServerAppConfig::postInit())
	{
		return false;
	}

	return true;
}

BW_END_NAMESPACE

// dbappmgr_config.cpp
