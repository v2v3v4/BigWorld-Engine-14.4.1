#ifndef THROTTLE_CONFIG_HPP
#define THROTTLE_CONFIG_HPP

#include "server/server_app_option.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This contains configuration options for noise.
 */
class ThrottleConfig
{
public:
	static ServerAppOption< float > behindThreshold;
	static ServerAppOption< float > spareTimeThreshold;
	static ServerAppOption< float > scaleForwardTime;
	static ServerAppOption< float > scaleBackTime;
	static ServerAppOption< float > min;

private:
	ThrottleConfig();
};

BW_END_NAMESPACE

#endif // THROTTLE_CONFIG_HPP
