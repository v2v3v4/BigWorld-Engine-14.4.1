#include "cellapp.hpp"
#include "cellapp_config.hpp"

#include "server/bwservice.hpp"

BW_USE_NAMESPACE

int BIGWORLD_MAIN( int argc, char * argv[] )
{
	return bwMainT< CellApp >( argc, argv );
}

// main.cpp
