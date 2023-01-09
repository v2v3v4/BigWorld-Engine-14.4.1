#ifndef CELLAPPMGR_CONFIG_HPP
#define CELLAPPMGR_CONFIG_HPP

#include "server/manager_app_config.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class stores the configuration options for CellAppMgr.
 */
class CellAppMgrConfig : public ManagerAppConfig
{
public:
	enum MetaLoadBalanceScheme
	{
		SCHEME_LARGEST,
		SCHEME_SMALLEST,
		SCHEME_HYBRID
	};

	static ServerAppOption< int > maxLoadingCells;
	static ServerAppOption< float > minLoadingArea;

	static ServerAppOption< float > cellAppTimeout;
	static ServerAppOption< uint64 > cellAppTimeoutInStamps;

	static ServerAppOption< float > archivePeriod;
	static ServerAppOption< int > archivePeriodInTicks;
	static ServerAppOption< bool > shouldArchiveSpaceData;

	static ServerAppOption< float > estimatedInitialCellLoad;
	static ServerAppOption< float > loadBalancePeriod;

	static ServerAppOption< int > metaLoadBalanceScheme;
	static ServerAppOption< float > metaLoadBalancePeriod;
	static ServerAppOption< float > metaLoadBalanceTolerance;

	static ServerAppOption< bool > shouldShowMetaLoadBalanceDebug;

	static ServerAppOption< float > shuttingDownDelay;
	static int shuttingDownDelayInTicks()
		{ return secondsToTicks( shuttingDownDelay(), 0 ); }

	static ServerAppOption< bool > shouldLimitBalanceToChunks;
	static ServerAppOption< float > loadSmoothingBias;

	static ServerAppOptionGetSet< bool > shouldMetaLoadBalance;

	static ServerAppOption< float > ghostDistance;

	static bool shouldBalanceUnloadedChunks()
		{ return maxLoadingCells() > 0; }

	static bool postInit();
};

BW_END_NAMESPACE

#endif // CELLAPPMGR_CONFIG_HPP
