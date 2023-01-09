#ifndef BW_LOG_MSG_HPP
#define BW_LOG_MSG_HPP

#include "bw_namespace.hpp"
#include "cstdmf_dll.hpp"
#include "debug_message_priority.hpp"
#include "debug_message_source.hpp"
#include "log_meta_data.hpp"

#include <stdarg.h>

#if defined( __GNUC__ ) || defined( __clang__ )
#define BW_FORMAT_ATTRIBUTE __attribute__ ( (format (printf, 2, 3 ) ) )
#else
#define BW_FORMAT_ATTRIBUTE
#endif

#define BW_UNIX_BACKTRACE (defined( unix ) && !defined( __ANDROID__ ))


BW_BEGIN_NAMESPACE

class StringBuilder;

class LogMsg
{
public:
	CSTDMF_DLL LogMsg( DebugMessagePriority priority,
		const char * pCategory = NULL );

	/*
	 * Category handling
	 */
	CSTDMF_DLL LogMsg & category( const char * pCategory );
	CSTDMF_DLL LogMsg & operator[]( const char * pCategory );


	/*
	 * Source handling
	 */
	CSTDMF_DLL LogMsg & source( DebugMessageSource source );


	/*
	 * Metadata handling
	 */
	template< class VALUE_TYPE >
	LogMsg & meta( const char * pKey, const VALUE_TYPE & value )
	{
		metaData_.add( pKey, value );
		return *this;
	}

	CSTDMF_DLL LogMsg & metaAsJSON( const char * pMetaData );

	/*
	 * Message finalisation
	 */
	CSTDMF_DLL void write( const char * pFormat, ... )
		BW_FORMAT_ATTRIBUTE;
	CSTDMF_DLL void operator()( const char * pFormat, ... )
		BW_FORMAT_ATTRIBUTE;

	CSTDMF_DLL void writeBackTrace();

	/*
	 * Static methods
	 */
	CSTDMF_DLL static void fini();

	CSTDMF_DLL static void formatMessage(
			char * /*out*/pMsgBuffer, size_t lenMsgBuffer,
			const char * pFormat, va_list argPtr );

	// TODO: as discussed with JamesM this functionality should probably be
	//       moved into a debug filter
	static void shouldWriteToSyslog( bool state = true );


#if defined( _WIN32 )
	// TODO: this hardly seems like the most appropriate place for these
	//       methods or state. Only appears to bw utilised in bw_winmain.cpp
	static void hasSeenCriticalMsg( bool criticalMsgOccurred )
	{
		s_hasSeenCriticalMsg_ = criticalMsgOccurred;
	}

	static bool hasSeenCriticalMsg()
	{
		return s_hasSeenCriticalMsg_;
	}

	CSTDMF_DLL static void logToFile( const char * pLine );
	CSTDMF_DLL static void automatedTest( bool isTest,
		const char * pDebugLogPath = NULL );
	CSTDMF_DLL static bool automatedTest();

# if ENABLE_MINI_DUMP
	CSTDMF_DLL static void createMiniDump();
# endif // ENABLE_MINI_DUMP

#endif // defined( _WIN32 )

protected:
	void criticalMessageHelper( bool isDevAssertion,
		const char * pFormat, va_list argPtr );

#if BW_UNIX_BACKTRACE
	CSTDMF_DLL void messageBackTrace( StringBuilder * pBuilder,
		bool shouldAddToMetaData );
#endif // BW_UNIX_BACKTRACE

	/*
	 *	Indicates if the write helper methods are already active to avoid
	 *	re-entrant calls to write from blowing up, mostly from within
	 *	criticalMessageHelper.
	 */
	bool inWrite_;

private:

	void doWrite( const char * pFormat, va_list argPtr );
	void writeHelper( const char * pFormat, ... ) BW_FORMAT_ATTRIBUTE;

	/**
	 *	This method determines whether an invocation of a LogMsg write should
	 *	cause a critical message to be handled by the assertion code or by
	 *	regular message handling code.
	 *
	 *	@return true if the message should be critical, false otherwise.
	 */
	bool shouldInvokeCriticalHandler() const
	{
		return ((priority_ == MESSAGE_PRIORITY_CRITICAL) &&
			(source_ != MESSAGE_SOURCE_SCRIPT));
	}


	/*
	 *	Critical message helper methods
	 */
#if defined( __linux__ ) || defined( EMSCRIPTEN )
	void linuxAssertAndAbort( const char stackTrace[] );
#endif // defined( __linux__ ) || defined( EMSCRIPTEN )

	void generateDevAssertMsg( StringBuilder & strBuilder,
		const char * pFormat, va_list argPtr );

#if defined( _WIN32 )
	void win32GenerateCriticalMsg( StringBuilder & strBuilder,
		const char * pFormat, va_list argPtr );

	void win32StackTraceThreads( StringBuilder & builder );
	void win32DumpAndAbort( char buffer[] );

	static bool s_automatedTest_;
	static char s_debugLogFile_[1024];
	static bool s_hasSeenCriticalMsg_;
#endif // defined( _WIN32 )

#if defined( _XBOX360 )
	void xbox360Stack( const char stackTrace[] );
#endif // defined( _XBOX360 )

	DebugMessagePriority priority_;
	DebugMessageSource source_;
	const char * pCategory_;
	LogMetaData metaData_; 
};


#define BW_CREATE_PRIORITY_MSG_HANDLER( NAME, PRIORITY )	\
class NAME : public LogMsg									\
{															\
public:														\
	NAME( const char * pCategory = NULL ) :					\
		LogMsg( PRIORITY, pCategory )						\
	{ }														\
};


BW_CREATE_PRIORITY_MSG_HANDLER( TraceMsg, MESSAGE_PRIORITY_TRACE )
BW_CREATE_PRIORITY_MSG_HANDLER( DebugMsg, MESSAGE_PRIORITY_DEBUG )
BW_CREATE_PRIORITY_MSG_HANDLER( InfoMsg, MESSAGE_PRIORITY_INFO )
BW_CREATE_PRIORITY_MSG_HANDLER( NoticeMsg, MESSAGE_PRIORITY_NOTICE )
BW_CREATE_PRIORITY_MSG_HANDLER( ErrorMsg, MESSAGE_PRIORITY_ERROR )
BW_CREATE_PRIORITY_MSG_HANDLER( WarningMsg, MESSAGE_PRIORITY_WARNING )
// CriticalMsg is defined below in order to define an extra method.
//BW_CREATE_PRIORITY_MSG_HANDLER( CriticalMsg, MESSAGE_PRIORITY_CRITICAL )
BW_CREATE_PRIORITY_MSG_HANDLER( HackMsg, MESSAGE_PRIORITY_HACK )
BW_CREATE_PRIORITY_MSG_HANDLER( AssetMsg, MESSAGE_PRIORITY_ASSET )


class CriticalMsg : public LogMsg
{
public:
	CriticalMsg( const char * pCategory = NULL ) :
		LogMsg( MESSAGE_PRIORITY_CRITICAL, pCategory )
	{ }

	CSTDMF_DLL void writeAndAssert( const char * pFormat, ... )
		BW_FORMAT_ATTRIBUTE;
};

BW_END_NAMESPACE

#endif // BW_LOG_MSG_HPP
