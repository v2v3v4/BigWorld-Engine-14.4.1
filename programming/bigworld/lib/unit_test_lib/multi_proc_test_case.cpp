#include "pch.hpp"
#include "multi_proc_test_case.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/bw_string.hpp"

#include "cstdmf/debug.hpp"


#ifdef MF_SERVER
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <memory>

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: MultiProcTestCase Implementation
// -----------------------------------------------------------------------------

/**
 *	Creates the specified number of child processes for them to run child
 *	instances created from the child instance factory. This is intended for
 *	subclasses of MultiProcTestCase to use in their test cases for spawning
 *	multiple child processes.
 */
void MultiProcTestCase::runChildren( int num,
		MultiProcTestCase::ChildProcessFactory * pFactory )
{
	int numCreated = 0;
	while (numCreated < num)
	{
		++numCreated;

		// This call is responsible for the new child process object
		if (!this->runChild( pFactory->create() ))
		{
			return; // RIP OUT FACTORY CODE
		}
	}
}


/**
 *	This method runs the input ChildProcess object in a forked process. This
 *	method is responsible for deleting this child process object.
 */
bool MultiProcTestCase::runChild( ChildProcess * pChildProcess )
{
#ifdef MF_SERVER
	int pid = fork();
	if (pid == -1)
	{
		bw_safe_delete(pChildProcess);
		this->fail( "Couldn't spawn child process" );
		return false;
	}
	if (pid == 0)
	{
		// I'm the child

		// set it to false because it leaks when forking
		BW::Allocator::setReportOnExit( false );

		int status = pChildProcess->run();

		bw_safe_delete(pChildProcess);
		exit( status );
	}
	else
	{
		// I'm the parent
		pids_.insert( pid );
	}
	bw_safe_delete(pChildProcess);
	return true;
#else
	return false;
#endif
}


/**
 *  This method terminates all currently running children.
 */
void MultiProcTestCase::killChildren()
{
#ifdef MF_SERVER
	for (ProcessIDs::iterator iter = pids_.begin(); iter != pids_.end(); ++iter)
	{
		::kill( *iter, SIGKILL );
	}

	hasKilledChildren_ = true;
#endif
}


/**
 *	This method updates the child process map.
 *
 *	@param shouldBlock If set to true, this method will block on waitpid.
 */
bool MultiProcTestCase::updateChildren( bool shouldBlock )
{
#ifdef MF_SERVER
	int status = 0;

	int options = shouldBlock ? 0 : WNOHANG;

	int pid = waitpid( -1, &status, options );

	if (pid == -1)
	{
		ERROR_MSG( "MultiProcTestCase::waitForAll: waitpid failed\n" );
		return false;
	}
	else if (pid != 0)
	{
		ProcessIDs::iterator pPid = pids_.find( pid );
		if (pPid == pids_.end())
		{
			printf( "an unexpected child died: %d\n", pid );
		}
		else
		{
			if (WIFEXITED( status ) || WIFSIGNALED( status ))
			{
				pids_.erase( pid );
				statuses_[pid] = status;
			}
		}
	}
	else
	{
		return false;
	}

	return true;
#else
	return false;
#endif
}


/**
 *	This method waits (blocks) for all pending child processes to exit. Call
 *	this after creating children via runChildren() or runChild().
 */
void MultiProcTestCase::waitForAll()
{
	// Let the main process run to completion.
	mainProcess_.run();

	// If the main process failed, terminate all children immediately.
	if (mainProcess_.hasFailed())
	{
		this->killChildren();
	}

	// Wait till all children complete.
	while (!pids_.empty())
	{
		this->updateChildren( /* shouldBlock: */ true );
	}
}


/**
 *	This method returns whether there are any running child processes.
 */
bool MultiProcTestCase::hasRunningChildren()
{
	return this->numRunningChildren() != 0;
}


/**
 *	This method returns the number of running child processes.
 */
int MultiProcTestCase::numRunningChildren()
{
	while (this->updateChildren( /* shouldBlock: */ false ))
	{
		// pass
	}

	return static_cast<int>( pids_.size() );
}


/**
 *	This method returns true if all children have passed. Call this only after
 *	calling runChildren() or runChild(). If some children do not pass, then it
 *	raises an CPPUnit assertion.
 */
bool MultiProcTestCase::checkAllChildrenPass()
{
#ifdef MF_SERVER
	// If children were intentionally killed, just return immediately.
	if (hasKilledChildren_)
	{
		return true;
	}

	Statuses::const_iterator pPid = statuses_.begin();
	bool allPassed = true;

	char detailMsg[256];
	detailMsg[255] = '\0';

	// raise an assertion with this message if one of our child processes fails
	BW::string failureMsg =
		"Some child processes did not successfully exit:\n";

	while (pPid != statuses_.end())
	{
		int pid = pPid->first;
		int status = pPid->second;

		if (WIFSIGNALED( status ))
		{
			int termSignal = WTERMSIG( status );
			bw_snprintf( detailMsg, sizeof( detailMsg ),
					"child %d got signal %d (%s)\n",
				pid, termSignal, strsignal( termSignal ) );

			failureMsg += detailMsg;

			allPassed = false;
		}
		else if (WIFEXITED( status ))
		{
			if (WEXITSTATUS( status ) != 0)
			{
				bw_snprintf( detailMsg, sizeof( detailMsg ),
						"child %d exited with status %d\n",
					pid, WEXITSTATUS( status ) );

				failureMsg += detailMsg;
				allPassed = false;
			}
		}
		++pPid;
	}

	statuses_.clear();

	if (!allPassed)
	{
		this->fail( failureMsg );
	}

	return allPassed;
#else
	// Force failure on Win32 until this gets implemented.
	this->fail( "Not implemented in Win32" );
	return false;
#endif
}

BW_END_NAMESPACE

// multiproc_test_case.cpp
