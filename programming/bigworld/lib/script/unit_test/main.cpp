#include "pch.hpp"

#include "resmgr/data_section_census.hpp"
#include "test_harness.hpp"

namespace BW
{
	extern int PyLogging_token;
	extern int ResMgr_token;
	static int s_pyscriptUnitTestTokens = PyLogging_token | ResMgr_token;
}

using namespace BW;

int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	Allocator::setCrashOnLeak( true );

	// saved away for the test harness
	ScriptUnitTestHarness::s_cmdArgC = argc;
	ScriptUnitTestHarness::s_cmdArgV = argv;

	const int result = BWUnitTest::runTest( "script", argc, argv );

	// Prevent memory leak
	Script::fini();

	return result;
}

// main.cpp
