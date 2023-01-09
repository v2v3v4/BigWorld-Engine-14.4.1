#ifndef BA_LOGIN_CONDITIONS_CONFIG_HPP
#define BA_LOGIN_CONDITIONS_CONFIG_HPP

#include "server/server_app_option.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class stores the login conditions options for BaseAppMgr.
 */
class LoginConditionsConfig
{
public:
	static ServerAppOption< float > minLoad;
	static ServerAppOption< float > minOverloadTolerancePeriod;
	static ServerAppOption< uint64 > minOverloadTolerancePeriodInStamps;
	static ServerAppOption< int > overloadLogins;

	static bool postInit();
};

BW_END_NAMESPACE

#endif // BA_LOGIN_CONDITIONS_CONFIG_HPP
// login_conditions_config.hpp
