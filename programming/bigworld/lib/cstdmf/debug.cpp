#include "pch.hpp"

#include "cstdmf/string_utils.hpp"

#include "bw_util.hpp"
#include "concurrency.hpp"
#include "debug.hpp"
#include "dprintf.hpp"

#include "timestamp.hpp"

#include <string.h>

#if !defined( _XBOX360 ) && !defined( _WIN32 )
#include <unistd.h>
#endif

DECLARE_DEBUG_COMPONENT2( "CStdMF", 0 );

BW_BEGIN_NAMESPACE

BW::string g_syslogAppName;


//-------------------------------------------------------
//	Section: MainThreadTracker
//-------------------------------------------------------

/**
 *	This static thread-local variable is initialised to false, and set to true
 *	in the constructor of the static s_mainThreadTracker object below
 */
static THREADLOCAL( bool ) s_isCurrentMainThread_( false );


/**
 *	Constructor
 */
MainThreadTracker::MainThreadTracker()
{
	s_isCurrentMainThread_ = true;
}


/**
 *	Static method that returns true if the current thread is the main thread,
 *  false otherwise.
 *
 *	@returns      true if the current thread is the main thread, false if not
 */
/* static */ bool MainThreadTracker::isCurrentThreadMain()
{
	return s_isCurrentMainThread_;
}


// Instantiate it, so it initialises the flag to the main thread
static MainThreadTracker s_mainThreadTracker;


//-------------------------------------------------------
//	Section: debug funtions
//-------------------------------------------------------

LogMsg & addMessageMetadataForScope( LogMsg & msg )
{
	return msg;
}


LogMsg & constWrapper_addMessageMetadataForScope( const LogMsg & msg )
{
	return addMessageMetadataForScope( const_cast< LogMsg & >( msg ) );
}


/**
 *	This function is used to strip the path and return just the basename from a
 *	path string.
 */
const char * bw_debugMunge( const char * path, const char * module )
{
	static char	staticSpace[128];

	const char * pResult = path;

	const char * pSeparator;

	pSeparator = strrchr( pResult, '\\' );
	if (pSeparator != NULL)
	{
		pResult = pSeparator + 1;
	}

	pSeparator = strrchr( pResult, '/' );
	if (pSeparator != NULL)
	{
		pResult = pSeparator + 1;
	}

	strcpy( staticSpace, "logger/cppThresholds/" );

	if ((module != NULL) && (module[0] != '\0'))
	{
		strcat( staticSpace, module );
		strcat( staticSpace, "/" );
	}

	strcat(staticSpace,pResult);
	return staticSpace;
}



char __scratch[] = "DebugLibTestString Tue Nov 29 11:54:35 EST 2005";


BW_END_NAMESPACE

// debug.cpp
