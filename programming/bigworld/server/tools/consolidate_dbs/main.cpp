#include "script/first_include.hpp"

#include "command_line_parser.hpp"
#include "consolidate_dbs_app.hpp"

#include "cstdmf/debug_filter.hpp"

#include "cstdmf/allocator.hpp"

#include "resmgr/bwresource.hpp"

#include "server/bwconfig.hpp"
#include "server/bwservice.hpp"


DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )


BW_USE_NAMESPACE

/**
 *	This is a simple class to help suppress some of the log spam when shutting
 *	down.
 */
class ShutdownLogSuppressor
{
public:
	ShutdownLogSuppressor() {}
	~ShutdownLogSuppressor()
	{
		DebugFilter::shouldWriteToConsole( false );
	}
};

int main( int argc, char * argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	// This will be enabled in app.init
	DebugFilter::instance().filterThreshold( MESSAGE_PRIORITY_WARNING );

	Allocator::setReportOnExit( false );

	BWResource bwresource;
	BWResource::init( argc, (const char **)argv );
	BWConfig::init( argc, argv );

	if (BWConfig::isBad())
	{
		return EXIT_SUCCESS;
	}

	CommandLineParser options;
	if (!options.init( argc, argv ))
	{
		ERROR_MSG( "Failed to parse command line options.\n" );
		return EXIT_FAILURE;
	}

	ConsolidateDBsApp app( !options.shouldIgnoreSqliteErrors() );

	bool shouldReportToDBApp = options.hadNonOptionArgs();

	// TODO: Scalable DB, now only acquire lock when do --clear action,
	// after full DBApp sharding implementation, consolidate_dbs should
	// acquire lock by default again.
	bool shouldLock = options.shouldClear();

	ShutdownLogSuppressor shutdownLogSuppressor;

	if (!app.init( options.isVerbose(), shouldReportToDBApp, shouldLock,
			options.primaryDatabaseInfo() ))
	{
		return EXIT_FAILURE;
	}

	START_MSG( "ConsolidateDBs", false );

	if (options.shouldClear())
	{
		if (app.clearSecondaryDBEntries())
		{
			return EXIT_SUCCESS;
		}
		else
		{
			ERROR_MSG( "Failed to clear secondary databases\n" );
			return EXIT_FAILURE;
		}
	}

	if (options.shouldList())
	{
		if (app.printDatabases())
		{
			return EXIT_SUCCESS;
		}
		else
		{
			ERROR_MSG( "Failed to list secondary databases\n" );
			return EXIT_FAILURE;
		}
	}

	if (!app.checkPrimaryDBEntityDefsMatch())
	{
		return EXIT_FAILURE;
	}

	if (options.hadNonOptionArgs())
	{
		// We were run with the secondary databases already transferred by the
		// snapshot_helper and are specified in a file list, the path to which
		// is specified on the command line.
		return app.consolidateSecondaryDBs( options.secondaryDatabases() ) ?
			EXIT_SUCCESS : EXIT_FAILURE;
	}
	else
	{
		// We were run with no specific primary database or secondary
		// databases specified, the consolidator must transfer those that are
		// registered in the primary database, before consolidating.
		return app.transferAndConsolidate() ? EXIT_SUCCESS : EXIT_FAILURE;
	}
}

// main.cpp
