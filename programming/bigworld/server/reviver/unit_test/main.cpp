#include "unit_test_lib/unit_test.hpp"

BW_USE_NAMESPACE

int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	Allocator::setReportOnExit( true );
	Allocator::setCrashOnLeak( true );

	return BWUnitTest::runTest( "reviver", argc, argv );
}

// main.cpp
