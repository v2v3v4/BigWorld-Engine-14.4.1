#include "pch.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/debug_filter.hpp"

#include "cstdmf/bgtask_manager.hpp"

BW_USE_NAMESPACE


int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	BgTaskManager::init();

	BW::Allocator::setCrashOnLeak( true );

	int ret = BWUnitTest::runTest( "cstdmf", argc, argv );

	BgTaskManager::fini();
	DebugFilter::fini(); // prevent singleton leak.

	return ret;
}

// main.cpp
