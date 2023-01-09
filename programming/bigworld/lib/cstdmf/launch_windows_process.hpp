#ifndef LAUNCH_PROCESS_HPP
#define LAUNCH_PROCESS_HPP

#include "bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE



#if defined _WIN32

#include "cstdmf_windows.hpp"

typedef struct  
{
	HANDLE process;
	HANDLE stdOutProcess;
	HANDLE stdErrProcess;
	HANDLE stdOutRead;
	HANDLE stdErrRead;
} ProcessHandles;

/**
 *	Launches an external process and waits for it to complete.
 *
 *	@param process name of the process to launch
 *
 *	@param args the arguments to pass to the process
 *
 *	@param pHandlesOut an optional structure used to store the handles of the process created
 *						if not NULL, stdOut & stdErr are piped to handles within this structure
 *						that can be read by passOnProcessOutput later on.
 *
 *  @param bCreatePipes if true creates some io pipes to pipe the stOut & StdErr from the launched process
 *                       to the parents stdOut & stdErr.
 *      
 *	@return the process exit code
 */
CSTDMF_DLL DWORD launchProcess( const BW::string &process, const BW::string &args, 
							   ProcessHandles & handlesOut, bool bCreatePipes, 
							   bool bCreateWindow, bool bSetFocus = false );

/**
 *	Reads, & outputs the stdOut & StdErr of the process handles passed in
 *	 used in conjunction with launchProcess
 *
 *	@param ph structure containing the created processes handles
 *
 *	@return the process exit code
 */

CSTDMF_DLL DWORD	passOnProcessOutput( ProcessHandles& ph );

/**
 *	Terminates a running process, and waits for it to end if required
 *
 */
void	terminateProcessAndWait(HANDLE& processHandle, bool bWait );

#endif // defined _WIN32


BW_END_NAMESPACE

#endif // LAUNCH_PROCESS_HPP

