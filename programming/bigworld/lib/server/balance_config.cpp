#include "balance_config.hpp"

#include <math.h>

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS BalanceConfig
#define BW_CONFIG_PREFIX ""
#define BW_COMMON_PREFIX "balance/"
#include "server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: General configuration
// -----------------------------------------------------------------------------

const float LEVEL_STEP = 0.5f;

BW_OPTION( float, maxCPUOffload, 0.02f );
BW_OPTION_RO( int, numCPUOffloadLevels, 5 );
DERIVED_BW_OPTION( float, minCPUOffload );

BW_OPTION( float, aggressionDecrease, 0.5f );
DERIVED_BW_OPTION( float, aggressionIncrease );

BW_OPTION_RO( float, aggressionIncreasePeriod, 10.f );
BW_OPTION_RO( float, maxAggressionIncreasePeriod, 40.f );

BW_OPTION( float, maxAggression, 1.f );
DERIVED_BW_OPTION( float, minAggression );

BW_OPTION_FULL( bool, demo, false, "balance/demo/enable",
		"config/balance/demo/enable" );
BW_OPTION_FULL( float, demoNumEntitiesPerCell, 100.f,
		"balance/demo/numEntitiesPerCell",
		"config/balance/demo/numEntitiesPerCell" );

/**
 *	This method returns the amount of CPU to attempt to offload for each
 *	entity bounds level.
 */
float BalanceConfig::cpuOffloadForLevel( int level )
{
	// Each level offloads twice as much as the previous with the last level
	// offloading maxCPUOffload.
	return maxCPUOffload() * powf( LEVEL_STEP, level );
}


/**
 *  TODO: Deprecated. We now use levels and loads that are sent from CellApps.
 *	This method returns the entity level associated with an attempt to offload.
 */
int BalanceConfig::levelForCPUOffload( float loadToOffload )
{
	// TODO: Should we not offload anything if the difference is too small?
	if (loadToOffload <= 0.0001f)
	{
		return 0;
	}

	float offloadFraction = loadToOffload/maxCPUOffload();

	// Find 0.5^x = offloadFraction
	float numHalves = logf( offloadFraction )/logf( LEVEL_STEP );
	int level = int( ceilf( numHalves ) );

	return BalanceConfig::clampedOffloadLevel( level );
}


/**
 *	This method clamps an offload level to the range of valid levels.
 */
int BalanceConfig::clampedOffloadLevel( int level )
{
	return Math::clamp( 0, level, numCPUOffloadLevels() - 1 );
}


/**
 *	This method clamps an aggression to the range of values.
 */
float BalanceConfig::clampedAggression( float aggression )
{
	return Math::clamp( minAggression(), aggression, maxAggression() );
}


/**
 *
 */
bool BalanceConfig::postInit( float balancePeriod )
{
	const float numDecreaseSteps =
		maxAggressionIncreasePeriod()/aggressionIncreasePeriod();
	float minToMaxRatio = powf( aggressionDecrease(), numDecreaseSteps );
	minAggression.set( maxAggression() * minToMaxRatio );

	// Calculate the increase each tick so that it takes
	// aggressionIncreasePeriod amount of time to reclaim a decrease.
	float ticksToReclaim = aggressionIncreasePeriod()/balancePeriod;
	float reclaimAmount = 1.f/aggressionDecrease();

	aggressionIncrease.set( expf( logf( reclaimAmount )/ticksToReclaim ) );

	minCPUOffload.set( cpuOffloadForLevel( numCPUOffloadLevels() - 1 ) );

	return true;
}

BW_END_NAMESPACE

// balance_config.cpp
