#include "baseappmgr_config.hpp"
#include "login_conditions_config.hpp"

#include "cstdmf/timestamp.hpp"

#define BW_CONFIG_CLASS BaseAppMgrConfig
#define BW_CONFIG_PREFIX "baseAppMgr/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: General settings
// -----------------------------------------------------------------------------

BW_OPTION( int, maxDestinationsInCreateBaseInfo, 250 );
BW_OPTION_RO( float, updateCreateBaseInfoPeriod, 5.f );
DERIVED_BW_OPTION( int, updateCreateBaseInfoPeriodInTicks );

BW_OPTION_RO( float, baseAppTimeout, 5.f );
DERIVED_BW_OPTION( uint64, baseAppTimeoutInStamps );


// -----------------------------------------------------------------------------
// Section: Post initialisation
// -----------------------------------------------------------------------------

/**
 *
 */
bool BaseAppMgrConfig::postInit()
{
	if (!ManagerAppConfig::postInit())
	{
		return false;
	}

	if (!LoginConditionsConfig::postInit())
	{
		return false;
	}

	if (baseAppTimeout() > ServerAppConfig::channelTimeoutPeriod())
	{
		ERROR_MSG( "BaseAppMgrConfig::postInit: "
				"baseAppMgr/baseAppTimeout (%.2f seconds) must not be greater "
				"than channelTimeoutPeriod (%.2f seconds)\n",
			baseAppTimeout(), ServerAppConfig::channelTimeoutPeriod() );
		return false;
	}

	updateCreateBaseInfoPeriodInTicks.set(
			secondsToTicks( updateCreateBaseInfoPeriod(), 1 ) );

	baseAppTimeoutInStamps.set(
		TimeStamp::fromSeconds( baseAppTimeout() ) );

	return true;
}

BW_END_NAMESPACE

// baseappmgr_config.cpp
