#include "unit_test_lib/unit_test.hpp"
#include "cstdmf/debug_filter.hpp"
#include "server/server_app_option.hpp"

#include "network/endpoint.hpp"

BW_USE_NAMESPACE

int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();
	
	initNetwork();

	Allocator::setReportOnExit( true );
	Allocator::setCrashOnLeak( true );

	int result = BWUnitTest::runTest( "baseapp", argc, argv );

	finiNetwork();

	// the config is not being read so the options have to be cleared manually
	ServerAppOptionIniter::deleteAll();

	DebugFilter::fini(); // prevent singleton leak.

	return result;
}

// main.cpp
