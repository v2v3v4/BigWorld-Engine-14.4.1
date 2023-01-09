#ifndef MANAGER_APP_CONFIG_HPP
#define MANAGER_APP_CONFIG_HPP

#include "server_app_config.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class contains the configuration options for an EntityApp.
 */
class ManagerAppConfig : public ServerAppConfig
{
public:
	static ServerAppOption< bool > shutDownServerOnBadState;
	static ServerAppOption< bool > shutDownServerOnBaseAppDeath;
	static ServerAppOption< bool > shutDownServerOnCellAppDeath;
	static ServerAppOption< bool > shutDownServerOnServiceAppDeath;
	static ServerAppOption< bool > shutDownServerOnServiceDeath;
	static ServerAppOption< bool > isBadStateWithNoServiceApps;

protected:
	// static bool postInit() { return ServerAppConfig::postInit(); }
};

BW_END_NAMESPACE

#endif // MANAGER_APP_CONFIG_HPP
