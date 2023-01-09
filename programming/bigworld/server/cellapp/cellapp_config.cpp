#include "cellapp_config.hpp"

#include "aoi_update_schemes.hpp"
#include "noise_config.hpp"

#include "chunk/base_chunk_space.hpp"
#include "network/msgtypes.hpp"
#include "server/balance_config.hpp"
#include "terrain/terrain_settings.hpp"

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS CellAppConfig
#define BW_CONFIG_PREFIX "cellApp/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

namespace
{
const float DEFAULT_AOI_RADIUS = 500.f;
}

// -----------------------------------------------------------------------------
// Section: Standard options
// -----------------------------------------------------------------------------

BW_OPTION   ( float,  loadSmoothingBias, 0.05f );
BW_OPTION_RO( float,  ghostDistance,     DEFAULT_AOI_RADIUS );
BW_OPTION_RO( float,  maxAoIRadius,      DEFAULT_AOI_RADIUS );
BW_OPTION   ( float,  defaultAoIRadius,  DEFAULT_AOI_RADIUS );
DERIVED_BW_OPTION( float,  packedXZScale       ); // Set in postInit

BW_OPTION   ( bool,   shouldResolveMailBoxes, false );
BW_OPTION   ( int,    entitySpamSize,         200 );
BW_OPTION   ( uint,    maxGhostsToDelete,     100 );
BW_OPTION_RO( float,  minGhostLifespan,       5.f );
DERIVED_BW_OPTION( int,    minGhostLifespanInTicks ); // Set in postInit
BW_OPTION   ( bool,   fastShutdown,           true );

// Only used during CellApp::init.
BW_OPTION_RO( bool, loadDominantTextureMaps, false );

BW_OPTION_RO( float, maxTimeBetweenPlayerUpdates, 0.f );
DERIVED_BW_OPTION( int, maxTimeBetweenPlayerUpdatesInTicks ); // Set in postInit

BW_OPTION_RO( float, backupPeriod, 10.f );
DERIVED_BW_OPTION( int,   backupPeriodInTicks ); // Set in postInit

BW_OPTION_RO( float, checkOffloadsPeriod, 0.1f );
DERIVED_BW_OPTION( int,   checkOffloadsPeriodInTicks ); // Set in postInit

BW_OPTION_RO( float, chunkLoadingPeriod, 0.02f );

BW_OPTION_RO( int, ghostUpdateHertz, 50 );

BW_OPTION_RO( double, reservedTickFraction, 0.05 );

BW_OPTION_RO( int, obstacleTreeDepth, BaseChunkSpace::obstacleTreeDepth() );

BW_OPTION_RO( float, sendWindowCallbackThreshold, 0.5f );

BW_OPTION( int, expectedMaxControllers, 25 );
BW_OPTION( int, absoluteMaxControllers, 50 );

BW_OPTION( bool, enforceGhostDecorators, true );
BW_OPTION( bool, treatAllOtherEntitiesAsGhosts, true );

BW_OPTION( float, maxPhysicsNetworkJitter, 0.2f );

BW_OPTION( float, maxTickStagger, 0.0f );

BW_OPTION( bool, shouldNavigationDropPosition, false );

BW_OPTION( bool, sendDetailedPlayerVehicles, false );

BW_OPTION( float, navigationMaxSlope, 45.f );
DERIVED_BW_OPTION( float, navigationMaxSlopeGradient ); // Set in postInit
BW_OPTION( float, navigationMaxClimb, 0.65f );

BW_OPTION( float, witnessUpdateMaxPriorityDelta, 5.f );
BW_OPTION( float, witnessUpdateDefaultMinDelta, 1.f );
BW_OPTION( float, witnessUpdateDefaultMaxDelta, 101.f );
BW_OPTION( float, witnessUpdateDeltaGrowthThrottle, 1.f + 1.f / 8.f);

// -----------------------------------------------------------------------------
// Section: Custom initialisation
// -----------------------------------------------------------------------------

/**
 *	This method is called after the configuration options have been read but
 *	before they have been printed.
 */
bool CellAppConfig::postInit()
{
	bool result = EntityAppConfig::postInit() &&
		NoiseConfig::postInit() &&
		AoIUpdateSchemes::init() &&
		// TODO: Move balancePeriod from cellAppMgr
		BalanceConfig::postInit( 1.f );

	CellAppConfig::updateDerivedSettings();

	return result && CellAppConfig::applySettings() &&
		CellAppConfig::sanityCheckSettings();
}


/**
 *	This method updates options that depend on other options.
 */
void CellAppConfig::updateDerivedSettings()
{
	const float maxPackedRelativePositionValue = floorf(
		std::min( PackedXZ::maxLimit( 1.f ), PackedXYZ::maxLimit( 1.f ) ) );

	packedXZScale.set( maxAoIRadius() / maxPackedRelativePositionValue );

	MF_ASSERT( PackedXZ::maxLimit( packedXZScale() ) >= maxAoIRadius() );
	MF_ASSERT( PackedXYZ::maxLimit( packedXZScale() ) >= maxAoIRadius() );

	maxTimeBetweenPlayerUpdatesInTicks.set( secondsToTicks(
		maxTimeBetweenPlayerUpdates(), -1 ) );
	minGhostLifespanInTicks.set( secondsToTicks( minGhostLifespan(), 0 ) );
	backupPeriodInTicks.set( secondsToTicks( backupPeriod(), 0 ) );
	checkOffloadsPeriodInTicks.set( secondsToTicks( checkOffloadsPeriod(), 1 ) );

	navigationMaxSlopeGradient.set ( 
		tanf( DEG_TO_RAD( CellAppConfig::navigationMaxSlope() ) ) );
}


/**
 *	This method applies any settings that need to be transferred to other
 *	locations like library settings.
 */
bool CellAppConfig::applySettings()
{
	Terrain::TerrainSettings::loadDominantTextureMaps(
			loadDominantTextureMaps() );

	BaseChunkSpace::obstacleTreeDepth( obstacleTreeDepth() );

	return true;
}


/**
 *	This method sanity checks the values of the configuration options.
 */
bool CellAppConfig::sanityCheckSettings()
{
	bool result = true;

	if (packedXZScale() <= 0)
	{
		ERROR_MSG( "CellAppConfig::sanityCheckSettings: PackedXZ Scale must "
				"have a positive value. It currently has a value of %.3f\n",
				packedXZScale() );
		result = false;
	}

	if (maxAoIRadius() > ghostDistance())
	{
		ERROR_MSG( "CellAppConfig::sanityCheckSettings: maxAoIRadius (%.1f) "
				"must not be greater than ghostDistance (%.1f)\n",
			maxAoIRadius(), ghostDistance() );
		result = false;
	}
	else if (maxAoIRadius() < ghostDistance())
	{
		INFO_MSG( "CellAppConfig::sanityCheckSettings: maxAoIRadius (%.1f) "
				"is less than ghostDistance (%.1f). "
				"They usually have the same value\n",
			maxAoIRadius(), ghostDistance() );
	}

	if (defaultAoIRadius() > maxAoIRadius())
	{
		ERROR_MSG( "CellAppConfig::sanityCheckSettings: defaultAoIRadius "
				"(%.1f) must not be greater than maxAoIRadius (%.1f)\n",
			defaultAoIRadius(), maxAoIRadius() );
		result = false;
	}

	if (witnessUpdateMaxPriorityDelta() <= 0.f)
	{
		ERROR_MSG( "CellAppConfig::sanityCheckSettings: "
				"witnessUpdateMaxPriorityDelta (%.1f) must have a "
				"positive value\n",
			witnessUpdateMaxPriorityDelta() );
		result = false;
	}

	if (witnessUpdateDefaultMinDelta() <= 0.f)
	{
		ERROR_MSG( "CellAppConfig::sanityCheckSettings: "
				"witnessUpdateDefaultMinDelta (%.1f) must have a "
				"positive value\n",
			witnessUpdateDefaultMinDelta() );
		result = false;
	}

	if (witnessUpdateDefaultMaxDelta() <= witnessUpdateDefaultMinDelta())
	{
		ERROR_MSG( "CellAppConfig::sanityCheckSettings: "
				"witnessUpdateDefaultMaxDelta (%.1f) must be larger than "
				"witnessUpdateDefaultMinDelta (%.1f)\n",
			witnessUpdateDefaultMaxDelta(),
			witnessUpdateDefaultMinDelta() );
		result = false;
	}

	if (witnessUpdateDeltaGrowthThrottle() < 1.f)
	{
		ERROR_MSG( "CellAppConfig::sanityCheckSettings: "
				"witnessUpdateDeltaGrowthThrottle (%.1f) must have a "
				"value not less than 1.0\n",
			witnessUpdateDeltaGrowthThrottle() );
		result = false;
	}

	return result;
}

BW_END_NAMESPACE

// cellapp_config.cpp
