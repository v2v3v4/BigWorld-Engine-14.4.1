#include "script/first_include.hpp"

#include "baseapp.hpp"
#include "baseapp_config.hpp"

#include "service_app.hpp"

#include "server/bwservice.hpp"

BW_USE_NAMESPACE


BW_BEGIN_NAMESPACE

namespace
{

bool isServiceApp( int argc, char * argv[] )
{
	bool isServiceApp = false;

	if (argc >= 0)
	{
		char * basec = strdup( argv[0] );
		char * bname = basename( basec );

		isServiceApp = (strcmp( bname, "serviceapp" ) == 0);

		free( basec );
	}

	return isServiceApp;
}

} // anonymous namespace

BW_END_NAMESPACE


int BIGWORLD_MAIN( int argc, char * argv[] )
{
	if (isServiceApp( argc, argv ))
	{
		return bwMainT< ServiceApp >( argc, argv );
	}
	else
	{
		return bwMainT< BaseApp >( argc, argv );
	}
}


// main.cpp
