#ifndef DPRINTF_HPP
#define DPRINTF_HPP

#include "config.hpp"
#include "bw_namespace.hpp"
#include "debug_message_priority.hpp"

#include <stdarg.h>
#include <stddef.h>

#if ENABLE_DPRINTF
# ifndef _WIN32
struct timeval;
# endif // _WIN32
#endif // ENABLE_DPRINTF

BW_BEGIN_NAMESPACE

#if ENABLE_DPRINTF

// These functions print a debug messages as safely as possible, so should be
// safely callable irrespective of threading or heap management/instrumentation
# ifndef _WIN32
	size_t convertTime( char *buf, size_t size, const timeval *time );

	CSTDMF_DLL void dprintf( const char * format, ... )
		__attribute__ ( (format (printf, 1, 2 ) ) );
	CSTDMF_DLL void dprintf( DebugMessagePriority messagePriority, const char * category,
			const char * format, ...)
		__attribute__ ( (format (printf, 3, 4 ) ) );

# else // _WIN32

	CSTDMF_DLL void dprintf( const char * format, ... );
	CSTDMF_DLL void dprintf( DebugMessagePriority messagePriority, const char * category,
			const char * format, ...);

# endif // _WIN32

	CSTDMF_DLL void vdprintf( const char * format, va_list argPtr, const char * pPriorityName,
		const char * pCategory, FILE * pFile );
	CSTDMF_DLL void vdprintf( const char * format, va_list argPtr,
			const char * pPriorityName = NULL, const char * pCategory = NULL );
	CSTDMF_DLL void vdprintf( const char * format, va_list argPtr,
			DebugMessagePriority messagePriority,
			const char * pCategory = NULL );

#else // ENABLE_DPRINTF

	inline void dprintf( const char * format, ... ) {}
	inline void dprintf( DebugMessagePriority messagePriority,
			const char * category, const char * format, ...) {}

	inline void vdprintf( const char * format, va_list argPtr,
			const char * pPriorityName = NULL, const char * pCategory = NULL )
			{}
	inline void vdprintf( const char * format, va_list argPtr,
			DebugMessagePriority messagePriority,
			const char * pCategory = NULL ) {}

#endif // ENABLE_DPRINTF

BW_END_NAMESPACE

#endif // DPRINTF_HPP
