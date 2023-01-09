#include "pch.hpp"

#ifdef _XBOX

#include <xtl.h>

#elif defined( _WIN32 )

#include "cstdmf/cstdmf_windows.hpp"

#endif

#include "config.hpp"

#if ENABLE_DPRINTF

#include "dprintf.hpp"

#include "debug_filter.hpp"
#include "string_builder.hpp"
#include "string_utils.hpp"

#include <stdio.h>
#include <string.h>
#include <time.h>

#if !defined( _WIN32 )
#include <sys/time.h>
#endif // !defined( _WIN32 )


BW_BEGIN_NAMESPACE

/**
 *	This function prints a debug message
 */
void dprintf( const char * format, ... )
{
	va_list argPtr;
	va_start( argPtr, format );

	vdprintf( format, argPtr );

	va_end( argPtr );
}


/**
 *	This function prints a debug message
 */
void dprintf( DebugMessagePriority messagePriority, const char * category,
	const char * format, ... )
{
	va_list argPtr;
	va_start( argPtr, format );

	vdprintf( format, argPtr, messagePriority, category );

	va_end( argPtr );
}

#if !defined( _WIN32 )

size_t convertTime( char *buf, size_t size, const timeval *time )
{
	size_t result = strftime( buf, size, "%F %T", localtime( &time->tv_sec ) );

	if (result != 0)
	{
		result += bw_snprintf( buf + result, size - result,
			".%03ld", time->tv_usec / 1000 );
	}
	return result;
}

/*
 *	This function prints a debug message
 */
void vdprintf( const char * format, va_list argPtr, const char * pPriorityName,
	const char * pCategory )
{
	if (!DebugFilter::shouldWriteToConsole())
	{
		return;
	}

	if (DebugFilter::shouldWriteTimePrefix())
	{
		timeval now;
		gettimeofday( &now, NULL );
		char timebuff[32];

		if (convertTime( timebuff, sizeof( timebuff ), &now ) != 0)
		{
			fprintf( DebugFilter::consoleOutputFile(), "%s: ", timebuff );
		}
	}

	if (pPriorityName != NULL)
	{
		fprintf( DebugFilter::consoleOutputFile(), "%s: ", pPriorityName );
	}

	if (pCategory != NULL && strlen( pCategory ) != 0)
	{
		fprintf( DebugFilter::consoleOutputFile(), "[%s] ", pCategory );
	}

	// Need to make a copy of the va_list here to avoid crashing on 64bit
	va_list tmpArgPtr;
	bw_va_copy( tmpArgPtr, argPtr );
	vfprintf( DebugFilter::consoleOutputFile(), format, tmpArgPtr );
	va_end( tmpArgPtr );
}


/**
 *	This function prints a debug message
 */
void vdprintf( const char * format, va_list argPtr,
	DebugMessagePriority messagePriority,
	const char * pCategory )
{
	vdprintf( format, argPtr, messagePrefix( messagePriority ), pCategory );
}
#else // !defined( _WIN32 )

/**
 *	This function prints a debug message, with a specified priorityname, & a specified output io file
 *		It is called by the other vdprintf functions.
 */
void vdprintf( const char * format, va_list argPtr, const char * pPriorityName,
			  const char * pCategory, FILE * pOutFile )
{
	static const size_t bufSize = 4096;
	char buf[ bufSize ];

	BW::StringBuilder output( buf, bufSize );

	if (pPriorityName != NULL)
	{
		output.appendf( "%s: ", pPriorityName );

		if ( DebugFilter::shouldWriteToConsole() )
		{
			fprintf( pOutFile, "%s: ", pPriorityName );
		}
	}

	if (pCategory != NULL && strlen( pCategory ) != 0)
	{
		output.appendf( "[%s] ", pCategory );

		if ( DebugFilter::shouldWriteToConsole() )
		{
			fprintf( pOutFile, "[%s] ", pCategory );
		}
	}

	output.vappendf( format, argPtr );

	OutputDebugStringA( buf );

	if ( DebugFilter::shouldWriteToConsole() )
	{
		vfprintf( pOutFile, format, argPtr );
	}
}

/**
 *	This function prints a debug message, with no debugmessagePriority
 *		Since we have no debugmessagepriority, we send the output to the DebugFilter::consoleOutputFIle()
 *		which by default is stderr.
 */
void vdprintf( const char * format, va_list argPtr, const char * pPriorityName,
			  const char * pCategory )
{
	FILE * outputFile = DebugFilter::consoleOutputFile();
	vdprintf( format, argPtr, pPriorityName, pCategory, outputFile );
}


/**
 *	This function prints a debug message using the debugMessagePriority name & sends it to the io defined by the 
 *		debugMessagePriority.
 */
void vdprintf( const char * format, va_list argPtr,
	DebugMessagePriority messagePriority,
	const char * pCategory )
{
	const char * pPriorityName = messagePrefix( messagePriority );
	FILE * pOutFile = DebugMessageOutput( messagePriority );
	
	vdprintf( format, argPtr, pPriorityName, pCategory, pOutFile );
}

#endif // !defined( _WIN32 )


BW_END_NAMESPACE

#endif // ENABLE_DPRINTF

// dprintf.cpp
