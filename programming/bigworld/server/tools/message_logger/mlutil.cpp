#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "cstdmf/debug.hpp"
#include "server/config_reader.hpp"

#include "mlutil.hpp"


BW_BEGIN_NAMESPACE

namespace MLUtil
{


/**
 * Determines the configuration file to use when non-provided.
 *
 * When no configuration file has been provided, this method
 * uses the /etc/bigworld.conf file to locate the tools directory
 * and then finds the message_logger.conf from there.
 *
 * @param configFile  The string where the location of the configuration
 *                    file to use is stored.
 *
 * @returns true on success, false on failure.
 */
bool determinePathToConfig( BW::string & configFile )
{
	struct stat statinfo;

	// Use global toolsdir and look for config in std location
	if (stat( "/etc/bigworld.conf", &statinfo ) == 0)
	{
		ConfigReader bwconf( "/etc/bigworld.conf" );

		if (!bwconf.read())
		{
			ERROR_MSG( "MLUtil::determinePathToConfig: Error whilst reading "
				"config from /etc/bigworld.conf\n" );
			return false;
		}
		else
		{
			BW::string toolsDir;
			if (!bwconf.getValue( "tools", "location", toolsDir ))
			{
				ERROR_MSG( "MLUtil::determinePathToConfig: No tools directory "
					"specified in /etc/bigworld.conf\n" );
				return false;
			}
			else
			{
				configFile = toolsDir + "/message_logger/message_logger.conf";

				if (stat( configFile.c_str(), &statinfo ))
				{
					ERROR_MSG( "MLUtil::determinePathToConfig: "
						"Config file doesn't exist in standard location '%s'\n",
						configFile.c_str() );
					return false;
				}
			}
		}
	}

	// Next is config in current directory
	else if (stat( "./message_logger.conf", &statinfo ) == 0)
	{
		configFile = "./message_logger.conf";
	}

	else
	{
		ERROR_MSG( "MLUtil::determinePathToConfig: No valid configuration "
				"file found\n" );
		return false;
	}

	return true;
}


bool isPathAccessible( const char *path )
{
	struct stat statinfo;
	if (stat( path, &statinfo ))
	{
		ERROR_MSG( "MLUtil::isPathAccessible: Directory %s doesn't exist\n",
			path );
		return false;
	}

	if (!S_ISDIR( statinfo.st_mode ))
	{
		ERROR_MSG( "MLUtil::isPathAccessible: %s already exists and is "
			"not a directory\n", path );
		return false;
	}

	if (!(statinfo.st_mode & S_IROTH))
	{
		ERROR_MSG( "MLUtil::isPathAccessible: %s is not readable\n", path );
		return false;
	}

	return true;
}


bool softMkDir( const char *path )
{
	struct stat statinfo;
	if (stat( path, &statinfo ))
	{
		if (mkdir( path, 0777 ))
		{
			ERROR_MSG( "MLUtil::softMkDir: Couldn't make log directory "
				"'%s': %s\n", path, strerror( errno ) );
			return false;
		}
	}
	else if (!S_ISDIR( statinfo.st_mode ))
	{
		ERROR_MSG( "MLUtil::softMkDir: %s already exists and is not a "
			"directory\n", path );
		return false;
	}
	else if (!(statinfo.st_mode & S_IRWXU))
	{
		ERROR_MSG( "MLUtil::softMkDir: Insufficient permissions for %s "
			"(%o)\n", path, statinfo.st_mode );
		return false;
	}
	else if (statinfo.st_uid != geteuid())
	{
		ERROR_MSG( "MLUtil::softMkDir: %s is not owned by me (uid:%d)\n",
			path, geteuid() );
		return false;
	}

	return true;
}

BW_END_NAMESPACE


} // namespace MLUtil

