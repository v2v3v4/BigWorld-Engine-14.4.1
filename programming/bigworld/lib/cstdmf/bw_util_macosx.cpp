#include "bw_util.hpp"

#include "debug.hpp"
#include "guard.hpp"

#include <mach-o/dyld.h>

#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE


namespace BWUtil
{


void sanitisePath( BW::string & path )
{
	// This method is a NOP for Mac OS X as BigWorld uses the same format.
}


BW::string getFilePath( const BW::StringRef & pathToFile )
{
	// dirname() can modify the input string so we need to make a copy
	size_t length = std::min( static_cast< size_t >( PATH_MAX ),
						pathToFile.length() );
	char pathCpy[PATH_MAX + 1];
	memcpy( pathCpy, pathToFile.data(), length );
	pathCpy[length] = '\0';

	return dirname( pathCpy );
}


bool getExecutablePath( char * pathBuffer, size_t pathBufferSize )
{
	uint32_t size = pathBufferSize;
	memset( pathBuffer, '\0', pathBufferSize );

	int retval = _NSGetExecutablePath( pathBuffer, &size );

	if (retval < 0)
	{
		ERROR_MSG( "BWUtil::getExecutablePath: Buffer overflow "
				"(require %zu bytes, got %zu)\n",
			static_cast<size_t>( size ), pathBufferSize );
		return false;
	}

	char realPath[PATH_MAX];
	if (!realpath( pathBuffer, realPath ))
	{
		ERROR_MSG( "BWUtil::getExecutablePath: "
				"Failed to get the real path: %s\n",
			strerror( errno ) );
		return false;
	}

	size_t realPathSize = strlen( realPath );
	if ((realPathSize + 1) > pathBufferSize)
	{
		ERROR_MSG( "BWUtil::getExecutablePath: Buffer overflow "
				"(require %zu bytes, got %zu)\n",
			realPathSize, pathBufferSize );
		return false;

	}

	memcpy( pathBuffer, realPath, realPathSize + 1 );
	return true;
}


} // namespace BWUtil


/**
 *	This function returns a temp file name.
 */
BW::string getTempFilePathName()
{
	BW_GUARD;
	char fileName[] = P_tmpdir "/BWTXXXXXX";

	int fd = mkstemp( fileName );
	if (fd < 0) 
	{
		ERROR_MSG( "BWUtil::getTempFilePathName: Unable create temporary file "
			"in %s\n", P_tmpdir );
		return BW::string();
	}

	close( fd );
	return BW::string( fileName );
}


BW_END_NAMESPACE


// bw_util_macosx.cpp
