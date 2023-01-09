#ifndef CELL_APP_CONFIG_HPP
#define CELL_APP_CONFIG_HPP

#include "server/entity_app_config.hpp"


BW_BEGIN_NAMESPACE

class CellAppConfig : public EntityAppConfig
{
public:

	static ServerAppOption< float > loadSmoothingBias;
	static ServerAppOption< float > ghostDistance;
	static ServerAppOption< float > maxAoIRadius;
	static ServerAppOption< float > defaultAoIRadius;
	static ServerAppOption< float > packedXZScale;
	static ServerAppOption< bool > fastShutdown;
	static ServerAppOption< bool > shouldResolveMailBoxes;
	static ServerAppOption< int > entitySpamSize;

	static ServerAppOption< uint > maxGhostsToDelete;
	static ServerAppOption< float > minGhostLifespan;
	static ServerAppOption< int > minGhostLifespanInTicks;

	static ServerAppOption< bool > loadDominantTextureMaps;

	static ServerAppOption< float > maxTimeBetweenPlayerUpdates;
	static ServerAppOption< int > maxTimeBetweenPlayerUpdatesInTicks;

	static ServerAppOption< float > backupPeriod;
	static ServerAppOption< int > backupPeriodInTicks;

	static ServerAppOption< float > checkOffloadsPeriod;
	static ServerAppOption< int > checkOffloadsPeriodInTicks;

	static ServerAppOption< float > chunkLoadingPeriod;

	static ServerAppOption< int > ghostUpdateHertz;
	static ServerAppOption< double > reservedTickFraction;

	static ServerAppOption< int > obstacleTreeDepth;
	static ServerAppOption< float > sendWindowCallbackThreshold;

	static ServerAppOption< int > expectedMaxControllers;
	static ServerAppOption< int > absoluteMaxControllers;

	static ServerAppOption< bool > enforceGhostDecorators;
	static ServerAppOption< bool > treatAllOtherEntitiesAsGhosts;

	static ServerAppOption< float > maxPhysicsNetworkJitter;

	static ServerAppOption< float > maxTickStagger;

	static ServerAppOption< bool > shouldNavigationDropPosition;

	static ServerAppOption< bool > sendDetailedPlayerVehicles;

	static ServerAppOption< float > navigationMaxSlope;
	static ServerAppOption< float > navigationMaxSlopeGradient;
	static ServerAppOption< float > navigationMaxClimb;

	static ServerAppOption< float > witnessUpdateMaxPriorityDelta;
	static ServerAppOption< float > witnessUpdateDefaultMinDelta;
	static ServerAppOption< float > witnessUpdateDefaultMaxDelta;
	static ServerAppOption< float > witnessUpdateDeltaGrowthThrottle;

	static bool postInit();

private:
	static void updateDerivedSettings();
	static bool applySettings();
	static bool sanityCheckSettings();
};

BW_END_NAMESPACE

#endif // CELL_APP_CONFIG_HPP
