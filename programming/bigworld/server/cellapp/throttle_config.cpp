#include "throttle_config.hpp"

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS ThrottleConfig
#define BW_CONFIG_PREFIX "cellApp/"
#define BW_COMMON_PREFIX "throttle/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: NoiseConfig
// -----------------------------------------------------------------------------

// These are deliberately not included in bw.xml's documentation.
BW_OPTION( float, behindThreshold, 0.1f );
BW_OPTION( float, spareTimeThreshold, 0.001f );
BW_OPTION( float, scaleForwardTime, 1.f );
BW_OPTION( float, scaleBackTime, 1.f );
BW_OPTION( float, min, 0.1f );

BW_END_NAMESPACE

// throttle_config.cpp
