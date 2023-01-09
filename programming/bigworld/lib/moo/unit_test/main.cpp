#include "pch.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/allocator.hpp"
#include "cstdmf/main_loop_task.hpp"
#include "moo/init.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_vlo.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/data_section_census.hpp"
#include "unit_test_lib/unit_test.hpp"
#include <memory>


BW_USE_NAMESPACE

int main( int argc, char * argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	BW::Allocator::setCrashOnLeak( true );
	
	const std::auto_ptr<CStdMf> cstdmfSingleton( new CStdMf );

	//For the unit tests, we force a particular resource path.
	const char * myargv[] =
		{
			argv[0], "--res", UNIT_TEST_RESOURCE_PATH
		};
	int myargc = ARRAY_SIZE( myargv );

	if ( !BWResource::init( myargc, myargv ) )
	{
		ERROR_MSG( "Unable to initialise BWResource.\n" );
		return 1;
	}

	if (!BgTaskManager::init())
	{
		ERROR_MSG( "Unable to initialise BgTaskManager.\n" );
		BWResource::fini();
		return 1;
	}
	if (!FileIOTaskManager::init())
	{
		ERROR_MSG( "Unable to initialise BgTaskManager.\n" );
		BgTaskManager::fini();
		BWResource::fini();
		return 1;
	}

	int result = 0;
	if (!Moo::init())
	{
		ERROR_MSG( "Unable to initialise Moo.\n" );
		FileIOTaskManager::fini();
		BgTaskManager::fini();
		BWResource::fini();
		return 1;
	}

	Chunk::init();

	result = BWUnitTest::runTest( "moo", argc, argv );

	Chunk::fini();
	ChunkVLO::fini();
	Moo::fini();

	MainLoopTasks::finiAll();
	DebugFilter::fini();

	FileIOTaskManager::fini();
	BgTaskManager::fini();
	BWResource::fini();

	return result;
}
