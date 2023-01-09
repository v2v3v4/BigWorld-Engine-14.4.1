#include "bots_config.hpp"

#include "connection/connection_transport.hpp"

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS BotsConfig
#define BW_CONFIG_PREFIX "bots/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Standard options
// -----------------------------------------------------------------------------

BW_OPTION	( BW::string, 		username, 				"Bot" )
BW_OPTION	( BW::string, 		password, 				"" )
BW_OPTION	( BW::string, 		serverName, 			"" )
BW_OPTION	( BW::string, 		tag,		 			"Default" )
BW_OPTION	( bool,				shouldUseTCP, 			false )
BW_OPTION	( bool,				shouldUseWebSockets,	false )
DERIVED_BW_OPTION( int,			transport ) // set in postInit
BW_OPTION	( uint16, 			port, 					0 )
BW_OPTION	( bool, 			shouldUseRandomName, 	true )
BW_OPTION	( bool, 			shouldUseScripts, 		true )
BW_OPTION	( BW::string, 		standinEntity, 			"DefaultEntity" )
BW_OPTION	( BW::string, 		controllerType, 		"Patrol" )
BW_OPTION	( BW::string, 		controllerData, 		"server/bots/test.bwp" )
BW_OPTION	( BW::string,		publicKey, 				"" )
BW_OPTION	( float,			logOnRetryPeriod,		0.0f )
BW_OPTION_RO( bool, 			shouldListenForGeometryMappings, false );
BW_OPTION_RO( float, 			chunkLoadingPeriod, 	0.02f );


bool BotsConfig::postInit()
{
	if (shouldUseTCP())
	{
		transport.set( shouldUseWebSockets() ? 
				CONNECTION_TRANSPORT_WEBSOCKETS :
				CONNECTION_TRANSPORT_TCP );
	}
	else
	{
		transport.set( CONNECTION_TRANSPORT_UDP );
	}

	return true;
}


BW_END_NAMESPACE


// bots_config.cpp

