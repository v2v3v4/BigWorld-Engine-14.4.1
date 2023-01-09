#ifndef NOISE_CONFIG_HPP
#define NOISE_CONFIG_HPP

#include "server/server_app_option.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This contains configuration options for noise.
 */
class NoiseConfig
{
public:
	static ServerAppOption< float > standardRange;
	static ServerAppOption< float > verticalSpeed;
	static ServerAppOption< float > horizontalSpeed;
	static ServerAppOption< float > horizontalSpeedSqr;

	static bool postInit();

private:
	NoiseConfig();
};

BW_END_NAMESPACE

#endif // NOISE_CONFIG_HPP
