#include "pch.hpp"

#include "debug_exception_filter.hpp"

#include "debug.hpp"

#if defined( _XBOX )

#include <xtl.h>

#elif defined( _WIN32 )

#include <time.h>

#endif

#if ENABLE_STACK_TRACKER

#include "stack_tracker.hpp"

#endif // ENABLE_STACK_TRACKER


BW_BEGIN_NAMESPACE

#if defined( _WIN32 )

#define EXCEPTION_CASE(e) case e: return #e


/**
 *
 */
static const char* exceptionCodeAsString( DWORD code )
{
	switch (code)
	{
	EXCEPTION_CASE( EXCEPTION_ACCESS_VIOLATION );
	EXCEPTION_CASE( EXCEPTION_INVALID_HANDLE );
	EXCEPTION_CASE( EXCEPTION_STACK_OVERFLOW );
	EXCEPTION_CASE( EXCEPTION_PRIV_INSTRUCTION );
	EXCEPTION_CASE( EXCEPTION_ILLEGAL_INSTRUCTION );
	EXCEPTION_CASE( EXCEPTION_BREAKPOINT );
	default:
		return "EXCEPTION";
	}
}


/**
 *
 */
static const char* accessViolationTypeAsString( ULONG_PTR type )
{
	switch (type)
	{
		case 0:
			return "Read";
		case 1:
			return "Write";
		case 8:
			return "Data Execution";
		default:
			return "Unknown";
	}
}


// flags to control exiting without msg and without error on exception
// used by open automate:

// On exception we want an error code (and not a message)
bool exitWithErrorCodeOnException = false;

// On exit even with exception we want to return 0 (as the exception was
// probably caused during exit due to singletons order)
int errorCodeForExitOnException = 1;

#if ENABLE_STACK_TRACKER


/**
 *
 */
DWORD ExceptionFilter( DWORD exceptionCode, struct _EXCEPTION_POINTERS * ep )
{
	PEXCEPTION_RECORD er = ep->ExceptionRecord;

	char extraMsg[ 1024 ];
	extraMsg[0] = '\0';

	if ((exceptionCode == EXCEPTION_ACCESS_VIOLATION) &&
		(er->NumberParameters >= 2))
	{
		bw_snprintf( extraMsg, sizeof( extraMsg ), "%s @ 0x%p",
			accessViolationTypeAsString( er->ExceptionInformation[0] ),
			er->ExceptionInformation[ 1 ] );
	}

	if (exitWithErrorCodeOnException)
	{
		ERROR_MSG( "Exception (%s : 0x%08X @ 0x%p) received, exiting.",
			exceptionCodeAsString( exceptionCode ), exceptionCode,
			er->ExceptionAddress);

		_exit( errorCodeForExitOnException );
	}

	// The critical message handler will append the stack trace for us.
	CRITICAL_MSG( "The BigWorld Client has encountered an unhandled exception "
			"and must close (%s : 0x%08X @ 0x%p) (%s)",
		exceptionCodeAsString( exceptionCode ), exceptionCode,
		er->ExceptionAddress, extraMsg);

	return EXCEPTION_CONTINUE_SEARCH;
}

#endif // ENABLE_STACK_TRACKER

#if !defined( _XBOX360 )

/**
 * This function exists with an error code when an exception happens
 */
LONG WINAPI ErrorCodeExceptionFilter( _EXCEPTION_POINTERS * exceptionInfo )
{
	ERROR_MSG( "Exception (%s : 0x%08lX) received, exiting.",
		exceptionCodeAsString( exceptionInfo->ExceptionRecord->ExceptionCode ),
		exceptionInfo->ExceptionRecord->ExceptionCode );

	_exit( 1 );
}


/**
 * This function exists with 0 code (success) when an exception happens
 */
LONG WINAPI SuccessExceptionFilter( _EXCEPTION_POINTERS * exceptionInfo )
{
	ERROR_MSG( "Exception (%s : 0x%08lX) received, exiting with success (0) "
			"error code.",
		exceptionCodeAsString( exceptionInfo->ExceptionRecord->ExceptionCode ),
		exceptionInfo->ExceptionRecord->ExceptionCode );

	_exit( 0 );
}

#endif // !defined( _XBOX360 )

#endif // defined( _WIN32 )

BW_END_NAMESPACE

// debug_exception_filter.cpp
