#include "server/config_reader.hpp"

#include "snapshot_constants.hpp"

#include <mysql/mysql.h>

#include <errno.h>
#include <string.h>

#include <cstring>
#include <cstdlib>

#include "cstdmf/allocator.hpp"
#include "cstdmf/stdmf.hpp"


BW_USE_NAMESPACE

bool becomeRootUser();
bool validate_config( ConfigReader & config );
bool acquire_snapshot( ConfigReader & config,
	const char * dbUser, const char * dbPass );
bool release_snapshot( ConfigReader & config );

namespace
{

#define LVM_BIN_DIR "/usr/sbin/"

const char *CONFIG_SECTION = "snapshot";

const char *LVCREATE = LVM_BIN_DIR"lvcreate";

const char *LVREMOVE = LVM_BIN_DIR"lvremove";

#undef LVM_BIN_DIR
}


/**
 *	This util executes privileged commands. The command arguments
 *	are read in from bigworld.conf which should be writable by
 *	root only.
 *
 *	To execute privileged commands the snapshot_helper binary needs
 *	to have it's setuid attribute set. This can be done by:
 *
 *	# chown root:root snapshot_helper
 *	# chmod 4511 snapshot_helper
 */
int main( int argc, char * argv[] )
{
	BW_SYSTEMSTAGE_MAIN();
	printf( "SnapshotHelper started\n" );

	// If no arguments are provided, test if setuid attribute is set
	if (argc <= 1)
	{
		return becomeRootUser() ?
			SnapshotHelper::SUCCESS : SnapshotHelper::FAILURE;
	}

	// Read the configuration file and confirm all appropriate values exist
	ConfigReader config( "/etc/bigworld.conf" );
	if (!config.read())
	{
		printf( "Unable to read config file.\n" );
		return SnapshotHelper::FAILURE;
	}

	if (!validate_config( config ))
	{
		return SnapshotHelper::FAILURE;
	}

	// Execute commands
	if (std::strcmp( argv[ 1 ], "acquire-snapshot" ) == 0)
	{
		if (argc != 4)
		{
			printf( "Invalid argument list for 'acquire-snapshot'\n" );
		}

		if (!acquire_snapshot( config, argv[ 2 ], argv[ 3 ] ))
		{
			return SnapshotHelper::FAILURE;
		}
	}
	else if (std::strcmp( argv[ 1 ], "release-snapshot" ) == 0)
	{
		if (argc != 2)
		{
			printf( "Invalid argument list for 'release-snapshot'\n" );
		}

		if (!release_snapshot( config ))
		{
			return SnapshotHelper::FAILURE;
		}
	}
	else
	{
		printf( "Unknown arguments provided.\n" );
		return SnapshotHelper::FAILURE;
	}

	return SnapshotHelper::SUCCESS;
}


/**
 *	This method validates that all configuration options required by the
 *	snapshot_helper program have been defined.
 */
bool validate_config( ConfigReader & config )
{
	bool status = true;

	const char *config_values[] = { "datadir",
		"lvgroup",
		"lvorigin",
		"lvsnapshot",
		"lvsizegb" };

	for (BW::uint i=0; i < ARRAY_SIZE( config_values ); ++i)
	{
		const char *value = config_values[ i ];
		BW::string testString;

		if (!config.getValue( CONFIG_SECTION, value, testString ))
		{
			printf( "Unable to read config value for '%s'\n", value );
			status = false;
		}
	}

	return status;
}


// TODO: sanitise the dbUser / dbPass, they are provided on the command line
bool acquire_snapshot( ConfigReader & config,
	const char * dbUser, const char * dbPass )
{
	// Elevate privileges for actual operations
	if (!becomeRootUser())
	{
		return false;
	}

	BW::string dataDir;
	config.getValue( CONFIG_SECTION, "datadir", dataDir );

	BW::string lvGroup;
	config.getValue( CONFIG_SECTION, "lvgroup", lvGroup );

	BW::string lvOrigin;
	config.getValue( CONFIG_SECTION, "lvorigin", lvOrigin );

	BW::string lvSnapshot;
	config.getValue( CONFIG_SECTION, "lvsnapshot", lvSnapshot );

	BW::string lvSizeGB;
	config.getValue( CONFIG_SECTION, "lvsizegb", lvSizeGB );


	// Connect to the database and apply a read lock.
	BW::string cmd;
	MYSQL sql;

	if (mysql_init( &sql ) == NULL)
	{
		printf( "Unable to initialise MySQL\n" );
		return false;
	}

	// TODO: assuming that the connection can be made on 'localhost'. it may
	//       need to use the hostname depending on the database user's GRANT
	//       privileges
	if (!mysql_real_connect( &sql, "localhost", dbUser, dbPass,
		NULL, 0, NULL, 0 ))
	{
		printf( "Failed to connect to MySQL: %s\n", mysql_error( &sql ) );
		return false;
	}

	cmd = "FLUSH TABLES WITH READ LOCK";
	if (mysql_real_query( &sql, cmd.c_str(), cmd.length() ) != 0)
	{
		printf( "Failed to apply read lock MySQL: %s\n", mysql_error( &sql ) );
		return false;
	}

	// Now create an LVM snapshot
	if (execl( LVCREATE, 
		(BW::string("-L") + lvSizeGB + "G").c_str(), 
		"-s", "-n", lvSnapshot.c_str(),
		(BW::string("/dev/") + lvGroup + "/" + lvOrigin).c_str(),
		NULL ) != 0 )
	{
		printf( "Failed to 'lvcreate' LVM snapshot.\n" );
		mysql_close( &sql );
		return false;
	}

// TODO: if we fail to unlock or mount or chmod the snapshot we can't really
//       recover.

// TODO: anything past this point must LVREMOVE as the LVCREATE has already worked,
//       any subsequent calls will otherwise fail

	cmd = "UNLOCK TABLES";
	if (mysql_real_query( &sql, cmd.c_str(), cmd.length() ) != 0)
	{
		printf( "Failed to unlock MySQL: %s\n", mysql_error( &sql ) );
		mysql_close( &sql );
		return false;
	}

	// The MySQL connection is no longer needed
	mysql_close( &sql );

// TODO: It's possible that the mount directory doesn't exist,
//       we should create it if it doesn't.
	if (execl( "mount",
		("/dev/" + lvGroup + "/" + lvSnapshot).c_str(),
		("/mnt/" + lvSnapshot + "/").c_str(),
		NULL ) != 0)
	{
		printf( "Failed to mount newly created snapshot.\n" );
		return false;
	}

	BW::string snapshotFiles( "/mnt/" + lvSnapshot + "/" + dataDir );

	// Relax permissions so we can take ownership of the backup files,
	// this makes sending and consolidating easier on the snapshot machine
	if (execl( "chmod", "-R", "755", snapshotFiles.c_str(), NULL ) != 0)
	{
		printf( "Failed to chmod the snapshot file.s\n" );
		return false;
	}

	printf( "%s\n", snapshotFiles.c_str() );

	return true;
}


/**
 *	This function unmounts the snapshot directory and releases it from the
 *	filesystem.
 */
bool release_snapshot( ConfigReader & config )
{
	// Elevate privileges for actual operations
	if (!becomeRootUser())
	{
		return false;
	}

	BW::string lvGroup;
	config.getValue( CONFIG_SECTION, "lvgroup", lvGroup );

	BW::string lvSnapshot;
	config.getValue( CONFIG_SECTION, "lvsnapshot", lvSnapshot );

	bool isOK = true;

	if (execl( "umount", ("/mnt/" + lvSnapshot + "/").c_str(), NULL ) != 0)
	{
		printf( "Unable to unmount snapshot.\n" );
		isOK = false;
	}

	if (execl( LVREMOVE, "-f",
		("/dev/" + lvGroup, "/" + lvSnapshot).c_str(), NULL ) != 0)
	{
		printf( "Unable to 'lvremove' snapshot.\n" );
		isOK = false;
	}

	return isOK;
}


/**
 *	This function changes the process privileges from a standard user to the
 *	root user. The allows the mount and LVM operations to be performed.
 */
bool becomeRootUser()
{
	if (setuid( 0 ) == -1)
	{
		printf( "setuid root failed: %s\n", strerror( errno ) );
		return false;
	}

	return true;
}

// snapshot_helper.cpp
