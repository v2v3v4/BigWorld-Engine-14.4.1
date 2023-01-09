#include "noise_config.hpp"

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS NoiseConfig
#define BW_CONFIG_PREFIX "cellApp/"
#define BW_COMMON_PREFIX "noise/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: NoiseConfig
// -----------------------------------------------------------------------------

BW_OPTION( float, standardRange, 10.f );
BW_OPTION( float, verticalSpeed, 1000000.f );
BW_OPTION( float, horizontalSpeed, 1000000.f );

BW_OPTION_FULL( float, horizontalSpeedSqr, 0.f,
					"", "config/noise/horizontalSpeedSqr" );


// -----------------------------------------------------------------------------
// Section: Custom initialisation
// -----------------------------------------------------------------------------

/**
 *	This method is called after the configuration options have been read but
 *	before they have been printed.
 */
bool NoiseConfig::postInit()
{
	horizontalSpeedSqr.set( horizontalSpeed() * horizontalSpeed() );

	return true;
}

BW_END_NAMESPACE

// noise_config.cpp
