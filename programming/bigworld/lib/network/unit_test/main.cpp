#include "pch.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/timestamp.hpp"

#include "network/endpoint.hpp"

BW_USE_NAMESPACE


int main( int argc, char* argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	new CStdMf;

	initNetwork();

	BW::Allocator::setReportOnExit( false );

	// Initialise stampsPerSecond and timing method
	stampsPerSecond();

	// Turn off output for network testing to avoid the spam generated.
	// DebugFilter::instance().filterThreshold( MESSAGE_PRIORITY_ERROR );
	DebugFilter::instance().shouldWriteTimePrefix( true );

	int ret = BWUnitTest::runTest( "network", argc, argv );

	BW::Allocator::setReportOnExit( true );
	BW::Allocator::setCrashOnLeak( true );

	finiNetwork();

	delete CStdMf::pInstance();

	return ret;
}

// main.cpp
