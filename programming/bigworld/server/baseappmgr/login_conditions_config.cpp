#include "login_conditions_config.hpp"

#include "cstdmf/timestamp.hpp"

#define BW_CONFIG_CLASS LoginConditionsConfig
#define BW_CONFIG_PREFIX "loginConditions/"
#define BW_COMMON_PREFIX "baseApp/"

#include "server/server_app_option_macros.hpp"
#include "server/manager_app_config.hpp"


BW_BEGIN_NAMESPACE

BW_OPTION_FULL( float, minLoad, 0.8f,
		"loginConditions/baseApp/minLoad",
		"config/loginConditions/minLoad" );

// It's read-only because we don't want to recalculate the derived
// option minOverloadTolerancePeriodInStamps
BW_OPTION_FULL_RO( float, minOverloadTolerancePeriod, 5.0f,
		"loginConditions/overloadTolerancePeriod",
		"config/loginConditions/minOverloadTolerancePeriod" );

DERIVED_BW_OPTION_FULL( uint64, minOverloadTolerancePeriodInStamps,
		"config/loginConditions/minOverloadTolerancePeriodInStamps" );

BW_OPTION_FULL( int, overloadLogins, 10,
		"loginConditions/baseApp/overloadLogins",
		"config/loginConditions/overloadLogins" );

// -----------------------------------------------------------------------------
// Section: Post initialisation
// -----------------------------------------------------------------------------

/**
 *
 */
bool LoginConditionsConfig::postInit()
{
	BWConfig::update(
			"loginConditions/baseApp/overloadTolerancePeriod",
			minOverloadTolerancePeriod.getRef() );
	BWConfig::update(
			"loginConditions/baseApp/minLoad/overloadTolerancePeriod",
			minOverloadTolerancePeriod.getRef() );

	minOverloadTolerancePeriodInStamps.set(
			TimeStamp::fromSeconds( minOverloadTolerancePeriod() ) );

	return true;
}

BW_END_NAMESPACE

// login_conditions_config.cpp
