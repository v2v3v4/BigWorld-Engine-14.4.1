#ifndef CHILD_PROCESS_HPP
#define CHILD_PROCESS_HPP

#include "network/interfaces.hpp"
#include "server/signal_processor.hpp"

#include <memory>
#include <sstream>
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class ChildProcess;


class IChildProcess
{
public:
	//
	//	Methods called on the parent process.
	//

	/**
	 *	This method is called for both success and failure cases when the
	 *	child process was successfully execv()'d. All remaining output
	 *	should have been read from the stdout and stderr pipes at this
	 *	point.
	 *
	 *	@param status  The exit() return code from the child process.
	 *	@param process A pointer to the ChildProcess class associated that
	 *	               completed.
	 */
	virtual void onChildComplete( int status, ChildProcess * process ) = 0;

	/**
	 *	This method can be implemented to perform specific termination behaviour
	 *	in case the call to execv() fails. Some processes may choose to return
	 *	different error codes on exit() at this point. By default this method
	 *	will call   exit( EXIT_FAILURE ).
	 *
	 *	@param result  The result from the call to execv().
	 */
	virtual void onExecFailure( int result );


	//
	//	Methods called on the child process
	//

	/**
	 *	This method is invoked by the child process after fork() to allow the
	 *	child to have all open sockets and file descriptors closed for it which
	 *	were duplicated as part of the fork().
	 */
	virtual void onChildAboutToExec() = 0;
};


/**
 *	This class handles fork()'ing a child process, exec()'ing
 *	the requested application, and collecting the output status
 *	and return value to notify the parent.
 */
class ChildProcess : public Mercury::InputNotificationHandler, 
		public SignalHandler
{
public:
	ChildProcess( Mercury::EventDispatcher & eventDispatcher,
		IChildProcess * pCallback,
		BW::string commandToRun );
	virtual ~ChildProcess();

	void addArg( const BW::string & newArg );

	BW::string commandLine() const;

	pid_t pid() const	{ return pid_; }

	virtual int handleInputNotification( int fd );

	bool isFinished() const { return (!isChildProcessRunning_); }

	bool startProcess();
	bool startProcessWithPipe( bool shouldPipeStdOut, bool shouldPipeStdErr );

	BW::string stdOut() const { return stdOutOutput_.str(); }
	BW::string stdErr() const { return stdErrOutput_.str(); }

private:
	bool startProcessWithFileDescriptors( int stdOutFDs[ 2 ],
		int stdErrFDs[ 2 ] );

	void execInFork();

	virtual void handleSignal( int sigNum );

	bool readFromChildProcess( FILE * pFile, BW::stringstream & outputStream );
	void emptyPipes();
	void closePipes();


	Mercury::EventDispatcher & eventDispatcher_;

	IChildProcess * pChildCompletedCB_;

	BW::string commandToRun_;

	typedef BW::vector< BW::string > ArgList;
	ArgList args_;

	pid_t pid_;

	int stdOutFD_;
	FILE * pStdOut_;
	BW::stringstream stdOutOutput_;

	int stdErrFD_;
	FILE * pStdErr_;
	BW::stringstream stdErrOutput_;

	bool isChildProcessRunning_;
};

BW_END_NAMESPACE

#endif // CHILD_PROCESS_HPP
