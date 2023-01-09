#include "server/server_app_config.hpp"


BW_BEGIN_NAMESPACE

class ReviverConfig : public ServerAppConfig
{
public:
	static ServerAppOption< float > reattachPeriod;
	static ServerAppOption< float > pingPeriod;
	static ServerAppOption< float > subjectTimeout;
	static ServerAppOption< bool > shutDownOnRevive;
	static ServerAppOption< int > timeoutInPings;
	static ServerAppOption< float > timeout;

	static bool postInit();
};

BW_END_NAMESPACE

// reviver_config.hpp
