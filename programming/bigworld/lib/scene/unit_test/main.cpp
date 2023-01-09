#include "pch.hpp"

#include <memory>
#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/allocator.hpp"
#include "cstdmf/debug_filter.hpp"

#if !defined(MF_SERVER)
#include "cstdmf/main_loop_task.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_vlo.hpp"
#include "moo/animation_channel.hpp"
#endif

BW_USE_NAMESPACE

int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	BW::Allocator::setCrashOnLeak( true );

	const std::auto_ptr<CStdMf> cstdmfSingleton( new CStdMf );

	const int result = BWUnitTest::runTest( "scene", argc, argv );

#if !defined(MF_SERVER)
	// Prevent memory leaks
	Chunk::fini();
	ChunkVLO::fini();
	Moo::AnimationChannel::fini();
	MainLoopTasks::finiAll();
#endif
	DebugFilter::fini();

	return result;
}

// main.cpp
