#include "cstdmf/debug_filter.hpp"
#include "cstdmf/stack_tracker.hpp"
#include "cstdmf/watcher.hpp"
#include "unit_test_lib/unit_test.hpp"

BW_BEGIN_NAMESPACE
DECLARE_WATCHER_DATA( NULL )
DECLARE_COPY_STACK_INFO( true )
BW_END_NAMESPACE

int main( int argc, char* argv[] )
{
	BW_USE_NAMESPACE
	BW_SYSTEMSTAGE_MAIN();

	bool bSucessfulInit = true;

	int result = 1;
	if(bSucessfulInit == true)
	{
		result = BWUnitTest::runTest( "Asset Pipeline Dependency Module", argc, argv );
	}

	// Prevent memory leak
	DebugFilter::fini();

	return result;
}
