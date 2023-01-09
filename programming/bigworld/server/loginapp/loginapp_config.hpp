#include "server/external_app_config.hpp"
#include "server/server_app_config.hpp"

#include "cstdmf/timestamp.hpp"


BW_BEGIN_NAMESPACE

class LoginAppConfig :
	public ServerAppConfig,
	public ExternalAppConfig
{
public:
	static ServerAppOption< bool > shouldShutDownIfPortUsed;
	static ServerAppOption< bool > verboseExternalInterface;
	static ServerAppOption< float > maxExternalSocketProcessingTime;
	static ServerAppOption< float > maxLoginDelay;

	static uint64 maxLoginDelayInStamps()
	{
		return TimeStamp::fromSeconds( maxLoginDelay() );
	}

	static ServerAppOption< BW::string > privateKey;

	static ServerAppOption< bool > allowLogin;
	static ServerAppOption< bool > allowProbe;
	static ServerAppOption< bool > logProbes;

	static ServerAppOption< bool > registerExternalInterface;
	static ServerAppOption< bool > allowUnencryptedLogins;

	static ServerAppOption< int > maxRepliesOnFailPerSecond;
	static ServerAppOption< bool > verboseLoginFailures;
	static ServerAppOption< int > loginRateLimit;
	static ServerAppOption< int > rateLimitDuration;

	static ServerAppOption< uint > ipAddressRateLimit;
	static ServerAppOption< uint > ipAddressPortRateLimit;

	static ServerAppOption< uint > maxUsernameLength;
	static ServerAppOption< uint > maxPasswordLength;
	static ServerAppOption< int > maxLoginMessageSize;

	static ServerAppOption< bool > shouldOffsetExternalPortByUID;

	static ServerAppOption< int > numStartupRetries;
	static ServerAppOption< bool > passwordlessLoginsOnly;
	static ServerAppOption< uint > ipBanListCleanupInterval;

	static ServerAppOption< BW::string > challengeType;


	static uint64 rateLimitDurationInStamps()
	{
		return TimeStamp::fromSeconds( rateLimitDuration() );
	}

	static bool rateLimitEnabled()
	{
		return (rateLimitDuration() > 0);
	}

	static bool postInit();
};

BW_END_NAMESPACE

// loginapp_config.hpp
