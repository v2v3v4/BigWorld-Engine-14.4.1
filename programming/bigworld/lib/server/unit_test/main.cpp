#include "unit_test_lib/unit_test.hpp"

BW_USE_NAMESPACE

int 		g_cmdArgC;
char ** 	g_cmdArgV;

BW_USE_NAMESPACE

int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	// saved away for the test harness
	g_cmdArgC = argc;
	g_cmdArgV = argv;

	return BWUnitTest::runTest( "server", argc, argv );
}

// main.cpp
