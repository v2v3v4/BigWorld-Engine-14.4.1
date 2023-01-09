#ifndef DB_LOGIN_CONDITIONS_CONFIG_HPP
#define DB_LOGIN_CONDITIONS_CONFIG_HPP

#include "server/server_app_option.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class stores the login conditions options for BaseAppMgr.
 */
class LoginConditionsConfig
{
public:
	static ServerAppOption< float > maxLoad;
	static ServerAppOption< float > overloadTolerancePeriod;

	static uint64 overloadTolerancePeriodInStamps();

	static bool postInit();
};

BW_END_NAMESPACE

#endif // DB_LOGIN_CONDITIONS_CONFIG_HPP
// login_conditions_config.hpp
