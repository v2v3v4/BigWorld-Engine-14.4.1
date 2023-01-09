#include "plugin_library.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_set.hpp"


#ifdef __unix__

#include <errno.h>

#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#include <string.h>

#endif

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PluginLibrary
// -----------------------------------------------------------------------------

typedef BW::set< BW::string > StringSet;

/**
 *	This function loads all plugins from the given directory name (optionally
 *	prefixed with the executing app's name) and runs their static initialisers.
 */
bool PluginLibrary::loadAllFromDirRelativeToApp(
	bool prefixWithAppName, const char * partialDir )
{
#if defined( __unix__ )
	char rlbuf[ 512 ];
	int rllen = readlink( "/proc/self/exe", rlbuf, sizeof( rlbuf ) );

	if ((rllen <= 0) || (rlbuf[ 0 ] != '/'))
	{
		ERROR_MSG( "PluginLibrary: "
			"Could not read symbolic link /proc/self/exe\n" );

		return false;
	}

	// Note: readlink does not append a NUL character.
	BW::string dllDirName = BW::string( rlbuf, rllen );

	if (!prefixWithAppName)
	{
		dllDirName = dllDirName.substr( 0, dllDirName.find_last_of( '/' ) );
	}

	dllDirName = dllDirName + partialDir + "/";

	DIR * dllDir = opendir( dllDirName.c_str() );

	if (dllDir == NULL)
	{
		// Only return success if the directory doesn't exist
		bool isOkay = (errno == ENOENT);

		if (!isOkay)
		{
			ERROR_MSG( "PluginLibrary: Failed to open plugin directory "
				"'%s': %s\n", dllDirName.c_str(), strerror( errno ) );
		}

		return isOkay;
	}

	StringSet dllNames;

	struct dirent * pEntry;

	while ((pEntry = readdir( dllDir )) != NULL)
	{
		BW::string dllName = pEntry->d_name;

		if ((dllName.length() < 3) ||
			(dllName.substr( dllName.length()-3 ) != ".so"))
		{
			continue;
		}

		if (dllName[0] != '/')
		{
			dllName = dllDirName + dllName;
		}

		dllNames.insert( dllName );
	}

	bool isOkay = true;

	for (StringSet::iterator it = dllNames.begin(); it != dllNames.end(); it++)
	{
		const BW::string & dllName = *it;

		TRACE_MSG( "PluginLibrary: Loading extension %s\n", dllName.c_str() );

		void * handle = dlopen( dllName.c_str(), RTLD_LAZY | RTLD_GLOBAL );

		if (handle == NULL)
		{
			ERROR_MSG( "PluginLibrary: "
				"Could not open library: %s\n"
				"PluginLibrary: reason was: %s\n", dllName.c_str(), dlerror() );
			isOkay = false;
		}
	}

	TRACE_MSG( "PluginLibrary: Finished loading extensions\n" );

	closedir( dllDir );

	return isOkay;

#elif defined( _WIN32 )

	char moduleName[ 512 ];

	GetModuleFileName( NULL, moduleName, sizeof( moduleName ) );
	BW::string dllDirName = BW::string( moduleName, strlen( moduleName ) - 4 );

	if (!prefixWithAppName)
	{
		dllDirName = dllDirName.substr( 0, dllDirName.find_last_of( '\\' ) );
	}

	dllDirName = dllDirName + partialDir + "\\*";

	WIN32_FIND_DATA findData;
	HANDLE firstFileHandle = FindFirstFile( dllDirName.c_str(), &findData );

	if (firstFileHandle == INVALID_HANDLE_VALUE)
	{
		ERROR_MSG( "PluginLibrary: Failed to open plugin directory '%s'\n",
			dllDirName.c_str() );
		return false;
	}

	StringSet dllNames;

	do
	{
		BW::string dllName = findData.cFileName;

		if ((dllName.length() < 4) ||
			(dllName.substr( dllName.length()-4 ) != ".dle"))
		{
			continue;
		}

		dllName = dllDirName.substr( 0, dllDirName.length() - 1 ) + dllName;
		dllNames.insert( dllName );

	} while (FindNextFile( firstFileHandle, &findData ));

	FindClose( firstFileHandle );

	bool isOkay = true;

	for (StringSet::iterator it = dllNames.begin(); it != dllNames.end(); it++)
	{
		const BW::string & dllName = *it;

		TRACE_MSG( "PluginLibrary: Loading extension %s\n", dllName.c_str() );

		HMODULE handle = LoadLibraryEx( dllName.c_str(), NULL,
			LOAD_WITH_ALTERED_SEARCH_PATH );

		if (handle == NULL)
		{
			ERROR_MSG( "PluginLibrary: "
				"Could not open library: %s\n", dllName.c_str() );
			isOkay = false;
		}
	}

	return isOkay;

#else

#error "No PluginLibrary defined for this platform."

#endif // __unix__
}

BW_END_NAMESPACE

// plugin_library.cpp
