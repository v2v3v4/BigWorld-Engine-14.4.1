#include "login_conditions_config.hpp"

#include "cstdmf/timestamp.hpp"

#define BW_CONFIG_CLASS LoginConditionsConfig
#define BW_CONFIG_PREFIX "loginConditions/"
#define BW_COMMON_PREFIX "dbApp/"

#include "server/server_app_option_macros.hpp"
#include "server/manager_app_config.hpp"


BW_BEGIN_NAMESPACE

BW_OPTION_FULL( float, maxLoad, 1.0f,
		"loginConditions/dbApp/maxLoad",
		"config/loginConditions/maxLoad" );

BW_OPTION_FULL( float, overloadTolerancePeriod, 5.0f,
		"loginConditions/overloadTolerancePeriod",
		"config/loginConditions/overloadTolerancePeriod" );

// -----------------------------------------------------------------------------
// Section: Post initialisation
// -----------------------------------------------------------------------------

/**
 *
 */
bool LoginConditionsConfig::postInit()
{
	BWConfig::update(
			"loginConditions/dbApp/overloadTolerancePeriod",
			overloadTolerancePeriod.getRef() );

	return true;
}

uint64 LoginConditionsConfig::overloadTolerancePeriodInStamps()
{
	return TimeStamp::fromSeconds( overloadTolerancePeriod() );
}

BW_END_NAMESPACE

// login_conditions_config.cpp
