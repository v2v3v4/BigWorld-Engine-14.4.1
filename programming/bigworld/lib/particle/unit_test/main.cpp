#include "pch.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/main_loop_task.hpp"
#include "moo/animation_channel.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_vlo.hpp"

BW_USE_NAMESPACE

int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	BW::Allocator::setCrashOnLeak( true );

	const std::auto_ptr<CStdMf> cstdmfSingleton( new CStdMf );

	int result = BWUnitTest::runTest( "particle", argc, argv );

	ChunkVLO::fini();
	Chunk::fini();
	Moo::AnimationChannel::fini();
	MainLoopTasks::finiAll();

	return result;
}

// main.cpp
