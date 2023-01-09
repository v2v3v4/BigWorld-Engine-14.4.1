#ifndef BASE_APP_CONFIG_HPP
#define BASE_APP_CONFIG_HPP

#include "server/entity_app_config.hpp"
#include "server/external_app_config.hpp"


BW_BEGIN_NAMESPACE

class BaseAppConfig :
	public EntityAppConfig,
	public ExternalAppConfig
{
public:
	static ServerAppOption< float > createBaseElsewhereThreshold;

	static ServerAppOption< float > backupPeriod;
	static ServerAppOption< int > backupPeriodInTicks;
	static ServerAppOption< int > backupSizeWarningLevel;
	static ServerAppOption< bool > shouldLogBackups;

	static ServerAppOption< float > archivePeriod;
	static ServerAppOption< int > archivePeriodInTicks;
	static ServerAppOption< float > archiveEmergencyThreshold;

	static ServerAppOption< bool > sendAuthToClient;

	static ServerAppOption< bool > shouldShutDownIfPortUsed;

	static ServerAppOption< float > inactivityTimeout;
	static ServerAppOption< int > clientOverflowLimit;

	static ServerAppOption< int > bitsPerSecondToClient;
	static ServerAppOption< int > bytesPerPacketToClient;

	static ServerAppOption< bool > backUpUndefinedProperties;
	static ServerAppOption< bool > shouldResolveMailBoxes;
	static ServerAppOption< bool > warnOnNoDef;
	static ServerAppOption< float > loadSmoothingBias;

	static ServerAppOption< float > reservedTickFraction;
	static ServerAppOption< uint64 > reservedTickTime;

	static ServerAppOption< bool > verboseExternalInterface;
	static ServerAppOption< float > maxExternalSocketProcessingTime;
	static ServerAppOption< float > sendWindowCallbackThreshold;

	static ServerAppOption< bool > shouldDelayLookUpSend;

	static bool postInit();
};

BW_END_NAMESPACE

#endif // BASE_APP_CONFIG_HPP
