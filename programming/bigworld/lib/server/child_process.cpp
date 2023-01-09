#include "child_process.hpp"

#include "cstdmf/debug.hpp"

#include "network/event_dispatcher.hpp"

#include "server/util.hpp"

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

namespace
{

const int BW_NO_FD = -1;

}

/*
 * Documentation in the header file.
 */
void IChildProcess::onExecFailure( int result )
{
	ERROR_MSG( "ChildProcess::onExecFailure: Unable to execv: %s\n",
		strerror( errno ) );
	exit( EXIT_FAILURE );
}



/**
 *	Constructor.
 */
ChildProcess::ChildProcess( Mercury::EventDispatcher & eventDispatcher,
	IChildProcess * callback, BW::string commandToRun ) :
	eventDispatcher_( eventDispatcher ),
	pChildCompletedCB_( callback ),
	commandToRun_( commandToRun ),
	pid_( 0 ),
	stdOutFD_( BW_NO_FD ),
	pStdOut_( NULL ),
	stdErrFD_( BW_NO_FD ),
	pStdErr_( NULL ),
	isChildProcessRunning_( false )
{
}


/**
 *	Destructor
 */
ChildProcess::~ChildProcess()
{
	if (pid_ != 0)
	{
		INFO_MSG( "ChildProcess::~ChildProcess: "
				"Stopping ongoing child process %d\n", pid_ );
		::kill( pid_, SIGINT );
	}

	this->closePipes();
}


/**
 *	This method adds an argument to the argument list that will be provided to
 *	exec() when the child process is executed.
 */
void ChildProcess::addArg( const BW::string & newArg )
{
	args_.push_back( newArg );
}


/**
 *	This method returns the command line as a string, containing all arguments.
 */
BW::string ChildProcess::commandLine() const
{
	BW::stringstream cmdLine;

	cmdLine << commandToRun_;

	ArgList::const_iterator iter = args_.begin();
	while (iter != args_.end())
	{
		cmdLine << " " << (*iter);
		++iter;
	}

	return cmdLine.str();
}


/**
 *	This method closes the pipes to the child process.
 */
void ChildProcess::closePipes()
{
	if (pStdOut_)
	{
		eventDispatcher_.deregisterFileDescriptor( stdOutFD_ );
		fclose( pStdOut_ );
		stdOutFD_ = BW_NO_FD;
		pStdOut_ = NULL;
	}

	if (pStdErr_)
	{
		eventDispatcher_.deregisterFileDescriptor( stdErrFD_ );
		fclose( pStdErr_ );
		stdErrFD_ = BW_NO_FD;
		pStdErr_ = NULL;
	}
}


/**
 *	This method is responsible for performing the fork() and exec() for the
 *	child process. It is also responsible for hooking up any open pipes so that
 *	the child process writes to the correct location.
 */
bool ChildProcess::startProcessWithFileDescriptors( int stdOutFDs[ 2 ],
	int stdErrFDs[ 2 ] )
{
	// TODO: This method never returns false, however there are failure cases
	//       to be considered here.

	isChildProcessRunning_ = true;

	SignalProcessor::instance().addSignalHandler( SIGCHLD, this );

	if ((pid_ = fork()) == 0)
	{
		// In the child process, close off the read end of the FDs as we will
		// only be writing to them.
		if (stdOutFDs[ 0 ] != BW_NO_FD)
		{
			close( stdOutFDs[ 0 ] );
		}
		if (stdErrFDs[ 0 ] != BW_NO_FD)
		{
			close( stdErrFDs[ 0 ] );
		}

		// Now duplicate the write ends of the pipe to the well known file
		// descriptors for stdout and stderr so that the child process will be
		// writing to our end of the pipe.
		if (stdOutFDs[ 1 ] != BW_NO_FD)
		{
			if (dup2( stdOutFDs[ 1 ], STDOUT_FILENO ) == -1)
			{
				ERROR_MSG( "ChildProcess::startProcess: stdout dup2() failed: "
					"%s\n", strerror( errno ) );
			}
			close( stdOutFDs[ 1 ] );
		}

		if (stdErrFDs[ 1 ] != BW_NO_FD)
		{
			if (dup2( stdErrFDs[ 1 ], STDERR_FILENO ) == -1)
			{
				ERROR_MSG( "ChildProcess::startProcess: stderr dup2() failed: "
					"%s\n", strerror( errno ) );
			}
			close( stdErrFDs[ 1 ] );
		}

		this->execInFork();

		// NB: We should never reach this point as the onChildProcessExit
		//     callback should always perform an exit( EXIT_FAILURE ) 
		ERROR_MSG( "ChildProcess::startProcess: "
			"Execution in child process failed.\n" );
		isChildProcessRunning_ = false;
	}

	return true;
}


/**
 *	This method starts a process without any pipes to the child process.
 */
bool ChildProcess::startProcess()
{
	int dummyFDs[ 2 ] = { BW_NO_FD, BW_NO_FD };

	return this->startProcessWithFileDescriptors( dummyFDs, dummyFDs );
}


/**
 *	This method runs an external command to consolidate data from secondary
 * 	databases.
 */
bool ChildProcess::startProcessWithPipe( bool shouldPipeStdOut,
	bool shouldPipeStdErr )
{
	if (!shouldPipeStdOut && !shouldPipeStdErr)
	{
		WARNING_MSG( "ChildProcess::startProcessWithPipe: "
			"Not required to pipe either stdout or stderr, consider using "
			"startProcess() instead.\n" );
	}

	int stdOutFDs[2] = { BW_NO_FD, BW_NO_FD };
	int stdErrFDs[2] = { BW_NO_FD, BW_NO_FD };

	if (shouldPipeStdOut)
	{
		// Create a pipe to capture the child's stderr
		if (pipe( stdOutFDs ) != 0)
		{
			ERROR_MSG( "ChildProcess::startProcessWithPipe: "
					"Failed to create stdout pipe: %s\n", strerror( errno ) );
			return false;
		}
	}

	if (shouldPipeStdErr)
	{
		// Create a pipe to capture the child's stderr
		if (pipe( stdErrFDs ) != 0)
		{
			ERROR_MSG( "ChildProcess::startProcessWithPipe: "
					"Failed to create stderr pipe: %s\n", strerror( errno ) );
			if (shouldPipeStdOut)
			{
				close( stdOutFDs[ 0 ] );
				close( stdOutFDs[ 1 ] );
			}
			return false;
		}
	}

	this->startProcessWithFileDescriptors( stdOutFDs, stdErrFDs );

	// Now we are the parent process we can register the pipes for reading.

	if (shouldPipeStdOut)
	{
		stdOutFD_ = stdOutFDs[0];
		pStdOut_ = fdopen( stdOutFD_, "r" );
		eventDispatcher_.registerFileDescriptor( stdOutFD_, this,
			"stdout" );
		// close the write end of the pipe as we only read
		close( stdOutFDs[1] );
	}

	if (shouldPipeStdErr)
	{
		stdErrFD_ = stdErrFDs[0];
		pStdErr_ = fdopen( stdErrFD_, "r" );
		eventDispatcher_.registerFileDescriptor( stdErrFD_, this,
			"stderr" );
		close( stdErrFDs[1] );
	}

	return true;
}



/**
 *	This method executes the desired program which will replace the current
 *	process' context with the new process.
 */
void ChildProcess::execInFork()
{
	// NB: Even if the process we are running is not in the bin/Hybrid64
	//     directory structure, we are still chdir()'ing there. This isn't
	//     too bad as core files will get placed there, however it is
	//     something potentially unexpected.

	// Find the path of the current executable
	BW::string path = ServerUtil::exeDir();

	// NB: Messages from this point on should be printf'd as we may have
	//     redirected the output from log messages
	// Change to the directory this executable is running from
	// (assumes bin/Hybrid64)
	if (chdir( path.c_str() ) == -1)
	{
		printf( "Failed to change directory to %s\n", path.c_str() );
		exit( EXIT_FAILURE );
	}

	// Give the child process a chance to close open file descriptors and
	// sockets from the parent.
	pChildCompletedCB_->onChildAboutToExec();

	// Generate the argument list in the form that execv() desires.
	const char **argv = NULL;

	// We need to add an extra argc as the arg list here doesn't take into
	// account the argv[0] being the process name.
	size_t argc = args_.size() + 1;
	// argv[ argc ] needs to be NULL, so that execv knows when to stop
	argv = new const char*[ argc+1 ];

	argv[ 0 ] = commandToRun_.c_str();
	ArgList::const_iterator iter = args_.begin();
	for( unsigned int currArgc = 1; currArgc < argc; ++currArgc, ++iter)
	{
		argv[ currArgc ] = (*iter).c_str();
	}
	argv[ argc ] = NULL;

	// Now try to turn ourself into the new process.
	int result = execv( commandToRun_.c_str(),
						const_cast< char * const *>( argv ) );

	// Should never reach here unless execv fails.
	delete [] argv;

	if (pChildCompletedCB_)
	{
		pChildCompletedCB_->onExecFailure( result );
	}
	// This is to ensure that regardless of onExecFailure implementation the
	// child process will always be killed.
	exit( EXIT_FAILURE );
}


/**
 *	Handle a signal dispatched from the SignalProcessor.
 */
void ChildProcess::handleSignal( int sigNum )
{
	int		status;
	pid_t 	childPID = ::waitpid( pid_, &status, WNOHANG );

	if (childPID == -1)
	{
		// Not us.
		return;
	}

	MF_ASSERT( childPID == pid_ );

	SignalProcessor::instance().clearSignalHandler( SIGCHLD, this );

	// Some basic checks here for better error reporting, all process specific
	// checks should be performed in the onChildComplete callback.
	if (WIFSIGNALED( status ))
	{
		ERROR_MSG( "ChildProcess::onChildProcessExit: "
				"Process was terminated by signal %d\n",
				int( WTERMSIG( status ) ) );
	}

	this->emptyPipes();
	this->closePipes();

	isChildProcessRunning_ = false;

	MF_ASSERT( pChildCompletedCB_ != NULL );
	pChildCompletedCB_->onChildComplete( status, this );

	// Callback can delete 'this'
}


/**
 *	This method is called when there is data waiting on the pipe from the child
 *	process. It calls the readFromChildProcess() method with the appropriate
 *	file pointer and output buffer.
 */
int ChildProcess::handleInputNotification( int fd )
{
	if (fd == stdOutFD_)
	{
		this->readFromChildProcess( pStdOut_, stdOutOutput_ );
	}
	else if (fd == stdErrFD_)
	{
		this->readFromChildProcess( pStdErr_, stdErrOutput_ );
	}

	return 0;
}


/**
 *	This method reads log data from the child process into the provided stream.
 */
bool ChildProcess::readFromChildProcess( FILE * pFile,
	BW::stringstream & outputStream )
{
	if (!pFile || feof( pFile ))
	{
		return false;
	}

	char buffer[ 1024 ];
	int count = fread( buffer, sizeof( char ), sizeof( buffer ), pFile );
	outputStream.write( buffer, count );

	return true;
}


/**
 *	This method reads any remaining output from the stdout and stderr pipes
 *	(if they exist).
 */
void ChildProcess::emptyPipes()
{
	while (this->readFromChildProcess( pStdOut_, stdOutOutput_ ))
	{
		/* Do nothing */;
	}

	while (this->readFromChildProcess( pStdErr_, stdErrOutput_ ))
	{
		/* Do nothing */;
	}
}

BW_END_NAMESPACE

// child_process.cpp
