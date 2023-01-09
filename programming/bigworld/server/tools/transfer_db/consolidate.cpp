#include "consolidate.hpp"

#include "cstdmf/stdmf.hpp"

#include "network/basictypes.hpp"
#include "network/machined_utils.hpp"

#include "server/bwconfig.hpp"

// TODO: migrate this to use SqliteConnection
#include "sqlite/sqlite3.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "cstdmf/bw_string.hpp"

DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )

BW_BEGIN_NAMESPACE

/*
 * TransferDB protocol:
 *
 * 1 byte - message type
 *          - 'e'  Error
 *          - 'n'  SqliteDB file transfer
 *          - 'd'  Delete SQLite DB file
 *
 * The 'e' and 'n' commands are sent from transfer_db to consolidate_db
 * The 'd' command is received by transfer_db from consolidate_db
 *
 * Error message:
 * 2   bytes - (uint16) size of error string following
 * <n> bytes - error string (not '\0' terminated)
 *
 * 
 * SQLiteDB file transfer message:
 * 2   bytes - (uint16) size of the filename string to follow
 * <n> bytes - the string of the sqlite db file being sent
 * 4   bytes - (uint32) size of the sqlite database file to follow
 * <n> bytes - the contents of the sqlite db file.
 *
 * Delete SQLite DB file message:
 * No further data on this message stream.
 */




/**
 *	Constructor.
 */
Consolidate::Consolidate( BW::string dbFilename ) :
	endpoint_(),
	sqliteSize_( 0 ),
	sqliteFilename_( dbFilename )
{
}


/**
 *	Destructor.
 */
Consolidate::~Consolidate()
{
}


/**
 *
 */
void Consolidate::error( BW::string errorStr, BW::string supplementalStr )
{
	MemoryOStream responseStream;

	uint16 stringSize = errorStr.size();

	bool hasSupplementalString = (bool)supplementalStr.size();

	if (hasSupplementalString)
	{
		// We add 3 bytes so we can suffix the the supplemental string
		// in brackets to the main error string.
		// ie: '%s (%s)' % (errorString, supplementalStr)
		stringSize += supplementalStr.size() + 3;
	}

	ERROR_MSG( "Consolidate::transferTo: %s (%s)",
		errorStr.c_str(), supplementalStr.c_str() );

	// Unfortunately the receiving size assumes a uint16 will be received so
	// we can't simply use the string streaming functionality.
	responseStream << 'e' << (uint16)stringSize;
	responseStream.addBlob( errorStr.c_str(), errorStr.size() );
	if (hasSupplementalString)
	{
		responseStream.addBlob( " (", 2 );
		responseStream.addBlob( supplementalStr.c_str(),
			supplementalStr.size() );
		responseStream << ')';
	}

	if (endpoint_.send( responseStream.data(), responseStream.size() ) !=
		responseStream.size() )
	{
		ERROR_MSG( "Consolidate::transferTo: "
			"Failed to send error message back to ConsolidateDBs\n" );
	}

	return;
}



/**
 *
 */
bool Consolidate::transferTo( Mercury::Address & receivingAddress )
{
	INFO_MSG( "Consolidate::transferTo: "
		"Establishing connection to %s\n", receivingAddress.c_str() );

	// Attempt to establish a connection
	endpoint_.socket( SOCK_STREAM );
	if (endpoint_.connect( receivingAddress.port, receivingAddress.ip ) == -1)
	{
		ERROR_MSG( "Consolidate::transferTo: "
			"Failed to establish a connection to receiver: %s\n",
			strerror( errno ) );
		return false;
	}

	INFO_MSG( "Consolidate::transferTo: "
		"Connected to %s\n", receivingAddress.c_str() );

	// Verify the file we want to transfer actually exists

	// stat() is used, rather than using the sqlite library to connect to the
	// db because sqlite will create the db file.
	struct stat statbuf;
	if (stat( sqliteFilename_.c_str(), &statbuf ) == -1)
	{
		this->error( "Unable to stat the sqlite file", strerror( errno ) );
		return false;
	}

	// This is used when we actually send the file. It saves us having
	// to seek to the EOF to get a file size.
	sqliteSize_ = statbuf.st_size;

	if (!this->prepareDBForSend())
	{
		return false;
	}

	if (!this->readAndSendDB())
	{
		WARNING_MSG( "Consolidate::transferTo: Transfer interrupted.\n" );
		return false;
	}

	INFO_MSG( "Consolidate::transferTo: Transfer completed.\n" );


	if (!this->waitForDeleteCommand())
	{
		return false;
	}

	INFO_MSG( "Consolidate::transferTo: All done.\n" );

	return true;
}


/**
 *	This method checks the filename requested is a valid SQLite DB file and
 *	removes any journal files before allowing it to be sent.
 */
bool Consolidate::prepareDBForSend()
{
	// Open and close the database to get rid of any journal files.
	sqlite3 *db;

	if (sqlite3_open( sqliteFilename_.c_str(), &db ) != SQLITE_OK)
	{
		this->error( "Unable to open sqlite file using sqlite3",
			sqlite3_errmsg( db ) );

		sqlite3_close( db );
		return false;
	}

	// This will be potentially allocated inside sqlite and must be free'd
	// using sqlite3_free()
	char *sqlErrStr = NULL;
	if (sqlite3_exec( db, "SELECT * FROM tbl_checksum", NULL, NULL, &sqlErrStr )
		!= SQLITE_OK)
	{
		this->error( "Unable to flush sqlite journal files", sqlErrStr );

		sqlite3_free( sqlErrStr );
		sqlite3_close( db );
		return false;

	}

	sqlite3_close( db );

	return true;
}


/**
 *
 */
bool Consolidate::readAndSendDB()
{
	// Open the secondary DB file to dump into memory and
	// send back to consolidateDBs
	int fd = open( sqliteFilename_.c_str(), O_RDONLY );
	if (fd == -1)
	{
		this->error( "Unable to open sqlite file", strerror( errno ) );

		return false;

	}

	INFO_MSG( "Consolidate::readAndSendDB: Sending file: %s",
		sqliteFilename_.c_str() );

	// Send just the filename first. This allows consolidateDBs to know which
	// file it is receiving if multiple have been requested from one machine
	MemoryOStream responseStream;

	// The receiving size assumes a uint16 will be received so the string
	// streaming functionality can't be used.
	responseStream << 'n' << (uint16)sqliteFilename_.size();
	responseStream.addBlob( sqliteFilename_.c_str(), sqliteFilename_.size() );
	// Next comes the file size of the SQLiteDB being transferred
	responseStream << (uint32)sqliteSize_;

	if (endpoint_.send( responseStream.data(),
			responseStream.remainingLength() ) == -1)
	{
		ERROR_MSG( "Consolidate::readAndSendDB: "
			"Unable to send initial file transfer data.\n" );
		return false;
	}


	char workingBuffer[ 4096 ];
	ssize_t amountRead = 0;
	bool fatalError = false;

	// Hard loop to read all the data in and send it straight to consolidateDBs
	do
	{
		amountRead = read( fd, workingBuffer, sizeof( workingBuffer ) );
		if (amountRead == -1)
		{
			switch (errno)
			{
			case EINTR:
				continue;
				break;

			default:
				fatalError = true;
				this->error( "Error reading sqlite file", strerror( errno ) );
				break;
			}
		}
		else if (amountRead > 0)
		{
			int remainingToSend = amountRead;
			int sentAmount = 0;

			while (remainingToSend > 0 && fatalError == false)
			{
				int lastSend = endpoint_.send( workingBuffer + sentAmount,
												remainingToSend );
				if (lastSend == -1)
				{
					if (errno == EINTR)
					{
						continue;
					}

					fatalError = true;
				}
				else
				{
					remainingToSend -= lastSend;
					sentAmount += lastSend;
				}
			}
		}

	} while (!fatalError && (amountRead != 0));

	close( fd );
	fd = 0;

	return !fatalError;
}


bool Consolidate::waitForDeleteCommand()
{
	char commandString[ 1 ];
	commandString[ 0 ] = '\0';
	if (endpoint_.recv( commandString, 1 ) == -1)
	{
		ERROR_MSG( "Consolidate::waitForDeleteCommand: "
				"Failed to receive delete command from ConsolidateDBs.\n" );
		return false;
	}

	if ( commandString[ 0 ] != 'd' )
	{
		ERROR_MSG( "Consolidate::waitForDeleteCommand: "
				"Received unexpected character '%c'\n", commandString[ 0 ] );
		return false;
	}
	
	INFO_MSG( "Consolidate::waitForDeleteCommand: Deleting '%s'\n",
		sqliteFilename_.c_str() );

	if (unlink( sqliteFilename_.c_str() ) == -1)
	{
		// ConsolidateDB has shutdown it's own end at this point so there
		// is no point sending it an error message.
		ERROR_MSG( "Consolidate::waitForDeleteCommand: "
				"Unable to remove the secondary DB file: %s\n", 
			strerror( errno ) );
		return false;
	}

	return true;
}

BW_END_NAMESPACE

// consolidate.cpp
