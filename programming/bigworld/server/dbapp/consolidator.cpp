#include "script/first_include.hpp"

#include "consolidator.hpp"

#include "dbapp.hpp"

#include "db_storage/idatabase.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/debug_message_categories.hpp"

#include "network/event_dispatcher.hpp"

#include "resmgr/bwresource.hpp"

#include "server/child_process.hpp"
#include "server/util.hpp"

#include <sys/types.h>
#include <sys/wait.h>

#include <string.h>
#include <errno.h>

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Constants
// -----------------------------------------------------------------------------
namespace
{

#define CONSOLIDATE_DBS_FILENAME_STR 	"consolidate_dbs"
#define CONSOLIDATE_DBS_RELPATH_STR 	"commands/" CONSOLIDATE_DBS_FILENAME_STR 
const int CONSOLIDATE_DBS_EXEC_FAILED_EXIT_CODE = 100;

} // end anonymous namespace


// -----------------------------------------------------------------------------
// Section: Consolidator
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Consolidator::Consolidator( DBApp & dbApp ) :
	dbApp_( dbApp ),
	pChildProcess_( NULL )
{
}


/**
 *	Destructor
 */
Consolidator::~Consolidator()
{
	if (pChildProcess_)
	{
		delete pChildProcess_;
	}
}


/*
 *	Override from IChildProcess.
 */
void Consolidator::onChildComplete( int status, ChildProcess * process )
{
	bool consolidationSucceeded = false;

	consolidationSucceeded = (WIFEXITED( status ) &&
								(WEXITSTATUS( status ) == EXIT_SUCCESS));

	if (!consolidationSucceeded && WIFEXITED( status ))
	{
		int exitCode = WEXITSTATUS( status );

		if (exitCode == CONSOLIDATE_DBS_EXEC_FAILED_EXIT_CODE)
		{
			BW::string fullPath =
				ServerUtil::exeDir() + "/" CONSOLIDATE_DBS_RELPATH_STR;

			SECONDARYDB_ERROR_MSG( "Consolidator::onChildComplete: "
				"Failed to execute '%s'.\n"
				"Please ensure that the " CONSOLIDATE_DBS_FILENAME_STR " "
				"executable exists and is runnable. You may need to build it "
				"manually as it not part of the standard package.\n",
				fullPath.c_str() );
		}
		else
		{
			SECONDARYDB_ERROR_MSG( "Consolidator::onChildComplete: "
					"Consolidate process exited with code %d\n"
					"Please examine the logs for ConsolidateDBs or run "
					CONSOLIDATE_DBS_FILENAME_STR " manually to determine "
					"the cause of the error\n",
					exitCode );
		}
	}

	if (consolidationSucceeded)
	{
		SECONDARYDB_TRACE_MSG( "Finished data consolidation\n" );
	}
	else
	{
		this->outputErrorLogs();
	}

	int attempt = 0;
	const int MAX_ATTEMPTS = 20;

	// Re-acquire lock to DB
	while (!dbApp_.getIDatabase().lockDB() &&
			attempt < MAX_ATTEMPTS)
	{
		SECONDARYDB_WARNING_MSG( "Consolidator::onChildComplete: "
				"Failed to re-lock database. Retrying (%d/%d).\n",
			   ++attempt, MAX_ATTEMPTS );
		sleep( 1 );
	}

	// Notify DBApp that consolidation has finished.
	dbApp_.onConsolidateProcessEnd( consolidationSucceeded );
}


/*
 *	Override from IChildProcess.
 */
void Consolidator::onChildAboutToExec()
{
}


/*
 *	Override from IChildProcess.
 */
void Consolidator::onExecFailure( int result )
{
	SECONDARYDB_ERROR_MSG( "Consolidator::onExecFailure: "
			"Failed to execv '%s': %s\n",
		CONSOLIDATE_DBS_RELPATH_STR, strerror( errno ) );

	exit( CONSOLIDATE_DBS_EXEC_FAILED_EXIT_CODE );
}


/**
 *	Return the database's event dispatcher.
 */
Mercury::EventDispatcher & Consolidator::dispatcher()
{
	return dbApp_.mainDispatcher();
}



/**
 *	This method is the entry point for the DBApp to start consolidation.
 */
bool Consolidator::startConsolidation()
{
	MF_ASSERT( pChildProcess_ == NULL );

	pChildProcess_ = new ChildProcess( this->dispatcher(), this,
							CONSOLIDATE_DBS_RELPATH_STR );


	// NOTE: BWResource::getPathAsCommandLine() has some weird code which
	// made it unsuitable for us.
	int numPaths = BWResource::getPathNum();
	if (numPaths > 0)
	{
		pChildProcess_->addArg( "--res" );

		BW::stringstream combinedResPath;

		combinedResPath << BWResource::getPath( 0 );
		for (int i = 1; i < numPaths; ++i)
		{
			combinedResPath << BW_RES_PATH_SEPARATOR <<
				BWResource::getPath( i );
		}

		pChildProcess_->addArg( combinedResPath.str() );
	}


	// Release the DBApp lock on the primary DB so that the consolidate_db
	// process can access it.
	dbApp_.getIDatabase().unlockDB();

	return pChildProcess_->startProcessWithPipe( /*shouldPipeStdOut:*/ false,
		/*shouldPipeStdErr:*/ true );
}


/**
 *	This method outputs any error logs from the child process.
 */
void Consolidator::outputErrorLogs() const
{
	MF_ASSERT( pChildProcess_ );

	BW::string consolidateDbErrors = pChildProcess_->stdErr();

	if (!consolidateDbErrors.empty())
	{
		BW::string::size_type pos = consolidateDbErrors.find( "ERROR" );

		if (pos == BW::string::npos)
		{
			pos = consolidateDbErrors.find( "WARNING" );

			if (pos == BW::string::npos)
			{
				pos = 0;
			}
		}

		SECONDARYDB_ERROR_MSG( "Consolidator::outputErrorLogs: "
			"Log output from consolidate_dbs process:\n%s\n",
				consolidateDbErrors.c_str() + pos );
	}
}

BW_END_NAMESPACE

// consolidator.cpp
