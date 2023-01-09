#include "pch.hpp"

#include "log_msg.hpp"
#include "bw_util.hpp"
#include "debug_filter.hpp"
#include "dprintf.hpp"
#include "guard.hpp"
#include "log_meta_data.hpp"
#include "string_builder.hpp"

#if defined( _WIN32 )
# if ENABLE_ENTER_DEBUGGER_MESSAGE
#include "critical_error_handler.hpp"
# endif // ENABLE_ENTER_DEBUGGER_MESSAGE

#include "cstdmf_init.hpp"
#include "memory_load.hpp"
#include "string_utils.hpp"
#endif // defined( _WIN32 )

#if defined( __linux__ ) || defined( EMSCRIPTEN )
#include "timestamp.hpp"
#endif // __linux__ || defined( EMSCRIPTEN )


#define	BW_PLATFORM_SUPPORTS_SYSLOG	(!defined( _WIN32 ) && !defined( PLAYSTATION3 ))

#if BW_PLATFORM_SUPPORTS_SYSLOG
# include <syslog.h>
# include <unistd.h>
#endif // BW_PLATFORM_SUPPORTS_SYSLOG

#if BW_UNIX_BACKTRACE
# define MAX_DEPTH 50
# include <execinfo.h>
# include <cxxabi.h>
#endif // BW_UNIX_BACKTRACE

#if ENABLE_MINI_DUMP
#pragma warning(push)
#pragma warning(disable: 4091)
#include <DbgHelp.h>
#pragma warning(pop)
#endif // ENABLE_MINI_DUMP

BW_BEGIN_NAMESPACE


static const int MAX_BUFFER_SIZE = 65536;
#if BW_ENABLE_STACKTRACE
static const int MAX_FROZEN_THREADS = 64;
#endif

bool g_shouldWriteToSyslog = false;

static const char s_developmentAssertionMessage[] = 
	"Development assertions may indicate failures caused by incorrect "
	"engine usage.\n"
	"In "
#if defined( MF_SERVER )
	"production mode"
#else
	"Release builds"
#endif
	", they do not cause the application to exit.\n"
	"Please investigate potential misuses of the engine at the time of the "
	"failure.\n";


#if defined( _WIN32 )
#define DEBUG_LOG_FILE "debug.log"

/*static*/ bool LogMsg::s_hasSeenCriticalMsg_ = false;
/*static*/ bool LogMsg::s_automatedTest_ = false;
/*static*/ char LogMsg::s_debugLogFile_[1024] = DEBUG_LOG_FILE;

static const BW::string s_executablePath = BWUtil::executablePath();
#endif // defined( _WIN32 )



/**
 *	Constructor.
 *
 *	@param priority
 *	@param pCategory
 */
LogMsg::LogMsg( DebugMessagePriority priority, const char * pCategory ) :
	inWrite_( false ),
	priority_( priority ),
	source_( MESSAGE_SOURCE_CPP ),
	pCategory_( pCategory )
{
}


/**
 *	This method sets the category for the log message.
 *
 *	@param pCategory
 *
 *	@return This method always returns this.
 */
LogMsg & LogMsg::category( const char * pCategory )
{
	pCategory_ = pCategory;
	return *this;
}


/**
 *	This method sets the category for the log message.
 *
 * 	@param pCategory
 *
 *	@return This method always returns this.
 */
LogMsg & LogMsg::operator[]( const char * pCategory )
{
	BW_GUARD;

	return this->category( pCategory );
}


/**
 *	This method sets the origin type of the log message.
 *
 *	@param source
 *
 *	@return This method always returns this.
 */
LogMsg & LogMsg::source( DebugMessageSource source )
{
	source_ = source;
	return *this;
}


/**
 *	This method handles the addition of a JSONified string of metadata.
 *
 *	This method will hopefully be deprecated in the future as it is only
 *	utilised from within the Python logging layer (pyscript/py_logging.cpp).
 *
 *	The responsibility for the parsing of the JSON will be placed into the
 *	log receiving side (in MessageLogger).
 *
 *	@param pMetaData A string of pre-JSONified metadata (ignored if NULL).
 *
 *	@return This method always returns this.
 */
CSTDMF_DLL LogMsg & LogMsg::metaAsJSON( const char * pMetaData )
{
	if (pMetaData)
	{
		metaData_.addJSON( pMetaData );
	}

	return *this;
}



/**
 *	This method is the primary entry point for writing a log message.
 *
 *	Messages with a CRITICAL severity are sent to the criticalMessageHelper,
 *	all other messages are immediately logged.
 */
void LogMsg::write( const char * pFormat, ... )
{
	BW_GUARD;

	// Lots of magic for args processing and filtering
	// Pass off to the network handlers
	// network handler receives LogMetaData object (or the MemoryOStream)
	va_list argPtr;
	va_start( argPtr, pFormat );

	if (this->shouldInvokeCriticalHandler())
	{
		this->criticalMessageHelper( false, pFormat, argPtr );
	}
	else
	{
		this->doWrite( pFormat, argPtr );
	}

	va_end( argPtr );
}


/**
 *	This method is a clone of write() to be used within criticalMessageHelper.
 */
void LogMsg::writeHelper( const char * pFormat, ... )
{
	BW_GUARD;

	va_list argPtr;
	va_start( argPtr, pFormat );

	this->doWrite( pFormat, argPtr );

	va_end( argPtr );
}


/**
 *	This method is the primary entry point for writing a log message.
 *
 *	Messages with a CRITICAL severity are sent to the criticalMessageHelper,
 *	all other messages are immediately logged.
 */
void LogMsg::operator()( const char * pFormat, ... )
{
	BW_GUARD;

	va_list argPtr;
	va_start( argPtr, pFormat );

	if (this->shouldInvokeCriticalHandler())
	{
		this->criticalMessageHelper( false, pFormat, argPtr );
	}
	else
	{
		this->doWrite( pFormat, argPtr );
	}

	va_end( argPtr );
}


/**
 *	This method provides the bulk of the write functionality once the var args
 *	has been processed.
 *
 *	@param pFormat	The format string of the message to print.
 *	@param argPtr	A va_list structure to the arguments for the format string.
 */
void LogMsg::doWrite( const char * pFormat, va_list argPtr )
{
	BW_GUARD;

	PROFILER_SCOPED( LogMsg_doWrite );

	bool shouldAccept = false;

#if !ENABLE_MSG_LOGGING
#if defined( _WIN32 )
	if (!DebugFilter::alwaysHandleMessage())
	{
		return;
	}
#endif // _WIN32
#else
	shouldAccept = DebugFilter::shouldAccept( priority_, pCategory_ );
#endif


	// Break early if this message should be filtered out.
	if (
#if defined( _WIN32 )
	!DebugFilter::alwaysHandleMessage() && 
#endif
	!shouldAccept)
	{
		return;
	}

	const size_t dmhBufferSize = 16384;

#if BW_PLATFORM_SUPPORTS_SYSLOG
	// send to syslog if it's been initialised
	if ((g_shouldWriteToSyslog) &&
		( (priority_ == MESSAGE_PRIORITY_ERROR) ||
		  (priority_ == MESSAGE_PRIORITY_CRITICAL) ))
	{
		char buffer[ dmhBufferSize * 2 ];

		// Need to make a copy of the va_list here to avoid crashing on 64bit
		va_list tmpArgPtr;
		bw_va_copy( tmpArgPtr, argPtr );
		vsnprintf( buffer, sizeof(buffer), pFormat, tmpArgPtr );
		buffer[ sizeof( buffer ) - 1 ] = '\0';
		va_end( tmpArgPtr );

		syslog( LOG_CRIT, "%s", buffer );
	}
#endif // BW_PLATFORM_SUPPORTS_SYSLOG

	bool handled = DebugFilter::instance().handleMessage(
				priority_, pCategory_, source_, metaData_, pFormat, argPtr );

	if (!handled && shouldAccept)
	{
#if BW_ENABLE_STACKTRACE
		// if its an error_message, add the callstack to it.
		if (priority_ == MESSAGE_PRIORITY_ERROR &&
			(source_ != MESSAGE_SOURCE_SCRIPT ||
			DebugFilter::shouldOutputErrorBackTrace()))
		{
			char newFormatString[ dmhBufferSize * 2 ] = { 0 };
			BW::StringBuilder newFormatStringBuilder(
				newFormatString, ARRAY_SIZE( newFormatString ) );

			newFormatStringBuilder.append( pFormat );

			// add the callstack string onto this error message
			char stackMsgString[16384] = { 0 };

			if (BW::stacktraceAccurate( stackMsgString, 16384 ))
			{
				newFormatStringBuilder.append( stackMsgString );
			}


			vdprintf( newFormatStringBuilder.string(),
				argPtr, priority_, pCategory_ );
		}
		else
#endif // BW_ENABLE_STACKTRACE
		{
			vdprintf( pFormat, argPtr, priority_, pCategory_ );
		}
	}
}


/**
 *	This method dumps a backtrace to logs.
 */
void LogMsg::writeBackTrace()
{
	// TODO: This behaviour has changed from the previous version.
	//       It used to write a new line for every stack line, this is
	//       bundled together.
#if BW_UNIX_BACKTRACE
	char buffer[ MAX_BUFFER_SIZE ];
	BW::StringBuilder stackBuilder( buffer, ARRAY_SIZE( buffer ) );

	this->messageBackTrace( &stackBuilder, /*shouldAddToMetaData*/ false );
	this->write( "%s\n", buffer );
#endif // BW_UNIX_BACKTRACE

	return;
}


#if BW_UNIX_BACKTRACE
/**
 *
 */
void LogMsg::messageBackTrace( StringBuilder * pBuilder,
	bool shouldAddToMetaData )
{
	BW_GUARD;

	if ((pBuilder == NULL) && !shouldAddToMetaData)
	{
		return;
	}

	void ** traceBuffer = new void * [MAX_DEPTH];
	uint32 depth = backtrace( traceBuffer, MAX_DEPTH );
	char ** traceStringBuffer = backtrace_symbols( traceBuffer, depth );

	char stackBuffer[ MAX_BUFFER_SIZE ];
	bw_zero_memory( stackBuffer, ARRAY_SIZE( stackBuffer ) );
	BW::StringBuilder stackBuilder( stackBuffer, ARRAY_SIZE( stackBuffer ) );

	for (uint32 i = 0; i < depth; i++)
	{
		// Format: <executable path>(<mangled-function-name>+<function
		// instruction offset>) [<eip>]
		BW::string functionName;

		BW::string traceString( traceStringBuffer[i] );
		BW::string::size_type begin = traceString.find( '(' );
		bool gotFunctionName = (begin != BW::string::npos);

		if (gotFunctionName)
		{
			// Skip the round bracket start.
			++begin;
			BW::string::size_type bracketEnd = traceString.find( ')', begin );
			BW::string::size_type end = traceString.rfind( '+', bracketEnd );
			BW::string mangled( traceString.substr( begin, end - begin ) );

			int status = 0;
			size_t demangledBufferLength = 0;
			char * demangledBuffer = abi::__cxa_demangle( mangled.c_str(), 0, 
				&demangledBufferLength, &status );

			if (demangledBuffer)
			{
				functionName.assign( demangledBuffer, demangledBufferLength );

				// __cxa_demangle allocates the memory for the demangled
				// output using malloc(), we need to free it.
#if defined( ENABLE_MEMTRACKER )
				raw_free( demangledBuffer );
#else
				free( demangledBuffer );
#endif // defined( ENABLE_MEMTRACKER )
			}
			else
			{
				// Didn't demangle, but we did get a function name, use that.
				functionName = mangled;
			}
		}

		stackBuilder.appendf( "#%d %s\n",
			i,
			(gotFunctionName) ? functionName.c_str() : traceString.c_str() );
	}

	if (pBuilder)
	{
		pBuilder->append( stackBuilder.string() );
	}

	if (shouldAddToMetaData)
	{
		// TODO: stackTrace assumes only a single stacktrace (ie: from a single
		//       thread). Should be consider building in support for multiple
		//       stacks?
		this->meta( LogMetaData::key_stackTrace, stackBuffer );
	}

#if defined( ENABLE_MEMTRACKER )
	raw_free( traceStringBuffer );
#else
	free( traceStringBuffer );
#endif // defined( ENABLE_MEMTRACKER )

	bw_safe_delete_array( traceBuffer );
}

#endif // BW_UNIX_BACKTRACE



/**
 *	This method generates any output that should be seen before a development
 *	assertion may early exit from the critical message handling method.
 */
void LogMsg::generateDevAssertMsg( StringBuilder & strBuilder,
	const char * pFormat, va_list argPtr )
{
#if defined( _WIN32 )
	this->win32GenerateCriticalMsg( strBuilder, pFormat, argPtr );
#else
	strBuilder.vappendf( pFormat, argPtr );
#endif // defined( _WIN32 )

	return;
}


/**
 *	This method is responsible for a outputting a critical message.
 *
 *	In order to output a critical message, the following flow is taken.
 *
 *	1) Convert the format string / args into a printable string.
 *	2) Generate a stack trace (and optionally add it as metadata).
 *	3) Print the message (and return if it is a development assertion).
 *	4) Pass the message to critical message handlers (if attached).
 *	5) Dump a core file and enter a debugger.
 */
void LogMsg::criticalMessageHelper( bool isDevAssertion,
		const char * pFormat, va_list argPtr )
{
	BW_GUARD;

	char buffer[ MAX_BUFFER_SIZE ];
	BW::StringBuilder stackBuilder( buffer, ARRAY_SIZE( buffer ) );

#if defined( _WIN32 )
	LogMsg::hasSeenCriticalMsg( true );
#endif // _WIN32

	this->generateDevAssertMsg( stackBuilder, pFormat, argPtr );

	if (isDevAssertion && !DebugFilter::instance().hasDevelopmentAssertions())
	{
		priority_ = MESSAGE_PRIORITY_WARNING;
	}

	// TODO: Note, this has moved up from being just before the call to
	//       handleCriticalMessage() as it is adding metadata and must be part
	//       of the call to write() which goes out to the network.
#if BW_UNIX_BACKTRACE
	this->messageBackTrace( NULL, /*shouldAddToMetaData*/ true );
#endif // BW_UNIX_BACKTRACE

	// Output the formatted message immediately
	this->writeHelper( "%s", buffer );


#if BW_PLATFORM_SUPPORTS_SYSLOG
	// send to syslog if it's been initialised
	if (g_shouldWriteToSyslog)
	{
		syslog( LOG_CRIT, "%s", buffer );
	}
#endif // BW_PLATFORM_SUPPORTS_SYSLOG


	// Now a special message for development assertions
	if (isDevAssertion)
	{
		// TODO: this write() will now re-send metadata associated with the
		//       the initial critical message.
		this->writeHelper( "%s", s_developmentAssertionMessage );
	}

	if (isDevAssertion && !DebugFilter::instance().hasDevelopmentAssertions())
	{
		// dev assert and we don't have dev asserts enabled
		return;
	}

	// TODO: by only adding to the metadata, handleCriticalMessage no longer
	//       sees the stack being output

	// now do program specific critical message handling.
	DebugFilter::instance().handleCriticalMessage( buffer );

#if defined( _WIN32 )
	// TODO: This seems to be doing the same thing as messageBackTrace().
	//       should we re-order this in order to allow a stack to be sent
	//       from Windows / Client / Content Tools -> MessageLogger?
	this->win32StackTraceThreads( stackBuilder );
#endif // _WIN32

#if defined( _XBOX360 )
	this->xbox360Stack( buffer );
#elif defined( _XBOX )

	OutputDebugString( buffer );
	ENTER_DEBUGGER();

#elif defined ( PLAYSTATION3 )

	printf( buffer );
	printf( "\n" );
	ENTER_DEBUGGER();

#elif defined( _WIN32 )

	this->win32DumpAndAbort( buffer );

#elif defined( __linux__ ) || defined( EMSCRIPTEN )

	this->linuxAssertAndAbort( buffer );
    
#elif defined( __APPLE__ )
    
    // TODO: This is tracking by BWT-28578

#else

#error "No stack generation implemented"

#endif // defined( _XBOX360 )
}


#if defined( _WIN32 )

/**
 *	This method is utilised by the Client and Content tools to generate a
 *	a stack trace message formatted for use under Windows.
 *
 *	@param strBuilder This is a string builder used to format the string for
 *	                  output.
 */
void LogMsg::win32GenerateCriticalMsg( StringBuilder & strBuilder,
	const char * pFormat, va_list argPtr )
{
	time_t rawtime;
	struct tm *timeinfo = NULL;

	time( &rawtime );
	timeinfo = localtime( &rawtime );

	strBuilder.appendf( "Application %s crashed ", s_executablePath.c_str() );
	char timeBuffer[2048] = { 0 };
	strftime( timeBuffer, ARRAY_SIZE( timeBuffer ) - 1, 
		"%m.%d.%Y at %H:%M:%S" BW_NEW_LINE BW_NEW_LINE "Message:" BW_NEW_LINE, 
		timeinfo );
	strBuilder.append( timeBuffer );
	strBuilder.vappendf( pFormat, argPtr );

#if ENABLE_STACK_TRACKER
	if (StackTracker::stackSize() > 0)
	{
		char stack[4098] = { 0 };
		StackTracker::buildReport( stack, ARRAY_SIZE( stack ) - 1 );
		strBuilder.append( BW_NEW_LINE BW_NEW_LINE "Guard trace:" BW_NEW_LINE );
		strBuilder.append( stack );
		strBuilder.append( BW_NEW_LINE BW_NEW_LINE );
	}
#endif // ENABLE_STACK_TRACKER
}


/**
 *	This method generates a stacktrace for each running thread on Windows.
 *
 *	@param builder A string builder to output the stacks to.
 */
void LogMsg::win32StackTraceThreads( StringBuilder & builder )
{

#if BW_ENABLE_STACKTRACE
	FrozenThreadInfo frozenThreads[MAX_FROZEN_THREADS] = { { 0 } };
	const size_t frozenThreadsCount = freezeAllThreads( frozenThreads,
		MAX_FROZEN_THREADS );

	builder.append( BW_NEW_LINE BW_NEW_LINE
		"Native trace:" BW_NEW_LINE BW_NEW_LINE );

	if (BW::stacktraceSymbolResolutionAvailable())
	{
		for (size_t i = 0; i < frozenThreadsCount; ++i)
		{
			char stack[4096] = { 0 };
			BW::stacktraceAccurate( stack, ARRAY_SIZE( stack ) - 1, 0,
				static_cast< int >( frozenThreads[i].id ) );
			builder.append( stack );
			builder.append( BW_NEW_LINE );
		}
	}
	else
	{
		builder.append( "Symbols not available" BW_NEW_LINE BW_NEW_LINE );
	}

# if BUILT_BY_BIGWORLD
	// BigWorld uploads crash reports to be uploaded over ftp. This requires
	// threads to be resumed so the network transport thread is working.
	bool unfreeze = true;
# else // BUILT_BY_BIGWORLD
	bool unfreeze = false;
# endif // BUILT_BY_BIGWORLD

	cleanupFrozenThreadsInfo( frozenThreads, frozenThreadsCount, unfreeze );

#else
	freezeAllThreads( NULL, 0 );
#endif // ENABLE_NATIVE_STACK

	builder.appendf( "Memory status:" BW_NEW_LINE
		"System: %lld/%lld [%3.2f%% used]" BW_NEW_LINE, Memory::usedMemory(),
		Memory::maxAvailableMemory(), Memory::memoryLoad() );
}


/**
 *	This method logs the error to a file and then terminates the application.
 *
 *	@param buffer A string containing the message associated with the abort.
 */
void LogMsg::win32DumpAndAbort( char buffer[] )
{
	if (s_automatedTest_ || CStdMf::checkUnattended())
	{
		_set_abort_behavior( 0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT );
		LogMsg::logToFile( "Critical Error, aborting test." );
		LogMsg::logToFile( buffer );
#if ENABLE_MINI_DUMP
		LogMsg::createMiniDump();
#endif // ENABLE_MINI_DUMP
		abort();
	}

#if ENABLE_ENTER_DEBUGGER_MESSAGE
	// Disable all abort() behaviour in case we call it
	_set_abort_behavior( 0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT );

	if (CriticalErrorHandler::get())
	{
		switch (CriticalErrorHandler::get()->ask( buffer ))
		{
		case CriticalErrorHandler::ENTERDEBUGGER:
			CriticalErrorHandler::get()->recordInfo( false );
			ENTER_DEBUGGER();
			break;
		case CriticalErrorHandler::EXITDIRECTLY:
			CriticalErrorHandler::get()->recordInfo( true );
			abort();
			break;
		}
	}
	else
	{
		abort();
	}
#else // ENABLE_ENTER_DEBUGGER_MESSAGE
	strcat( buffer, "\n\nThe application must exit.\n" );
	::MessageBox( 0, bw_utf8tow( buffer ).c_str(),
		L"Critical Error Occurred", MB_ICONHAND | MB_OK );
	abort();
#endif // ENABLE_ENTER_DEBUGGER_MESSAGE
}
#endif // defined( _WIN32 )


#if defined( _WIN32 )
/**
 *	This static method outputs a message to a debug log file.
 *
 *	@param pMessageString A pointer to a string containing message to write.
 */
/*static*/ void LogMsg::logToFile( const char * pMessageString )
{
	BW_GUARD;
	if (!s_automatedTest_)
	{
		return;
	}

	FILE * pLog = fopen( s_debugLogFile_, "a" );
	if (!pLog)
	{
		return;
	}

	time_t rawtime;
	time( &rawtime );

	fprintf( pLog, "%s\n", pMessageString );
	fprintf( stderr, "\n%s%s\n", ctime( &rawtime ), pMessageString );

	fclose( pLog );
}


/**
 *	This method sets the automated test status.
 *
 *	@param isTest A boolean indicating whether the application is running as
 *	              an automated test.
 *	@param pDebugLogPath A string indicating the path the log file should be
 *	                     written to.
 */
/*static*/ void LogMsg::automatedTest( bool isTest,
	const char * pDebugLogPath )
{
	s_automatedTest_ = isTest;

	if (NULL != pDebugLogPath)
	{
		sprintf( s_debugLogFile_, "%s\\" DEBUG_LOG_FILE, pDebugLogPath );
	}
}


/**
 *	This method indicates if the application is running as an automated test.
 *
 *	@return true if the application is an automated test, false otherwise.
 */
/*static*/ bool LogMsg::automatedTest()
{
	return s_automatedTest_;
}

#endif // _WIN32


#if defined( _XBOX360 )

/**
 *	This method generates an error message for xbox platforms.
 *
 *	@param stackTrace The string to display in the error message.
 */
void LogMsg::xbox360Stack( const char stackTrace[] )
{
	OutputDebugStringA( stackTrace );

	LPCWSTR buttons[] = { L"Exit" };
	XOVERLAPPED         overlapped; // Overlapped object for message box UI
	MESSAGEBOX_RESULT   result; // Message box button pressed result

	ZeroMemory( &overlapped, sizeof( XOVERLAPPED ) );

	const size_t dmhBufferSize = 16384;

	char tcbuffer[ dmhBufferSize * 2 ];
	WCHAR wcbuffer[ dmhBufferSize * 2 ];

	vsnprintf( tcbuffer, ARRAY_SIZE(tcbuffer), pFormat, argPtr );
	tcbuffer[sizeof( tcbuffer )-1] = '\0';

	MultiByteToWideChar( CP_UTF8, 0, tcbuffer, -1, wcbuffer,
		ARRAYSIZE( wcbuffer ) );

	DWORD dwRet = XShowMessageBoxUI( 0,
				L"Critical Error",			// Message box title
				wcbuffer,					// Message string
				ARRAYSIZE(buttons),			// Number of buttons
				buttons,					// Button captions
				0,							// Button that gets focus
				XMB_ERRORICON,				// Icon to display
				&result,					// Button pressed result
				&overlapped );

	while (!XHasOverlappedIoCompleted( &overlapped ))
	{
		extern IDirect3DDevice9 * g_pd3dDevice;
		g_pd3dDevice->Clear( 0L, NULL,
				D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
			D3DCOLOR_XRGB( 0,0,0 ), 1.0f, 0L );
		g_pd3dDevice->Present( 0, 0, NULL, 0 );
	}

	for (int i=0; i < 60; i++)
	{
		extern IDirect3DDevice9 * g_pd3dDevice;
		g_pd3dDevice->Clear( 0L, NULL,
			D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
			D3DCOLOR_XRGB( 0,0,0 ), 1.0f, 0L );
		g_pd3dDevice->Present( 0, 0, NULL, 0 );
	}

	XLaunchNewImage( "", 0 );

	//ENTER_DEBUGGER();
}
#endif // defined( _XBOX360 )



#if defined( __linux__ ) || defined( EMSCRIPTEN )

/**
 *	This method generates an assert.* file for Linux, then dumps core.
 *
 *	@param stackTrace A string containing the backtrace to dump to the assert
 *	                  file.
 *
 *	@return This method will never return.
 */
void LogMsg::linuxAssertAndAbort( const char stackTrace[] )
{
	char filename[512];
	char hostname[256];

	if (gethostname( hostname, sizeof( hostname ) ) != 0)
	{
		hostname[0] = 0;
	}

	char exeName[512];
	const char * pExeName = "unknown";

	int len = readlink( "/proc/self/exe", exeName, sizeof(exeName) - 1 );
	if (len > 0)
	{
		exeName[ len ] = '\0';

		char * pTemp = strrchr( exeName, '/' );
		if (pTemp != NULL)
		{
			pExeName = pTemp + 1;
		}
	}

	bw_snprintf( filename, sizeof( filename ), "assert.%s.%s.%d.log",
		pExeName, hostname, getpid() );

	FILE * assertFile = bw_fopen( filename, "a" );
	fprintf( assertFile, "%s", stackTrace );
	fclose( assertFile );

	volatile uint64 crashTime = timestamp(); // For reference in the coredump.
	crashTime = crashTime; // Disable compiler warning about unused variable.

	abort();
}
#endif // defined( __linux__ ) || defined( EMSCRIPTEN )


/**
 *	This method format the message of argPtr into pMsgBuffer
 *	
 *  @param pMsgBuffer		out, the buffer to save the formatted message.
 *	@param lenMsgBuffer		max size of lenMsgBuffer.
 *	@param pFormat			the format string to print.
 *	@param argPtr			the arguments of the message to be formatted.
 */
/*static*/ void LogMsg::formatMessage(
		char * /*out*/pMsgBuffer, size_t lenMsgBuffer,
		const char * pFormat, va_list argPtr )
{
	BW_GUARD;

	int size = bw_vsnprintf( pMsgBuffer, lenMsgBuffer, pFormat, argPtr );
	size = std::min< int >( static_cast< int >( lenMsgBuffer ) - 1, size );

	IF_NOT_MF_ASSERT_DEV( size >= 0 )
	{
		// Invalid format string, null terminate the buffer at the beginning
		if (lenMsgBuffer != 0)
		{
			pMsgBuffer[0] = 0;
		}

		return;
	}

	if ((size > 0) && (pMsgBuffer[ size - 1 ] == '\n'))
	{
		// If the message ends with a newline remove it
		pMsgBuffer[ size - 1 ] = 0;
	}
}


/**
 *	This method sets the status of whether or not logging messages should be
 *	directed to syslog.
 */
/*static*/ void LogMsg::shouldWriteToSyslog( bool state )
{
	BW_GUARD;
	g_shouldWriteToSyslog = state;
}


/**
 *	This method finalises the dependency chain for LogMsg.
 */
/*static*/ void LogMsg::fini()
{
	BW_GUARD;
	DebugFilter::fini();
}


#if ENABLE_MINI_DUMP
/**
 *
 */
/*static*/ void LogMsg::createMiniDump()
{
	HMODULE hMod = ::LoadLibrary( L"DbgHelp.dll" );

	if (!hMod)
	{
		LogMsg::logToFile( "Unable to create Minidump: "
			"Failed to load debughlp.dll" );
		return;
	}

	typedef BOOL (WINAPI * MiniDumpWriteDump_t)( HANDLE hProcess,
		DWORD ProcessId,
		HANDLE hFile,
		MINIDUMP_TYPE DumpType,
		PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
		PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
		PMINIDUMP_CALLBACK_INFORMATION CallbackParam );

	MiniDumpWriteDump_t pMiniDumpWriteDump = 
		(MiniDumpWriteDump_t)GetProcAddress( hMod, "MiniDumpWriteDump" );

	if (!pMiniDumpWriteDump)
	{
		LogMsg::logToFile( "Unable to create Minidump: "
			"Failed to get MiniDumpWriteDump" );
		return;
	}

	time_t now = time( NULL );
	tm* t = localtime( &now );
	wchar_t filename[BW_MAX_PATH];
	bw_snwprintf( filename, sizeof(filename),
		L"%S_%04d%02d%02d_%02d%02d%02d.dmp",
		BWUtil::executableBasename().c_str(),
		1900+t->tm_year, t->tm_mon+1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec );
	HANDLE hFile = CreateFile ( filename,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL );

	if (!hFile)
	{
		LogMsg::logToFile( "Unable to create Minidump: "
			"Failed to open file" );
		return;
	}

	if (pMiniDumpWriteDump( GetCurrentProcess(),
		GetCurrentProcessId(),
		hFile,
		(MINIDUMP_TYPE)(MiniDumpWithFullMemory |
			MiniDumpIgnoreInaccessibleMemory),
		NULL,
		NULL,
		NULL ))
	{
		char outputMessage[1024];
		bw_snprintf( outputMessage, ARRAY_SIZE(outputMessage),
			"Successfully created Minidump to %S\n",
			filename );
		LogMsg::logToFile( outputMessage );

	}
	CloseHandle( hFile );
}
#endif // ENABLE_MINI_DUMP


/**
 *
 */
void CriticalMsg::writeAndAssert( const char * pFormat, ... )
{
	BW_GUARD;

	va_list argPtr;
	va_start( argPtr, pFormat );

	inWrite_ = true;
	this->criticalMessageHelper( true, pFormat, argPtr );
	inWrite_ = false;

	va_end( argPtr );

}


BW_END_NAMESPACE

#undef BW_PLATFORM_SUPPORTS_SYSLOG
#undef BW_UNIX_BACKTRACE

// log_msg.cpp
