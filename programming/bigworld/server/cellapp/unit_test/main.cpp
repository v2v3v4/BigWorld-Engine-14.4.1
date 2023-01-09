#include "pch.hpp"

#include "unit_test_lib/unit_test.hpp"
#include "cstdmf/debug_filter.hpp"

BW_USE_NAMESPACE

int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	Allocator::setCrashOnLeak( true );

	int ret = BWUnitTest::runTest( "cellapp", argc, argv );

	DebugFilter::fini(); // prevent singleton leak.

	return ret;
}

// main.cpp
