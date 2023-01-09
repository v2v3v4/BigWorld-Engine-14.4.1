#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "mysql_synchronise.hpp"

#include "cstdmf/arg_parser.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/debug_filter.hpp"

#include "network/event_dispatcher.hpp"
#include "network/logger_message_forwarder.hpp"
#include "network/machined_utils.hpp"

#include "resmgr/bwresource.hpp"

#include "server/bwconfig.hpp"
#include "server/bwservice.hpp"


BW_USE_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Main
// -----------------------------------------------------------------------------

int main( int argc, char * argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	ArgParser argParser( "sync_db" );
	argParser.add( "verbose", "Display verbose program output to the console" );
	argParser.add( "run-from-dbapp", "Flags if running within DBApp" );
	argParser.add( "dry-run", "Perform a trial run with no changes made" );
	argParser.add( "host", "MySql Host" );
	argParser.add( "port", "MySQL Port" );
	argParser.add( "database", "MySQL Database" );
	argParser.add( "username", "MySQL Username" );
	argParser.add( "password", "MySQL Password" );
	argParser.add( "secure-auth", "Wether or not use MySQL secure auth "
			"password format, default is 'true' if this option is present." );

	if (!argParser.parse( argc, argv ))
	{
		argParser.printUsage();
		return EXIT_FAILURE;
	}

	BWResource bwresource;
	BWResource::init( argc, const_cast< const char ** >( argv ) );
	BWConfig::init( argc, argv );

	bool isVerbose = argParser.get( "verbose", false );
	bool shouldLock = !argParser.get( "run-from-dbapp", false );
	bool isDryRun = argParser.get( "dry-run", false );

	const DBConfig::ConnectionInfo & connBWConfig = DBConfig::connectionInfo();

	DBConfig::ConnectionInfo conn;
	conn.host = argParser.get( "host", connBWConfig.host );
	conn.port = argParser.get( "port", connBWConfig.port );
	conn.database = argParser.get( "database", connBWConfig.database );
	conn.username = argParser.get( "username", connBWConfig.username );
	conn.password = argParser.get( "password", connBWConfig.password );
	conn.secureAuth = connBWConfig.secureAuth;

	if (argParser.isPresent( "secure-auth" ))
	{
		conn.secureAuth = argParser.get( "secure-auth", true );
	}

	DebugFilter::shouldWriteToConsole( isVerbose );

	START_MSG( "SyncDB" );

	MySqlSynchronise mysqlSynchronise;

	if (!mysqlSynchronise.init( isVerbose, shouldLock, isDryRun, conn ))
	{
		ERROR_MSG( "Initialisation failed\n" );
		return EXIT_FAILURE;
	}

	if (!mysqlSynchronise.run())
	{
		ERROR_MSG( "Sync to database failed\n" );
		return EXIT_FAILURE;
	}

	INFO_MSG( "Sync to database successful\n" );

	return EXIT_SUCCESS;
}

// sync_db.cpp
