#include "dbappmgr.hpp"
#include "dbappmgr_config.hpp"
#include "server/bwservice.hpp" 

BW_USE_NAMESPACE

int BIGWORLD_MAIN( int argc, char * argv[] )
{
	return bwMainT< DBAppMgr >( argc, argv );
}

// main.cpp
