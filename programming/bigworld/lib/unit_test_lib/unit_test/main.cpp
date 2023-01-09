#include "unit_test_lib/unit_test.hpp"

// TODO: Temporary hack until a library is made for Windows.
#ifdef _WIN32
#include "unit_test_lib/unit_test.cpp"
#endif

BW_USE_NAMESPACE

int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	BW::Allocator::setCrashOnLeak( true );

	return BWUnitTest::runTest( "unit_test", argc, argv );
}

// main.cpp
