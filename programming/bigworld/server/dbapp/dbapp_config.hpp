#include "server/server_app_config.hpp"

BW_BEGIN_NAMESPACE

class DBAppConfig : public ServerAppConfig
{
public:
	static ServerAppOption< int > dumpEntityDescription;
	static ServerAppOption< uint32 > desiredBaseApps;
	static ServerAppOption< uint32 > desiredCellApps;
	static ServerAppOption< uint32 > desiredServiceApps;
	static ServerAppOption< int > maxConcurrentEntityLoaders;
	static ServerAppOption< int > numDBLockRetries;
	static ServerAppOption< bool > shouldCacheLogOnRecords;
	static ServerAppOption< bool > shouldDelayLookUpSend;

	// From LoginConditionsConfig
	static ServerAppOption< float > overloadLevel;
	static ServerAppOption< float > overloadTolerancePeriod;

	// Shared DB config is available via DBConfig::get() (lib/db/db_config.hpp)

	static bool postInit();
};

BW_END_NAMESPACE

// dbapp_config.hpp
