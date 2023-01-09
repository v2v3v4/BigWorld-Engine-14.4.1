#include "external_app_config.hpp"

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS ExternalAppConfig
#define BW_CONFIG_PREFIX ""
#include "server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ExternalAppConfig
// -----------------------------------------------------------------------------

BW_OPTION_RO( float, externalLatencyMin, 0.f );
BW_OPTION_RO( float, externalLatencyMax, 0.f );
BW_OPTION_RO( float, externalLossRatio, 0.f );

BW_OPTION_RO( BW::string, externalInterface, "" );

BW_OPTION_RO( uint, tcpServerBacklog, 511 );


bool ExternalAppConfig::postInit()
{
	return true;
}

BW_END_NAMESPACE

// external_app_config.cpp
