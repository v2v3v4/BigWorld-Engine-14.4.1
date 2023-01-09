#include "loginapp.hpp"
#include "loginapp_config.hpp"
#include "server/bwservice.hpp" 

BW_USE_NAMESPACE

int BIGWORLD_MAIN( int argc, char * argv[] )
{
	return bwMainT< LoginApp >( argc, argv );
}

// main.cpp
