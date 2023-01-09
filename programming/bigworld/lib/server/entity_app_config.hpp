#ifndef ENTITY_APP_CONFIG_HPP
#define ENTITY_APP_CONFIG_HPP

#include "server_app_config.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class contains the configuration options for an EntityApp.
 */
class EntityAppConfig : public ServerAppConfig
{
protected:
	static bool postInit();
};

BW_END_NAMESPACE

#endif // ENTITY_APP_CONFIG_HPP
