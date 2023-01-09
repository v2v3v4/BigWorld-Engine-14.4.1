#ifndef DBAPP_MGR_CONFIG_HPP
#define DBAPP_MGR_CONFIG_HPP

#include "server/server_app_config.hpp"

#include "cstdmf/timestamp.hpp"


BW_BEGIN_NAMESPACE

class DBAppMgrConfig : public ServerAppConfig
{
public:

	static bool postInit();
};

BW_END_NAMESPACE

#endif // DBAPP_MGR_CONFIG_HPP
