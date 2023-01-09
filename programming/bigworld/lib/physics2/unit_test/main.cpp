#include "pch.hpp"

#include "cstdmf/allocator.hpp"
#include "unit_test_lib/unit_test.hpp"

BW_USE_NAMESPACE


int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	Allocator::setCrashOnLeak( true );

	return BWUnitTest::runTest( "physics2", argc, argv );
}

// main.cpp
