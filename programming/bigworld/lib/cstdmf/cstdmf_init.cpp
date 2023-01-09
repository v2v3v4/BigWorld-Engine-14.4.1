#include "pch.hpp"

#include "cstdmf_init.hpp"

#include "bw_util.hpp"
#include "log_msg.hpp"
#include "string_builder.hpp"

BW_BEGIN_NAMESPACE

/// cstdmf library Singleton
BW_SINGLETON_STORAGE( CStdMf );

#ifndef MF_SERVER

void CStdmfInvalidParameterHandler( const wchar_t* expression,
							   const wchar_t* function,
							   const wchar_t* file,
							   unsigned int line,
							   uintptr_t pReserved )
{
	char char_expression[256] = { 0 };
	char char_function[256] = { 0 };
	char char_file[256] = { 0 };
	
	wcstombs( char_expression, expression, 256 );
	wcstombs( char_function, function, 256 );
	wcstombs( char_file, file, 256 );

	char errorMsgString[2048] = { 0 };
	BW::StringBuilder errorStringBuilder( errorMsgString, ARRAY_SIZE( errorMsgString ) );

	errorStringBuilder.append( "============INVALID PARAMETER===========================" );
	errorStringBuilder.append( BW_NEW_LINE );
	errorStringBuilder.appendf( "Invalid parameter detected in call to %s%s", char_function, char_expression );
	errorStringBuilder.append( BW_NEW_LINE );
	errorStringBuilder.appendf( " File %s, Line: %d", char_file, line );
	errorStringBuilder.append( BW_NEW_LINE );

#ifdef _WIN32
	char lastErrorString[2048] = { 0 };
	if (BWUtil::getLastErrorMsg( lastErrorString,2048 ))
	{
		errorStringBuilder.append( lastErrorString );
	}
#endif

	CRITICAL_MSG( "%s", errorStringBuilder.string() );
}
#endif // MF_SERVER


/**
 *	Default Constructor.
 */
CStdMf::CStdMf()
{
	DebugFilter::instance();
	DogWatchManager::instance();

	// redirect any bad parameters to our debug function
#ifdef _WIN32
	_set_invalid_parameter_handler( CStdmfInvalidParameterHandler );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG ); // disable the error dialog box, but still send output to the debugger output window.
#endif
}


/**
 *	Destructor.
 */
CStdMf::~CStdMf()
{
	DogWatchManager::fini();
	DebugFilter::fini();
}


/**
 *
 */
/*static*/bool CStdMf::checkUnattended()
{
	static bool checkedUnattended = false;
	static bool unattended = false;

	if (checkedUnattended)
	{
		return unattended;
	}

#if defined(_WINDOWS_)
	// get command line options
	LPSTR commandLine = ::GetCommandLineA();
	
	// check for the callstack mode
	if (strstr( commandLine, "-unattended" ))
	{
		unattended = true;
	}

	// check environment variable
	char unattendedBuffer[256];
	DWORD retLength = ::GetEnvironmentVariableA( "BW_UNATTENDED_MACHINE",
		(LPSTR)unattendedBuffer, ARRAY_SIZE( unattendedBuffer ) );

	if (retLength > 0)
	{
		// check the value
		if (strstr( unattendedBuffer, "TRUE" ))
		{
			unattended = unattended | true;
		}
		else if (strstr( unattendedBuffer, "FALSE" ))
		{
			unattended = unattended | false;
		}
	}

	if (unattended)
	{
		SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX |
			SEM_NOALIGNMENTFAULTEXCEPT | SEM_NOOPENFILEERRORBOX );

		LogMsg::automatedTest( true );

		_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
		_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
		_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR  );
		_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE );
		_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE );
		_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE );

		_set_error_mode(_OUT_TO_STDERR);
	}
#endif

	checkedUnattended = true;
	return unattended;
}

BW_END_NAMESPACE

// cstdmf.cpp
