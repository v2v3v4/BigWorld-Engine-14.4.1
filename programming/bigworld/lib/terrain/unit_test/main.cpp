#include "pch.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/main_loop_task.hpp"
//#include "resmgr/auto_config.hpp"

#ifndef MF_SERVER
#include "moo/init.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_vlo.hpp"
#endif

BW_USE_NAMESPACE


int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	Allocator::setCrashOnLeak( true );

	new CStdMf;
	BgTaskManager::init();
	FileIOTaskManager::init();
#ifndef MF_SERVER
	Moo::init();
	Chunk::init();
#endif

	int ret = BWUnitTest::runTest( "terrain", argc, argv );

#ifndef MF_SERVER
	Chunk::fini();
	ChunkVLO::fini();
	Moo::fini();
	MainLoopTasks::finiAll();
#endif
	BgTaskManager::fini();
	FileIOTaskManager::fini();
	delete CStdMf::pInstance();

	return ret;
}


// main.cpp
