#include "table_synchroniser.hpp"

#include "network/event_dispatcher.hpp"

#include "server/signal_set.hpp"
#include "server/signal_processor.hpp"
#include "server/util.hpp"

#include "cstdmf/debug.hpp"

#include <sys/types.h>
#include <sys/wait.h>

#include <cstdlib>
#include <errno.h>

#define SYNC_DB_FILENAME_STR 	"sync_db"
#define SYNC_DB_RELPATH_STR 	"commands/" SYNC_DB_FILENAME_STR

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

namespace // (anonymous)
{


/**
 *	Signal handler for waits for a child process to terminate, and then
 *	notifies the TableSynchroniser.
 */
class ChildWaiter : public SignalHandler
{
public:
	explicit ChildWaiter( TableSynchroniser & tableSynchroniser, pid_t pid ):
			tableSynchroniser_( tableSynchroniser ),
			pid_( pid )
	{}

	virtual ~ChildWaiter()
	{}

	virtual void handleSignal( int sigNum )
	{
		if (sigNum == SIGCHLD)
		{
			int status = 0;

			if (pid_ == ::waitpid( pid_, &status, WNOHANG ))
			{

				DEBUG_MSG( "ChildWaiter::handleSignal: "
						"child %d exited with status = %d\n",
					pid_, status ); 
				tableSynchroniser_.onProcessExited( status );
			}
		}
		else
		{
			MF_ASSERT( sigNum == SIGINT || sigNum == SIGHUP );

			DEBUG_MSG( "ChildWaiter::handleSignal: "
					"aborting\n" );

			tableSynchroniser_.abort();
		}
	}

private:
	TableSynchroniser & tableSynchroniser_;
	pid_t pid_;
};


} // end anonymous namespace


/**
 * 	Destructor.
 */
TableSynchroniser::~TableSynchroniser()
{
	this->abort();
}


/**
 *	Run the table synchroniser.
 */
bool TableSynchroniser::run( Mercury::EventDispatcher & dispatcher )
{
	if (running_)
	{
		ERROR_MSG( "TableSynchroniser::run: Already running\n" );
		return false;
	}

	const BW::string path = ServerUtil::exeDir();
	const BW::string syncdbPath = path + "/" SYNC_DB_RELPATH_STR;

	int accessStatus = ::access( syncdbPath.c_str(), X_OK );

	if (accessStatus == -1)
	{
		ERROR_MSG( "MySqlDatabase::syncTablesToDefs: Unable to access '%s'. "
				"%s.\n", 
			syncdbPath.c_str(), strerror( errno ) );
		return false;
	}

	// Check we will be able to chdir() to the directory containing the
	// executable as the forked process.
	accessStatus = ::access( path.c_str(), X_OK );
	if (accessStatus == -1)
	{
		ERROR_MSG( "MySqlDatabase::syncTablesToDefs: "
				"Unable to chdir into '%s'\n",
			path.c_str() );
		return false;
	}

	running_ = true;

	pid_t childPID = ::fork();

	if (childPID == 0)
	{
		if (chdir( path.c_str() ) == -1)
		{
			ERROR_MSG( "MySqlDatabase::syncTablesToDefs: Failed to change "
					"directory to '%s'. %s.\n",
					path.c_str(), strerror( errno ) );
			exit( 1 );
		}

		this->execSyncDB( syncdbPath );
		// This should run exec, and so never reach here.
	}
	pid_ = childPID;

	ChildWaiter childWaiter( *this, childPID );
	SignalProcessor::instance().addSignalHandler( SIGCHLD, &childWaiter );
	SignalProcessor::instance().addSignalHandler( SIGINT, &childWaiter );
	SignalProcessor::instance().addSignalHandler( SIGHUP, &childWaiter );

	Signal::Set set;
	set.set( SIGCHLD );
	set.set( SIGINT );
	set.set( SIGHUP );

	while (running_)
	{
		SignalProcessor::instance().waitForSignals( set );
	}

	return this->wasSuccessful();
}


/**
 *	Stops the child process if it is running.
 */
void TableSynchroniser::abort()
{
	if (running_ && pid_ != -1)
	{
		INFO_MSG( "TableSynchroniser::abort: stopping sync_db child\n" );
		::kill( pid_, SIGINT );
	}
}


/**
 *	Receive notification on a child process exiting.
 */
void TableSynchroniser::onProcessExited( int status )
{
	TRACE_MSG( "TableSynchroniser::onProcessExited: status = %d\n",
		status );
	status_ = status;
	running_ = false;
}


/**
 *	Return true if the sync_db process has exited.
 */
bool TableSynchroniser::didExit() const
{
	return this->isFinished() && WIFEXITED( status_ );
}


/**
 *	Return the exit code, only valid if the sync_db process has exited.
 */
int TableSynchroniser::exitCode() const
{
	return WEXITSTATUS( status_ );
}


/**
 *	Return true if the sync_db process was terminated by a signal.
 */
bool TableSynchroniser::wasSignalled() const
{
	return WIFSIGNALED( status_ );
}


/**
 *	Return the signal number, only valid if the sync_db process was terminated
 *	by a signal.
 */
int TableSynchroniser::signal() const
{
	return WTERMSIG( status_ );
}


/**
 *	Executes the sync_db process.
 */
void TableSynchroniser::execSyncDB( const BW::string & syncdbPath )
{
	char * argv[] =
	{
		const_cast< char * >( syncdbPath.c_str() ),
		const_cast< char * >( "--run-from-dbapp" ),
		NULL
	};


	if (execvp( syncdbPath.c_str(), argv ) < 0)
	{
		ERROR_MSG( "MySqlDatabase::syncTablesToDefs: Failed to execv '%s'. "
				"%s.\n", 
			syncdbPath.c_str(), 
			strerror( errno ) );
		exit( 1 );
	}
}

BW_END_NAMESPACE

// table_synchroniser.cpp
