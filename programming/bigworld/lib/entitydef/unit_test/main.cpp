#include "pch.hpp"
#include "resmgr/data_section_census.hpp"
#include "test_harness.hpp"

#include "entitydef/data_type.hpp"
#include "entitydef/meta_data_type.hpp"
#include "chunk/chunk.hpp"
#include "cstdmf/debug_filter.hpp"

#if !defined( MF_SERVER )
#include "chunk/chunk_vlo.hpp"
#include "cstdmf/main_loop_task.hpp"
#include "moo/init.hpp"
#include "moo/animation_channel.hpp"
#include "input/input.hpp"
#endif // !defined( MF_SERVER )

BW_BEGIN_NAMESPACE

extern int Math_token;
extern int ResMgr_token;
extern int PyScript_token;
extern int ChunkUserDataObject_token;
extern int PyUserDataObject_token;
extern int UserDataObjectDescriptionMap_Token;

int s_tokens =
	Math_token |
	ResMgr_token |
	PyScript_token |
	ChunkUserDataObject_token |
	PyUserDataObject_token |
	UserDataObjectDescriptionMap_Token;

BW_END_NAMESPACE


BW_USE_NAMESPACE

int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

#if !defined( MF_SERVER )
	// Because chunk pulls in romp, which pulls in input
	InputDevices inputDevices;
#endif // !defined( MF_SERVER )

	Allocator::setCrashOnLeak( true );

	// saved away for the test harness
	EntityDefUnitTestHarness::s_cmdArgC = argc;
	EntityDefUnitTestHarness::s_cmdArgV = argv;

	const int result = BWUnitTest::runTest( "entitydef", argc, argv );

	// Temporarily disabled, until UserDataObject::createRefType() isn't
	// seen as a leak anymore. (Or doesn't leak anymore...)
	Allocator::setCrashOnLeak( false );
	// Prevent memory leak
#if !defined( MF_SERVER )
	MainLoopTasks::finiAll();
	ChunkVLO::fini();
	Moo::AnimationChannel::fini();
	Moo::fini();
#endif // !defined( MF_SERVER )
	Chunk::fini();
	Script::fini();
	DataType::clearStaticsForReload();
	MetaDataType::fini();
	DebugFilter::fini();

	return result;
}

// main.cpp
