#include "login_conditions_config.hpp"

#include "cstdmf/timestamp.hpp"

#define BW_CONFIG_CLASS LoginConditionsConfig
#define BW_CONFIG_PREFIX "loginConditions/"
#define BW_COMMON_PREFIX "cellApp/"

#include "server/server_app_option_macros.hpp"
#include "server/manager_app_config.hpp"


BW_BEGIN_NAMESPACE

BW_OPTION_FULL( float, avgLoad, 0.85f,
		"loginConditions/cellApp/avgLoad",
		"config/loginConditions/avgLoad" );

// It's read-only because we don't want to recalculate the derived
// option avgOverloadTolerancePeriodInStamps
BW_OPTION_FULL_RO( float, avgOverloadTolerancePeriod, 5.0f,
		"loginConditions/overloadTolerancePeriod",
		"config/loginConditions/avgOverloadTolerancePeriod" );

DERIVED_BW_OPTION_FULL( uint64, avgOverloadTolerancePeriodInStamps,
		"config/loginConditions/avgOverloadTolerancePeriodInStamps" );

BW_OPTION_FULL( float, maxLoad, 0.9f,
		"loginConditions/cellApp/maxLoad",
		"config/loginConditions/maxLoad" );

// It's read-only because we don't want to recalculate the derived
// option maxOverloadTolerancePeriodInStamps
BW_OPTION_FULL_RO( float, maxOverloadTolerancePeriod, 5.0f,
		"loginConditions/overloadTolerancePeriod",
		"config/loginConditions/maxOverloadTolerancePeriod" );

DERIVED_BW_OPTION_FULL( uint64, maxOverloadTolerancePeriodInStamps,
		"config/loginConditions/maxOverloadTolerancePeriodInStamps" );

// -----------------------------------------------------------------------------
// Section: Post initialisation
// -----------------------------------------------------------------------------

/**
 *
 */
bool LoginConditionsConfig::postInit()
{
	BWConfig::update(
			"loginConditions/cellApp/overloadTolerancePeriod",
			avgOverloadTolerancePeriod.getRef() );
	BWConfig::update(
			"loginConditions/cellApp/avgLoad/overloadTolerancePeriod",
			avgOverloadTolerancePeriod.getRef() );

	BWConfig::update(
			"loginConditions/cellApp/overloadTolerancePeriod",
			maxOverloadTolerancePeriod.getRef() );
	BWConfig::update(
			"loginConditions/cellApp/maxLoad/overloadTolerancePeriod",
			maxOverloadTolerancePeriod.getRef() );

	if (avgLoad() >= maxLoad())
	{
		WARNING_MSG( "LoginConditionsConfig: loginConditions/cellApp/avgLoad"
			"should be less than loginConditions/cellApp/maxLoad\n" );
	}

	avgOverloadTolerancePeriodInStamps.set(
			TimeStamp::fromSeconds( avgOverloadTolerancePeriod() ) );
	maxOverloadTolerancePeriodInStamps.set(
			TimeStamp::fromSeconds( maxOverloadTolerancePeriod() ) );


	return true;
}

BW_END_NAMESPACE

// login_conditions_config.cpp
