#include "server/manager_app_config.hpp"


BW_BEGIN_NAMESPACE

class BaseAppMgrConfig : public ManagerAppConfig
{
public:
	static ServerAppOption< int > maxDestinationsInCreateBaseInfo;
	static ServerAppOption< float > updateCreateBaseInfoPeriod;
	static ServerAppOption< int > updateCreateBaseInfoPeriodInTicks;

	static ServerAppOption< float > baseAppTimeout;
	static ServerAppOption< uint64 > baseAppTimeoutInStamps;

	static bool postInit();
};

BW_END_NAMESPACE

// baseappmgr_config.hpp
