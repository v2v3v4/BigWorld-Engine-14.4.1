#include "pch.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/debug_filter.hpp"

BW_USE_NAMESPACE

int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	BW::Allocator::setCrashOnLeak( true );

	const int result = BWUnitTest::runTest( "math", argc, argv );

	// Prevent memory leak
	DebugFilter::fini();

	return result;
}

// main.cpp
