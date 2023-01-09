#include "pch.hpp"
#include "launch_windows_process.hpp"
#include "guard.hpp"
#include "string_utils.hpp"
#include "bw_util.hpp"

BW_BEGIN_NAMESPACE

#if defined _WIN32

/**
 *	pHandlesOut is a struct containing the process handle for terminating,
 *		and the stdOut & StdErr pipes that need to be read & closed later on
 *		in passOnProcessOutput. 
 *
 *	@param process String containing the name of the executable to launch
 *	@param args String containing the arguments to the executable
 *	@param handlesOut Structure returning the Handles of the new process & its io
 *	@param	bCreatePipes. bool to indicate whether to create io pipes to pipe stdOut & stdErr
 *		from the created process.
 *
 */
DWORD launchProcess( const BW::string &process, const BW::string &args,
					ProcessHandles & handlesOut, bool bCreatePipes, 
					bool bCreateWindow, bool bSetFocus  )
{
	BW_GUARD;

	// construct the process command line
	const size_t cmdLineLength = 1024;
	wchar_t wCommandLineBuf[cmdLineLength];
	bw_snwprintf( wCommandLineBuf, 
		cmdLineLength,
		L"%S.exe %S",
		process.c_str(),
		args.c_str() );

	HANDLE stdoutProcess = NULL, stderrProcess = NULL;
	HANDLE stdoutRead = NULL, stderrRead = NULL;

	SECURITY_ATTRIBUTES saAttr; 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL;

	if (bCreatePipes == true)
	{
		//create stdout pipe
		if (!CreatePipe(&stdoutRead,&stdoutProcess,&saAttr,0))  
		{
			ERROR_MSG( "Failed to create stdout pipe\n" );
			return S_FALSE;
		}

		//create stderr pipe
		if (!CreatePipe(&stderrRead,&stderrProcess,&saAttr,0))  
		{
			ERROR_MSG( "Failed to create stderr pipe\n" );
			CloseHandle(stdoutProcess);
			CloseHandle(stdoutRead);
			return S_FALSE;
		}
	}
	
	// spawn the process
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si,sizeof(si));
	si.cb = sizeof(si);
	
	if (bCreatePipes == true)
	{
		si.hStdError = stderrProcess;
		si.hStdOutput = stdoutProcess;
	}
	else
	{
		si.hStdError = NULL;
		si.hStdOutput = NULL;
	}

	if (bSetFocus == false)
	{
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_SHOWMINNOACTIVE;
	}

	TRACE_MSG( "Launching windows process: %ls\n", wCommandLineBuf );
	si.hStdInput = NULL;
	si.dwFlags |= STARTF_USESTDHANDLES;
	ZeroMemory(&pi,sizeof(pi));
	if (!CreateProcess(NULL,
		wCommandLineBuf,
		NULL,
		NULL,
		TRUE,
		bCreateWindow ? NULL : CREATE_NO_WINDOW,
		NULL,
		NULL,
		&si,
		&pi)
		)
	{
		char windowsErrorMessage[2048];
		if (BWUtil::getLastErrorMsg(windowsErrorMessage,2048))
		{
			ERROR_MSG( "Failed to launch process %s %s\n Error was: %s\n", 
				process.c_str(), args.c_str(), windowsErrorMessage );
		}
		else
		{
			ERROR_MSG( "Failed to launch process %s %s\n", process.c_str(), 
				args.c_str() );
		}

		if (bCreatePipes == true)
		{
			CloseHandle(stdoutProcess);
			CloseHandle(stdoutRead);
			CloseHandle(stderrProcess);
			CloseHandle(stderrRead);
		}
		handlesOut.process = NULL;
		handlesOut.stdOutRead = NULL;
		handlesOut.stdOutProcess = NULL;
		handlesOut.stdErrRead = NULL;
		handlesOut.stdErrProcess = NULL;
		return S_FALSE;
	}

	handlesOut.process = pi.hProcess;
	if (bCreatePipes == true)
	{
		handlesOut.stdOutRead = stdoutRead;
		handlesOut.stdOutProcess = stdoutProcess;
		handlesOut.stdErrRead = stderrRead;
		handlesOut.stdErrProcess = stderrProcess;
	}

	return S_OK;
}

/**
 *	passOnProcessOutput:
 *
 *		read the stdout & stderr from the process passed in, and send it 
 *		to the current processes stdout & stderr. 
 *		Also, wait until the process has finished before returning
 *
 *	@param ph a structure holding the handles to the process to read and
 *				wait on
 */

DWORD	passOnProcessOutput( ProcessHandles& ph )
{
	DWORD dwRet = S_OK;
	if (ph.process)
	{
		while (true)
		{
			if (!GetExitCodeProcess(ph.process, &dwRet))
			{
				WARNING_MSG("[passOnProcessOutput] While waiting for process to"
					" exit, getExitCodeProcess() returned a failure\n");
				dwRet = S_FALSE;
				break;
			}

			char buf[1024];
			unsigned long read, written, avail;
			while (true)
			{
				PeekNamedPipe( ph.stdOutRead, buf, 1023, &read, &avail, NULL );
				if (read == 0)
				{
					break;
				}

				// redirect the processes std output to our std output
				memset( buf, 0, 1024 );
				ReadFile( ph.stdOutRead, buf, 1023 ,&read, NULL );
				WriteFile( GetStdHandle(STD_OUTPUT_HANDLE), buf, read, &written, NULL );
				OutputDebugStringA( buf );
			}

			while (true)
			{
				PeekNamedPipe( ph.stdErrRead, buf, 1023, &read, &avail, NULL );
				if (read == 0)
				{
					break;
				}

				// redirect the processes std err to our std err
				memset( buf, 0, 1024 );
				ReadFile( ph.stdErrRead, buf, 1023 ,&read, NULL );
				WriteFile( GetStdHandle(STD_ERROR_HANDLE), buf, read, &written, NULL );
				OutputDebugStringA( buf );
			}

			if (dwRet != STILL_ACTIVE)
			{
				break;
			}
		}

	}
	else
	{
		dwRet = S_FALSE;
	}
	
	return dwRet;
}

void terminateProcessAndWait(HANDLE& processHandle, bool bWait )
{
	TerminateProcess(processHandle,0);

	if ( bWait == true )
	{
		WaitForSingleObject(processHandle,INFINITE);

		CloseHandle(processHandle);
	}
}

#endif // defined _WIN32

BW_END_NAMESPACE

// launch_process.cpp
