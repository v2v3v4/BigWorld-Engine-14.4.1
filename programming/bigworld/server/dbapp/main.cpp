#include "script/first_include.hpp"

#include "dbapp.hpp"
#include "dbapp_config.hpp"
#include "server/bwservice.hpp"

BW_USE_NAMESPACE

int BIGWORLD_MAIN( int argc, char * argv[] )
{
	return bwMainT< DBApp >( argc, argv );
}

// main.cpp

