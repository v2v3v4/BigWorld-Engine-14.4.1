#ifndef BALANCE_CONFIG_HPP
#define BALANCE_CONFIG_HPP

#include "server_app_option.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class contains the configuration options for an EntityApp.
 */
class BalanceConfig
{
public:
	static ServerAppOption< float > maxCPUOffload;
	static ServerAppOption< int > numCPUOffloadLevels;
	static ServerAppOption< float > minCPUOffload;

	static ServerAppOption< float > aggressionDecrease;
	static ServerAppOption< float > aggressionIncrease;

	static ServerAppOption< float > aggressionIncreasePeriod;
	static ServerAppOption< float > maxAggressionIncreasePeriod;

	static ServerAppOption< float > maxAggression;
	static ServerAppOption< float > minAggression;

	static ServerAppOption< bool > demo;
	static ServerAppOption< float > demoNumEntitiesPerCell;

	static float cpuOffloadForLevel( int level );
	static int levelForCPUOffload( float loadToOffload );

	static int clampedOffloadLevel( int level );
	static float clampedAggression( float aggression );

	static bool postInit( float balancePeriod );
};

BW_END_NAMESPACE

#endif // BALANCE_CONFIG_HPP
