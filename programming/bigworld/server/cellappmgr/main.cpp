#include "cellappmgr.hpp"
#include "cellappmgr_config.hpp"
#include "server/bwservice.hpp"

BW_USE_NAMESPACE

int BIGWORLD_MAIN( int argc, char * argv[] )
{
	return bwMainT< CellAppMgr >( argc, argv );
}

// main.cpp
