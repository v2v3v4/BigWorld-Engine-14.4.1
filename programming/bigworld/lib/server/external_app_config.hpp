#ifndef EXTERNAL_APP_CONFIG_HPP
#define EXTERNAL_APP_CONFIG_HPP

#include "server_app_option.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class contains the configuration options for an EntityApp.
 */
class ExternalAppConfig
{
public:
	static ServerAppOption< float > externalLatencyMin;
	static ServerAppOption< float > externalLatencyMax;
	static ServerAppOption< float > externalLossRatio;

	static ServerAppOption< BW::string > externalInterface;

	static ServerAppOption< uint > tcpServerBacklog;

protected:
	static bool postInit();
};

BW_END_NAMESPACE

#endif // EXTERNAL_APP_CONFIG_HPP
