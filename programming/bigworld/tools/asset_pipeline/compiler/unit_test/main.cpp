#include "cstdmf/debug_filter.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/data_section_census.hpp"
#include "unit_test_lib/unit_test.hpp"

BW_BEGIN_NAMESPACE
DECLARE_WATCHER_DATA( NULL )
DECLARE_COPY_STACK_INFO( false )
BW_END_NAMESPACE

int main( int argc, char* argv[] )
{
	BW_USE_NAMESPACE
	BW_SYSTEMSTAGE_MAIN();

	// Initialise the file systems
	//For the unit tests, we force a particular resource path.
	const char * UNIT_TEST_RESOURCE_PATH = "../..";
	const char * myargv[] =
	{
		argv[0],
		//Test-specific resources.
		"--res", UNIT_TEST_RESOURCE_PATH
	};
	int myargc = ARRAY_SIZE( myargv );

	bool bInitRes = BWResource::init( myargc, myargv );

	// Initialise the background task manager
	BgTaskManager::init();

	int result = 1;
	if(bInitRes == true)
	{
		result = BWUnitTest::runTest( "Asset Pipeline Compiler Module", argc, argv );
	}

	// Fini the background task manager
	BgTaskManager::fini();

	// Prevent memory leak
	DebugFilter::fini();

	// Cleanup the file systems
	BWResource::fini();

	// DataSectionCensus is created on first use, so delete at end of App
	DataSectionCensus::fini();

	return result;
}
