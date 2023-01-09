#ifndef SERVER__TOOLS__BOTS__BOTS_CONFIG_HPP
#define SERVER__TOOLS__BOTS__BOTS_CONFIG_HPP

#include "cstdmf/bw_namespace.hpp"

#include "server/server_app_config.hpp"


BW_BEGIN_NAMESPACE

class BotsConfig : public ServerAppConfig
{
public:
	static ServerAppOption< BW::string > username;
	static ServerAppOption< BW::string > password;
	static ServerAppOption< BW::string > tag;
	static ServerAppOption< BW::string > serverName;
	static ServerAppOption< bool > shouldUseTCP;
	static ServerAppOption< bool > shouldUseWebSockets;
	static ServerAppOption< int > transport;
	static ServerAppOption< uint16 > port;
	static ServerAppOption< bool > shouldUseRandomName;
	static ServerAppOption< bool > shouldUseScripts;
	static ServerAppOption< BW::string > standinEntity;
	static ServerAppOption< BW::string > controllerType;
	static ServerAppOption< BW::string > controllerData;
	static ServerAppOption< BW::string > publicKey;
	static ServerAppOption< float > logOnRetryPeriod;
	static ServerAppOption< bool > shouldListenForGeometryMappings;
	static ServerAppOption< float > chunkLoadingPeriod;

	static bool postInit();

};

BW_END_NAMESPACE

#endif // SERVER__TOOLS__BOTS__BOTS_CONFIG_HPP

