#include "pch.hpp"

#include "bw_platform_info.hpp"

#include "debug.hpp"
#include "stdmf.hpp"

#include <sys/stat.h>

#if !defined( _WIN32 ) && !defined( _WIN64 )
#include <unistd.h>
#endif

#include <errno.h>
#include <string.h>


BW_BEGIN_NAMESPACE

namespace { // anonymous

const BW::string REDHAT_VERSION_FILE = "/etc/redhat-release";

const BW::string SHORT_NAME_ENTERPRISE_LINUX = "el";
const BW::string SHORT_NAME_CENTOS = "centos";
const BW::string SHORT_NAME_FEDORA = "fedora";
const BW::string SHORT_NAME_RHEL = "rhel";
const BW::string SHORT_NAME_MACOSX = "macosx";
const BW::string SHORT_NAME_WIN32 = "win32";
const BW::string SHORT_NAME_WIN64 = "win64";
const BW::string SHORT_NAME_UNKNOWN = "unknown";

}; // anonymous

/**
 *	Parse a version file on a known RedHat / CentOS machine.
 *
 *	'CentOS release 5.9 (Final)'
 *	'CentOS release 6.4 (Final)'
 *
 *	'Red Hat Enterprise Linux Server release 5.2 (Tikanga)'
 *	'Red Hat Enterprise Linux Server release 6.3 (Santiago)'
 *
 *	'Fedora release 17 (Beefy Miracle)'
 *
 *	CentOS 6+ have /etc/centos-release
 */
bool PlatformInfo::parseRedHatRelease( BW::string & shortPlatformName )
{
	FILE *fp = fopen( REDHAT_VERSION_FILE.c_str(), "r" );

	if (fp == NULL)
	{
		ERROR_MSG( "PlatformInfo::parseRedHatRelease: "
				"Unable to open existing file '%s': %s\n",
			REDHAT_VERSION_FILE.c_str(), strerror( errno ) );

		return false;
	}

	char versionLine[ BUFSIZ ];

	if (fgets( versionLine, sizeof( versionLine ), fp ) == NULL)
	{
		ERROR_MSG( "PlatformInfo::parseRedHatRelease: "
				"Failed to read line from '%s': %s\n",
			REDHAT_VERSION_FILE.c_str(), strerror( errno ) );

		return false;
	}

	fclose( fp );


	BW::string versionLineStr( versionLine );

	if (versionLineStr.find( "CentOS" ) != BW::string::npos)
	{
		shortPlatformName = SHORT_NAME_CENTOS;
	}
	else if (versionLineStr.find( "Red Hat" ) != BW::string::npos)
	{
		shortPlatformName = SHORT_NAME_RHEL;
	}
	else if (versionLineStr.find( "Fedora" ) != BW::string::npos)
	{
		shortPlatformName = SHORT_NAME_FEDORA;
	}
	else
	{
		ERROR_MSG( "PlatformInfo::parseRedHatRelease: "
			"Unknown RedHat distribution: %s\n", versionLine );
		return false;
	}

	size_t foundPos = versionLineStr.find( " release " );
	const BW::string unknownVersionSuffix = "-unknown";

	if (foundPos == BW::string::npos)
	{
		WARNING_MSG( "PlatformInfo::parseRedHatRelease: "
				"Unable to determine RedHat release information: %s\n",
			versionLine );
		shortPlatformName += unknownVersionSuffix;
		return true;
	}

	int versionNumber;

	if (sscanf( versionLine + foundPos, " release %d", &versionNumber ) != 1)
	{
		ERROR_MSG( "PlatformInfo::parseRedHatRelease: "
			"Unable to determine version number in: %s\n", versionLine );

		shortPlatformName += unknownVersionSuffix;
		return true;
	}

	char versionNumBuf[ 8 ];
	bw_snprintf( versionNumBuf, sizeof( versionNumBuf ), "%d", versionNumber );
	shortPlatformName += versionNumBuf;

	return true;
}


/**
 *	Return a string representation of the platform name to be used for locating
 *	server binaries. This may be different the Operating System distribution
 *	as we condense RHEL / CentOS to 'el' to provide easier cross platform
 *	support.
 */
const BW::string & PlatformInfo::buildStr()
{
#if defined( _WIN64 )
	return SHORT_NAME_WIN64;
#elif defined( _WIN32 )
	return SHORT_NAME_WIN32;
#elif defined( __APPLE__ )
	return SHORT_NAME_MACOSX;
#else
	static BW::string s_cachedBuildPlatformName;

	if (!s_cachedBuildPlatformName.empty())
	{
		return s_cachedBuildPlatformName;
	}

	s_cachedBuildPlatformName = PlatformInfo::str();

	// Replace leading SHORT_NAME_CENTOS or SHORT_NAME_RHEL with
	// SHORT_NAME_ENTERPRISE_LINUX
	if (s_cachedBuildPlatformName.find( SHORT_NAME_CENTOS ) == 0)
	{
		s_cachedBuildPlatformName.replace( 0, SHORT_NAME_CENTOS.size(),
			SHORT_NAME_ENTERPRISE_LINUX );
	}
	else if (s_cachedBuildPlatformName.find( SHORT_NAME_RHEL ) == 0)
	{
		s_cachedBuildPlatformName.replace( 0, SHORT_NAME_RHEL.size(),
			SHORT_NAME_ENTERPRISE_LINUX );
	}

	return s_cachedBuildPlatformName;
#endif
}


/**
 *	Return a string representation of the Operating System platform name.
 *
 *	This name is distinct for each version of the same operating system. For
 *	example, centos5, centos6, win32, win64.
 */
const BW::string & PlatformInfo::str()
{
#if defined( _WIN64 )
	return SHORT_NAME_WIN64;
#elif defined( _WIN32 )
	return SHORT_NAME_WIN32;
#elif defined( __APPLE__ )
	return SHORT_NAME_MACOSX;
#else
	// Linux platforms

	static BW::string s_cachedPlatformName;

	if (!s_cachedPlatformName.empty())
	{
		return s_cachedPlatformName;
	}

	struct stat statBuf;
	if (stat( REDHAT_VERSION_FILE.c_str(), &statBuf ) == 0)
	{
		if (!PlatformInfo::parseRedHatRelease( s_cachedPlatformName ))
		{
			s_cachedPlatformName.clear();
		}
	}

	if (s_cachedPlatformName.empty())
	{
		s_cachedPlatformName = SHORT_NAME_UNKNOWN;
	}

	return s_cachedPlatformName;
#endif
}


BW_END_NAMESPACE


// bw_platform_info.cpp
