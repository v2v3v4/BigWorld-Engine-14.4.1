/*
 * This file contains malloc / realloc / free replacements to be used for
 * integrating into Python. This code was originally included as a standard
 * Python modification with BigWorld, however has been removed to clean
 * up the default Python implementation. The code is retained here for
 * re-use by customers if the need arises.
 */

// -----------------------------------------------------------------------------
// Section: Python core (i.e. NOT PyObjectPlus) memory allocation tracking
// -----------------------------------------------------------------------------
#include "network/basictypes.hpp"

#include <stdlib.h>

extern "C"
{
void * log_malloc( size_t n );
void * log_realloc( void * m, size_t n );
void log_free( void * m );
};


/**
 *	Logging interceptor for python 'malloc' calls
 */
void * log_malloc( unsigned int n )
{
#if defined( _DEBUG ) && defined( _WIN32 )
	if (n >= 1024*1024)
	{
		// python allocating more than 1 MB!
		ENTER_DEBUGGER()
	}
#endif

	uint16 * m = (uint16*)bw_malloc( n );
	return m;
}


/**
 *	Logging interceptor for python 'realloc' calls
 */
void * log_realloc( void * m, size_t n )
{
#if defined( _DEBUG ) && defined( _WIN32 )
	if (n >= 1024*1024)
	{
		// python allocating more than 1 MB!
		ENTER_DEBUGGER()
	}
#endif

	m = bw_realloc( m, n );
	return m;
}


/**
 *	Logging interceptor for python 'free' calls
 */
void log_free( void * m )
{
	bw_free( m );
}

// py_memory_log.cpp
