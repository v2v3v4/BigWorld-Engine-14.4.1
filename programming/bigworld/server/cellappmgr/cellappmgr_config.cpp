#include "cellappmgr_config.hpp"

#include "cellappmgr.hpp" // For SCHEME_LARGEST
#include "cell_app_load_config.hpp"
#include "login_conditions_config.hpp"

#include "cstdmf/timestamp.hpp"
#include "server/balance_config.hpp"

#define BW_CONFIG_CLASS CellAppMgrConfig
#define BW_CONFIG_PREFIX "cellAppMgr/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: General options
// -----------------------------------------------------------------------------

BW_OPTION( int, maxLoadingCells, 4 );
BW_OPTION( float, minLoadingArea, 1000000.f );

BW_OPTION_RO( float, cellAppTimeout, 3.f );
DERIVED_BW_OPTION( uint64, cellAppTimeoutInStamps );

BW_OPTION_RO( float, archivePeriod, 0.f );
DERIVED_BW_OPTION( int, archivePeriodInTicks );
BW_OPTION_RO( bool, shouldArchiveSpaceData, true );

BW_OPTION( float, estimatedInitialCellLoad, 0.1f );
BW_OPTION_RO( float, loadBalancePeriod, 1.f );

BW_OPTION( int, metaLoadBalanceScheme, SCHEME_HYBRID );
BW_OPTION_RO( float, metaLoadBalancePeriod, 3.f );
BW_OPTION_RO( float, metaLoadBalanceTolerance, 0.05f );
BW_OPTION( bool, shouldShowMetaLoadBalanceDebug, false );

BW_OPTION_AT( float, shuttingDownDelay, 1.f, "" );

BW_OPTION( bool, shouldLimitBalanceToChunks, true );

BW_OPTION( float, loadSmoothingBias, 0.05f );

BW_OPTION_SETTER( bool, shouldMetaLoadBalance,
	BW::CellAppMgr::shouldMetaLoadBalance,
	BW::CellAppMgr::setShouldMetaLoadBalance );

BW_OPTION_RO_AT( float, ghostDistance, 500.f, "cellApp/" );

// -----------------------------------------------------------------------------
// Section: Post initialisation
// -----------------------------------------------------------------------------

/**
 *
 */
bool CellAppMgrConfig::postInit()
{
	if (!ManagerAppConfig::postInit())
	{
		return false;
	}

	if (!CellAppLoadConfig::postInit())
	{
		return false;
	}

	if (!BalanceConfig::postInit( CellAppMgrConfig::loadBalancePeriod() ))
	{
		return false;
	}

	if (!LoginConditionsConfig::postInit())
	{
		return false;
	}

	if (cellAppTimeout() > ServerAppConfig::channelTimeoutPeriod())
	{
		ERROR_MSG( "CellAppMgrConfig::postInit: "
				"cellAppMgr/cellAppTimeout (%.2f seconds) must not be greater "
				"than channelTimeoutPeriod (%.2f seconds)\n",
			cellAppTimeout(), ServerAppConfig::channelTimeoutPeriod() );
		return false;
	}

	cellAppTimeoutInStamps.set( TimeStamp::fromSeconds( cellAppTimeout() ) );

	archivePeriodInTicks.set( secondsToTicks( archivePeriod(), 0 ) );

	if (archivePeriodInTicks() > 0 && shouldArchiveSpaceData())
	{
		WARNING_MSG( "CellAppMgrConfig::postInit: "
				"cellAppMgr/shouldArchiveSpaceData is true. "
				"Space data archiving has been deprecated.\n" );
	}

	switch (metaLoadBalanceScheme())
	{
		case SCHEME_SMALLEST:
		case SCHEME_LARGEST:
		case SCHEME_HYBRID:
			break;
		default:
			ERROR_MSG( "CellAppMgr::init: Unknown metaLoadBalanceScheme "
				"'%d'\n", metaLoadBalanceScheme() );
			return false;
	}

	return true;
}

BW_END_NAMESPACE

// cellappmgr_config.cpp
