#include "pch.hpp"

#include "resmgr/data_section_census.hpp"
#include "test_harness.hpp"


BW_USE_NAMESPACE

int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	Allocator::setCrashOnLeak( true );

	// saved away for the test harness
	ResMgrUnitTestHarness::s_cmdArgC = argc;
	ResMgrUnitTestHarness::s_cmdArgV = argv;

	const int result = BWUnitTest::runTest( "resmgr", argc, argv );

	return result;
}

// main.cpp
