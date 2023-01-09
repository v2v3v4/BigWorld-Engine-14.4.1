#include "logger.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/debug_filter.hpp"
#include "cstdmf/log_msg.hpp"

#include "server/bwservice.hpp"

#include <signal.h>

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

// ----------------------------------------------------------------------------
// Section: Signal handlers
// ----------------------------------------------------------------------------

namespace
{
Logger * gLogger;
bool g_finished = false;

class SetWriteToConsoleStaticInitialiser
{
public:
	SetWriteToConsoleStaticInitialiser( bool value )
		{ DebugFilter::shouldWriteToConsole( value ); }
};

SetWriteToConsoleStaticInitialiser s_setWriteToConsoleStaticInitialiser( false );

}


void sigint( int /* sig */ )
{
	g_finished = true;
}


void sighup( int /* sig */ )
{
	if (gLogger != NULL)
	{
		gLogger->shouldRoll( true );
	}
}


void sigusr1( int /* sig */ )
{
	DEBUG_MSG( "Received SIGUSR1.\n" );
	if (gLogger != NULL)
	{
		gLogger->shouldValidateHostnames( true );
	}
	else
	{
		ERROR_MSG( "No logger found, unable to validate hostnames.\n" );
	}
}

BW_END_NAMESPACE


BW_USE_NAMESPACE

// ----------------------------------------------------------------------------
// Section: Main
// ----------------------------------------------------------------------------

int BIGWORLD_MAIN( int argc, char * argv[] )
{
	Logger logger;
	gLogger = &logger;

	signal( SIGHUP, sighup );
	signal( SIGINT, sigint );
	signal( SIGTERM, sigint );
	signal( SIGUSR1, sigusr1 );

	// Enable error messages to go to syslog
	LogMsg::shouldWriteToSyslog( true );
	INFO_MSG( "---- Logger is running ----\n" );

	if (!logger.init( argc, argv ))
	{
		return 1;
	}

	while (!g_finished)
	{
		logger.handleNextMessage();
	}

	return 0;
}

// main.cpp
