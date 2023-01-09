#include "id_config.hpp"

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS IDConfig
#define BW_CONFIG_PREFIX "baseApp/"
#define BW_COMMON_PREFIX "ids/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: IDConfig
// -----------------------------------------------------------------------------

BW_OPTION_RO( int, criticallyLowSize, 100 );
BW_OPTION_RO( int, lowSize, 2000 );
BW_OPTION_RO( int, desiredSize, 2400 );
BW_OPTION_RO( int, highSize, 3000 );

BW_END_NAMESPACE

// id_config.cpp
