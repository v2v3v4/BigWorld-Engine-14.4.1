#ifndef CA_LOGIN_CONDITIONS_CONFIG_HPP
#define CA_LOGIN_CONDITIONS_CONFIG_HPP

#include "server/server_app_option.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class stores the login conditions options for CellAppMgr.
 */
class LoginConditionsConfig
{
public:
	static ServerAppOption< float > avgLoad;
	static ServerAppOption< float > avgOverloadTolerancePeriod;
	static ServerAppOption< uint64 > avgOverloadTolerancePeriodInStamps;
	static ServerAppOption< float > maxLoad;
	static ServerAppOption< float > maxOverloadTolerancePeriod;
	static ServerAppOption< uint64 > maxOverloadTolerancePeriodInStamps;

	static bool postInit();
};

#endif // CA_LOGIN_CONDITIONS_CONFIG_HPP

BW_END_NAMESPACE

// login_conditions_config.hpp
