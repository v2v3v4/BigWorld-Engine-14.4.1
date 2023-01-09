#include "baseappmgr.hpp"
#include "baseappmgr_config.hpp"

#include "server/bwservice.hpp"

#include "cellappmgr/cellappmgr_interface.hpp"

#define DEFINE_INTERFACE_HERE
#include "cellappmgr/cellappmgr_interface.hpp"


BW_USE_NAMESPACE


int BIGWORLD_MAIN( int argc, char * argv[] )
{
	return bwMainT< BaseAppMgr >( argc, argv );
}


// main.cpp
