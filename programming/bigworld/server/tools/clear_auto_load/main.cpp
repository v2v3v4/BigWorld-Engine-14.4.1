#include "clear_auto_load.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/debug_filter.hpp"

#include "resmgr/bwresource.hpp"

#include "server/bwconfig.hpp"
#include "server/bwservice.hpp"

#include <cstdio>

namespace // anonymous
{
const char * USAGE_MESSAGE =
	"Usage: clear_auto_load [options]\n\n"							
	"Options:\n"														
	" --verbose | -v      Display verbose program output to the console.\n"
	" --help | -h         Program usage.\n";

} // end namespace (anonymous)


BW_USE_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Main
// -----------------------------------------------------------------------------


int main( int argc, char * argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	bool isVerbose = false;

	for (int i = 1; i < argc; ++i)
	{
		if (((strcmp( argv[i], "--res" ) == 0) ||
			 (strcmp( argv[i], "-r" ) == 0)) && i < (argc - 1))
		{
			++i;
		}
		else if ((strcmp( argv[i], "--verbose" ) == 0) ||
				(strcmp( argv[i], "-v" ) == 0))
		{
			isVerbose = true;
		}
		else if ((strcmp( argv[i], "--help" ) == 0) ||
				(strcmp( argv[i], "-h" ) == 0))

		{
			puts( USAGE_MESSAGE );
			return EXIT_SUCCESS;
		}
		else
		{
			printf( "Unrecognised option: %s\n", argv[i] );
			return EXIT_FAILURE;
		}
	}

	DebugFilter::shouldWriteToConsole( isVerbose );

	BWResource bwResource;
	BWResource::init( argc, const_cast< const char ** >( argv ) );
	BWConfig::init( argc, argv );

	ClearAutoLoad clearAutoLoad;

	if (!clearAutoLoad.init( isVerbose ))
	{
		ERROR_MSG( "Failed to initialise\n" );
		return EXIT_FAILURE;
	}

	START_MSG( "ClearAutoLoad" );

	if (!clearAutoLoad.run())
	{
		ERROR_MSG( "Failed to clear auto-load data\n" );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

// main.cpp
