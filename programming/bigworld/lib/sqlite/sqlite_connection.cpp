#include "sqlite_connection.hpp"

#include "sqlite/sqlite3.h"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
SqliteConnection::SqliteConnection() :
	pDbHandle_( NULL )
{
}


/**
 *	Destructor.
 */
SqliteConnection::~SqliteConnection()
{
	if (pDbHandle_)
	{
		MF_VERIFY( sqlite3_close( pDbHandle_ ) != SQLITE_BUSY );
	}
}



/**
 *	This method opens an sqlite database read for read / write access.
 *
 * 	@param sqliteDbPath  The path to the sqlite database to open.
 *
 *	@returns true on success, false on error.
 */
bool SqliteConnection::open( const BW::string & sqliteDbPath )
{
	if (pDbHandle_)
	{
		ERROR_MSG( "SqliteConnection::open: "
			"An existing database has already been opened.\n" );
		return false;
	}

	if (sqlite3_open( sqliteDbPath.c_str(), &pDbHandle_ ) != SQLITE_OK)
	{
		ERROR_MSG( "SqliteConnection::open: "
			"Failed to open database '%s': %s\n",
			sqliteDbPath.c_str(), sqlite3_errmsg( pDbHandle_ ) );
		return false;
	}

	return true;
}


/**
 *	This method performs a safe backup of the source database into our
 *	database.
 *
 *	@param pSourceDb  A pointer to the sqlite database to backup.
 *	@param databaseName The database name of the database to backup within the
 *						sqlite DB file.
 *
 *	@returns true on success, false on error.
 */
bool SqliteConnection::backup( const SqliteConnection * pSourceDb,
	const BW::string & databaseName )
{
	if (!pSourceDb)
	{
		ERROR_MSG( "SqliteConnection::backup: "
			"No source sqlite database provided.\n" );
		return false;
	}

	sqlite3_backup * pBackup = sqlite3_backup_init(
								pDbHandle_, databaseName.c_str(),
								pSourceDb->pDbHandle_, databaseName.c_str() );
	if (pBackup == NULL)
	{
		ERROR_MSG( "SqliteConnection::backup: "
			"Failed to initialise the backup: %s\n",
			sqlite3_errmsg( pDbHandle_ ) );
		return false;
	}

	// -1 tells the backup to copy the entire source to the destination.
	int backupStatus = sqlite3_backup_step( pBackup, -1 );
	bool backupComplete = (backupStatus == SQLITE_DONE);
	if (!backupComplete)
	{
		ERROR_MSG( "SqliteConnection::backup: "
			"Failed to backup the database: errcode %d\n", backupStatus );
	}

	if (sqlite3_backup_finish( pBackup ) != SQLITE_OK)
	{
		ERROR_MSG( "SqliteConnection::backup: "
			"Error encountered finalising the backup: %s\n",
			sqlite3_errmsg( pDbHandle_ ) );
		return false;
	}

	return backupComplete;
}


/**
 *	This method returns the last error that was encountered on the sqlite
 *	database.
 */
const char * SqliteConnection::lastError() const
{
	return sqlite3_errmsg( pDbHandle_ );
}

BW_END_NAMESPACE

// sqlite_connection.cpp
