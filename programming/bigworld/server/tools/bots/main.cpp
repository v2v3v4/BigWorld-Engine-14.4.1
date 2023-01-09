#include "script/first_include.hpp"

#include "main_app.hpp"

#include "cstdmf/debug_filter.hpp"

#include "network/logger_message_forwarder.hpp"

#include "server/bwservice.hpp"

DECLARE_DEBUG_COMPONENT2( "Bots", 0 )

#ifdef _WIN32
#include <signal.h>

void bwStop()
{
	raise( SIGINT );
}

char szServiceDependencies[] = "machined";
#endif // _WIN32

#include "server/bwservice.hpp"


BW_USE_NAMESPACE

int BIGWORLD_MAIN( int argc, char * argv[] )
{
	DebugFilter::shouldWriteToConsole( true );

	bool shouldLog = BWConfig::get( "bots/shouldLog", true );

	return bwMainT< MainApp >( argc, argv, shouldLog );
}

// main.cpp
