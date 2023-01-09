// network_utils.cpp
#include "pch.hpp"

#if defined( _WIN32 )
#include <WinSock2.h>
#else // defined( _WIN32 )
#include <errno.h>
#include <string.h>
#endif // defined( _WIN32 )

#include "cstdmf/debug.hpp"

#include "network/misc.hpp"
#include "network/basictypes.hpp"
#include "network_utils.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This method returns whether an errno (or WinSock error) value should be
 *	ignored.
 *
 *	@return true if the error can be ignored, false otherwise.
 */
bool isErrnoIgnorable()
{
#if defined( _WIN32 )
	int err = WSAGetLastError();
	return (err == WSAEWOULDBLOCK) ||
			(err == WSAEINPROGRESS) ||
			(err == WSAEINTR);
#else // defined( _WIN32 )
	return (errno == EAGAIN) || (errno == EWOULDBLOCK) || 
		(errno == EINPROGRESS) || (errno == EINTR);
#endif // defined( _WIN32 )
}


/**
 *	This method returns a string representation of the last error status
 *	encountered (either with errno or WinSock depending on the platform).
 *
 *	@return An error description string on success, NULL if unable to create a
 *		string representation of the current error status.
 */
const char * lastNetworkError()
{
	BW::string retError;
#if defined( _WIN32 )
	int errCode = WSAGetLastError();
	return networkErrorMessage( errCode );
#else
	return strerror( errno );
#endif
}


/**
 *	This function returns the appropriate human-readable message for the
 *	given network error number.
 */
const char * networkErrorMessage( int error )
{
#if defined( _WIN32 )
	static char errString[512];

	int size = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM,
		0, error, 0, errString, sizeof(errString), 0 );

	if (size == 0)
	{
		bw_snprintf( errString, sizeof( errString ),
			"error %d (FormatMessageA failed)", error );
	}

	return errString;
#else
	return strerror( error );
#endif
}

/**
 *	This method returns the kernel settings for the relevant maximum buffer size
 *  obtained by reading the corresponding file content under /proc/sys/net/core/.
 * 
 *	@param isReadBuffer  boolean to indicate whether the function should check
 *	                   	 for read or write buffer sizes.
 *
 *	@returns The value of the requested socket buffer. 
 */
int getMaxBufferSize( bool isReadBuffer )
{
	int bufSize = isReadBuffer ? MIN_RCV_SKT_BUF_SIZE : MIN_SND_SKT_BUF_SIZE;
	
#if defined( __linux__ ) && !defined( EMSCRIPTEN )
	const char * filePath = NULL;
	FILE *file = NULL;
	
	if (isReadBuffer)
	{
		filePath = "/proc/sys/net/core/rmem_max";
	}
	else
	{
		filePath = "/proc/sys/net/core/wmem_max";
	}

	file = fopen( filePath, "r" );	
	if (file == NULL)
	{
		WARNING_MSG( "Unable to read buffer size from: %s, error:%s\n",
			filePath, strerror( errno ) );
	}
	else
	{
		fscanf( file, "%d", &bufSize );
		fclose( file );
	}
#endif

	return bufSize;
}

BW_END_NAMESPACE

// network_utils.cpp
