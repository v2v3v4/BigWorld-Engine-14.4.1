#include "pch.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/debug_filter.hpp"
#include "cstdmf/main_loop_task.hpp"
#include "cstdmf/dogwatch.hpp"
#include "cstdmf/watcher.hpp"
#include "resmgr/data_section_census.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_vlo.hpp"
#include "moo/animation_channel.hpp"

#include "test_harness.hpp"

BW_USE_NAMESPACE

int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	BW::Allocator::setCrashOnLeak( true );

	// saved away for the test harness
	CompiledSpaceUnitTestHarness::s_cmdArgC = argc;
	CompiledSpaceUnitTestHarness::s_cmdArgV = argv;

	const int result = BWUnitTest::runTest( "compiled_space", argc, argv );

	// Prevent memory leaks
	DebugFilter::fini();
	Moo::AnimationChannel::fini();
	MainLoopTasks::finiAll();
	ChunkVLO::fini();
	Chunk::fini();
	DogWatchManager::fini();

#if ENABLE_WATCHERS
	Watcher::fini();
#endif

	return result;
}

// main.cpp
