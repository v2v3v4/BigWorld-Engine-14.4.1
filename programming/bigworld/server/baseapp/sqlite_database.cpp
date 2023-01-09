#include "sqlite_database.hpp"

#include "cstdmf/debug_message_categories.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/timestamp.hpp"
#include "cstdmf/string_utils.hpp"

#include "sqlite/sqlite3.h"

#include <sstream>

#include <errno.h>
#include <string.h>
#include <sys/statfs.h>


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Row
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Row::Row( DatabaseID dbID, EntityTypeID typeID, GameTime time,
	   	MemoryOStream & stream, WriteToDBReplyStructPtr pReplyStruct ):
	dbID_( dbID ),
	typeID_( typeID ),
	time_( time ),
	pReplyStruct_( pReplyStruct )
{
	blobSize_ = stream.remainingLength();
	pBlob_ = new char[blobSize_];
	memcpy( pBlob_, stream.retrieve( blobSize_ ), blobSize_ );
}


/**
 * Destructor.
 */
Row::~Row()
{
	if (pBlob_)
	{
		delete[] pBlob_;
		pBlob_ = NULL;
	}
}


/**
 * This method writes the row to disk.
 */
void Row::writeToDB( sqlite3_stmt & stmt ) const
{
	if (sqlite3_bind_int64( &stmt, 1, dbID_ ) != SQLITE_OK)
	{
		SECONDARYDB_ERROR_MSG( "Row::writeToDB: Binding dbID to prepared"
				"statment failed\n" );
	}

	if (sqlite3_bind_int( &stmt, 2, typeID_ ) != SQLITE_OK)
	{
		SECONDARYDB_ERROR_MSG( "Row::writeToDB: Binding typeID to prepared"
				"statment failed\n" );
	}

	if (sqlite3_bind_int( &stmt, 3, time_ ) != SQLITE_OK)
	{
		SECONDARYDB_ERROR_MSG( "Row::writeToDB: Binding time to prepared"
				"statment failed\n" );
	}

	if (sqlite3_bind_blob( &stmt, 4, pBlob_, blobSize_, SQLITE_STATIC )
			!= SQLITE_OK)
	{
		SECONDARYDB_ERROR_MSG( "Row::writeToDB: Binding blob to prepared"
				"statment failed\n" );
	}

	if (sqlite3_step( &stmt ) != SQLITE_DONE)
	{
		SECONDARYDB_ERROR_MSG( "Row::writeToDB: "
			"Executing prepared statement failed\n" );
	}

	if (sqlite3_reset( &stmt ) != SQLITE_OK)
	{
		SECONDARYDB_ERROR_MSG( "Row::writeToDB: "
			"Resetting prepared statement failed\n" );
	}
}


/**
 * This method notifies write completed.
 */
void Row::onWriteToDBComplete()
{
	pReplyStruct_->onWriteToDBComplete( true );
}


// -----------------------------------------------------------------------------
// Section: Transaction
// -----------------------------------------------------------------------------

/**
 * Destructor.
 */
Transaction::~Transaction()
{
	rows_.clear();
}


/**
 *  This method adds a write to the transaction. It is actually written to
 *  disk when the transaction is committed.
 */
void Transaction::writeToDB( RowPtr pRow )
{
	rows_.push_back( pRow );
}


/**
 * This method commits the transaction.
 */
void Transaction::commit( sqlite3 & con, const BW::string & table )
{
	BW::string strStmt( "INSERT INTO " );
	strStmt += table;
	strStmt += " VALUES (?, ?, ?, ?)";

	sqlite3_stmt * pStmt;

	if (sqlite3_prepare( &con, strStmt.c_str(), -1, &pStmt, 0 ) != SQLITE_OK)
	{
		SECONDARYDB_CRITICAL_MSG( "Transaction::commit: "
				"Failed to prepare sqlite statement: %s\n",
			sqlite3_errmsg( &con ) );
	}

	char * errmsg;

	if (sqlite3_exec( &con, "BEGIN", 0, 0, &errmsg ) != SQLITE_OK)
	{
		SECONDARYDB_ERROR_MSG( "Transaction::commit: Cannot begin transaction "
				"[%s]\n", errmsg );
	}

	Rows::iterator iter = rows_.begin();
	Rows::iterator end = rows_.end();

	while (iter != end)
	{
		(*iter)->writeToDB( *pStmt );
		++iter;
	}

	if (sqlite3_exec( &con, "COMMIT", 0, 0, &errmsg ) != SQLITE_OK)
	{
		SECONDARYDB_ERROR_MSG( "Transaction::commit: "
			"Cannot commit transaction [%s]\n", errmsg );
	}

	if (sqlite3_finalize( pStmt ) != SQLITE_OK)
	{
		SECONDARYDB_ERROR_MSG( "Transaction::commit: "
				"Destroying prepared statement failed: %s\n",
			sqlite3_errmsg( &con ) );
	}
}


/**
 * This method notifies writess completed.
 */
void Transaction::onWriteToDBComplete()
{
	Rows::iterator iter = rows_.begin();
	Rows::iterator end = rows_.end();

	while (iter != end)
	{
		(*iter)->onWriteToDBComplete();
		++iter;
	}

	rows_.clear();
}


// -----------------------------------------------------------------------------
// Section: TransactionPool
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
TransactionPool::TransactionPool() : poolSize_( 0 )
{
}


/**
 *	Destructor.
 */
TransactionPool::~TransactionPool()
{
	if (pool_.size() != poolSize_)
	{
		SECONDARYDB_WARNING_MSG( "TransactionPool::~TransactionPool: "
				"pool_.size() = %" PRIzu ". poolSize_ = %" PRIzu "\n",
			pool_.size(), poolSize_ );
	}

	Transactions::iterator iter = pool_.begin();

	while (iter != pool_.end())
	{
		delete *iter;

		++iter;
	}

	pool_.clear();
}


/**
 *	This method acquires a transaction from the pool.
 */
Transaction * TransactionPool::acquire()
{
	if (pool_.empty())
	{
		pool_.push_back( new Transaction );
		++poolSize_;

		if (poolSize_ >= 100)
		{
			SECONDARYDB_WARNING_MSG( "TransactionPool::acquire: "
							"Pool size increased to %" PRIzu "\n",
						poolSize_ );
		}
		else if (poolSize_ >= 10)
		{
			SECONDARYDB_INFO_MSG( "TransactionPool::acquire: "
							"Pool size increased to %" PRIzu "\n",
						poolSize_ );
		}
	}

	Transaction * pTrans = pool_.back();
	pool_.pop_back();

	return pTrans;
}


/**
 *	This method releases a transaction back into the pool.
 */
void TransactionPool::release( Transaction * pTrans, bool okayIfNotEmpty )
{
	MF_ASSERT( pTrans->empty() || okayIfNotEmpty );

	pool_.push_back( pTrans );
}


// -----------------------------------------------------------------------------
// Section: CommitTask
// -----------------------------------------------------------------------------

/**
 *	This class simply perform callbacks from foreground and background threads.
 */
class CommitTask : public BackgroundTask
{
public:
	CommitTask( SqliteDatabase & db, Transaction * pTrans,	bool shouldFlip ):
		BackgroundTask ( "CommitTask" ),
   		db_( db ),
   		pTrans_( pTrans ),
   		shouldFlip_( shouldFlip )
	{
	}

	virtual void doBackgroundTask( TaskManager & mgr )
	{
		db_.commitBgTask( pTrans_, shouldFlip_ );
		mgr.addMainThreadTask( this );
	}

	virtual void doMainThreadTask( TaskManager & mgr )
	{
		db_.commitFgTask( pTrans_ );
	}

private:
	SqliteDatabase &	db_;
	Transaction *		pTrans_;
	bool 				shouldFlip_;
};


// -----------------------------------------------------------------------------
// Section: SqliteDatabase
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
SqliteDatabase::SqliteDatabase( const BW::string & filename,
		const BW::string & dir ) :
	isRegistered_( false ),
	pCon_( NULL ),
	path_( dir + filename ),
	pCurrTable_( &flipTable_ ),
	pTrans_( transPool_.acquire() ),
	flipTable_( "tbl_flip" ),
	flopTable_( "tbl_flop" ),
	dbIDColumn_( "sm_dbID" ),
	typeIDColumn_( "sm_typeID" ),
	timeColumn_( "sm_time" ),
	blobColumn_( "sm_blob" ),
	checksumTable_( "tbl_checksum" ),
	checksumColumn_( "sm_checksum" )
{
}


/**
 *	This method initialises this object.
 *
 *	@return True on success, otherwise false.
 */
bool SqliteDatabase::init( const BW::string & checksum )
{
	if (!this->open())
	{
		return false;
	}

	if (!this->checkFileSystemDevice())
	{
		return false;
	}

	if (!this->writeChecksumTable( checksum ))
	{
		SECONDARYDB_ERROR_MSG( "SqliteDatabase::init: Failed '%s'\n",
				sqlite3_errmsg( pCon_ ) );
		if (sqlite3_errcode( pCon_ ) == SQLITE_BUSY)
		{
			SECONDARYDB_ERROR_MSG( "SqliteDatabase::init: "
				"If you are running a server from a Windows share, try adding "
				"the 'nobrl' option to your /etc/fstab share line. This is a "
				"known issue with older versions of Samba.\n" );
		}
		return false;
	}

	// Create both tables
	this->flipTable();
	this->flipTable();

	taskMgr_.initWatchers( "SqliteDatabase" );
	taskMgr_.startThreads( "SqliteDatabase", 1 );

	SECONDARYDB_INFO_MSG( "SqliteDatabase::init: "
		"Secondary database started\n" );

	return true;
}


/**
 *	This method returns true if the database is located on a filesystem that is
 *	supported, false otherwise.
 */ 
bool SqliteDatabase::checkFileSystemDevice()
{
	static const long BAD_MAGICS[] = { 
		/* CIFS_MAGIC_NUMBER = */ 	0xFF534D42,
		/* SMB_SUPER_MAGIC = */ 	0x517B,
	};

	struct statfs info;
	if (0 != statfs( path_.c_str(), &info ))
	{
		SECONDARYDB_ERROR_MSG( "SqliteDatabase::checkFileSystemDevice: "
				"failed to call statfs on path \"%s\": %s\n",
			path_.c_str(), strerror( errno ) );
		return false;
	}

	for (size_t i = 0; i < sizeof( BAD_MAGICS ) / sizeof( long ); ++i)
	{
		if (info.f_type == BAD_MAGICS[i])
		{
			SECONDARYDB_ERROR_MSG( "SqliteDatabase::checkFileSystemDevice: "
				"database is located on a CIFS or SMB file system, "
				"for which sqlite locking behaviour is undefined.\n" );
			return false;
		}
	}

	return true;
}


/**
 *	Destructor.
 */
SqliteDatabase::~SqliteDatabase()
{
	transPool_.release( pTrans_, true );
	pTrans_ = NULL;

	taskMgr_.stopAll( true /*discardPendingTask*/, true /*waitForThreads*/ );

	if (pCon_)
	{
		this->close();
	}

	if (!isRegistered_)
	{
		if (remove( path_.c_str() ) != 0)
		{
			char errnoBuf[1024];

			SECONDARYDB_ERROR_MSG( "SqliteDatabase::~SqliteDatabase: "
					"Failed to delete unregistered database '%s': %s\n",
				path_.c_str(),
				strerror_r( errno, errnoBuf, sizeof( errnoBuf ) ) );
		}
	}

	SECONDARYDB_INFO_MSG("SqliteDatabase::~SqliteDatabase: "
		"Secondary database stopped\n" );
}


/**
 *  This method adds an entity as a row to the current transaction.
 */
void SqliteDatabase::writeToDB( DatabaseID & dbID, EntityTypeID typeID,
		GameTime & time, MemoryOStream & stream,
		WriteToDBReplyStructPtr pReplyStruct ) const
{
	pTrans_->writeToDB(
			new Row( dbID, typeID, time, stream, pReplyStruct ) );
}


/**
 *  This method starts the commit sequence.
 */
void SqliteDatabase::commit( bool shouldFlip )
{
	if (pTrans_->empty() && !shouldFlip)
	{
		return;
	}

	Transaction * pCommitTrans = pTrans_;
	pTrans_ = transPool_.acquire();

	taskMgr_.addBackgroundTask(
			new CommitTask( *this, pCommitTrans, shouldFlip ) );
}


/**
 *  This method commits a transaction. This should be called from a
 *  background task.
 */
void SqliteDatabase::commitBgTask( Transaction * pTrans, bool shouldFlip )
{
	if (!pTrans->empty())
	{
		pTrans->commit( *pCon_, *pCurrTable_);
	}

	if (shouldFlip)
	{
		this->flipTable();
	}
}


/**
 *  This method ends the commit sequence by replying to the writeToDB()
 *  calls. This should be called from a foreground task.
 */
void SqliteDatabase::commitFgTask( Transaction * pTrans )
{
	pTrans->onWriteToDBComplete();

	transPool_.release( pTrans );
}


/**
 *  This method relays the tick to TaskMgr.
 */
void SqliteDatabase::tick()
{
	taskMgr_.tick();
}


/**
 *  This method opens a database file.
 */
bool SqliteDatabase::open()
{
	MF_ASSERT ( pCon_ == NULL );

	if (sqlite3_open( path_.c_str(), &pCon_ ) != SQLITE_OK)
	{
		SECONDARYDB_ERROR_MSG( "SqliteDatabase::open: "
				"Could not open %s - '%s'\n",
			path_.c_str(), sqlite3_errmsg( pCon_ ) );
		return false;
	}

	MF_ASSERT ( pCon_ != NULL );

	return true;
}


/**
 *  This method closes the database file.
 */
void SqliteDatabase::close()
{
	MF_ASSERT ( pCon_ != NULL );

	int rc = sqlite3_close( pCon_ );

	MF_ASSERT( rc == SQLITE_OK );

	pCon_ = NULL;
}


/**
 *  This method writes the checksum table.
 */
bool SqliteDatabase::writeChecksumTable( const BW::string & checksum ) const
{
	char * errmsg;
	BW::string stmt;

	stmt =  "DROP TABLE IF EXISTS " + checksumTable_;
	if (sqlite3_exec( pCon_, stmt.c_str(), 0, 0, &errmsg ) != SQLITE_OK)
	{
		return false;
	}

	stmt = "CREATE TABLE " + checksumTable_ + " (" + checksumColumn_ + " TEXT)";
	if (sqlite3_exec( pCon_, stmt.c_str(), 0, 0, &errmsg ) != SQLITE_OK)
	{
		return false;
	}

	stmt = "INSERT INTO " + checksumTable_ + " VALUES (\""+ checksum + "\")";
	if (sqlite3_exec( pCon_, stmt.c_str(), 0, 0, &errmsg ) != SQLITE_OK )
	{
		return false;
	}

	return true;
}


/**
 *  This method flip flops across the two tables.
 */
void SqliteDatabase::flipTable()
{
	pCurrTable_ = (pCurrTable_ != &flipTable_) ? &flipTable_ : &flopTable_;

	char * errmsg;
	BW::string stmt;

	// Could store stmt to prevent re-generation
	stmt = "DROP TABLE IF EXISTS " + *pCurrTable_;
	sqlite3_exec( pCon_, stmt.c_str(), 0, 0, &errmsg );

	stmt = "CREATE TABLE " + *pCurrTable_ + " (" + dbIDColumn_ +
		" INTEGER, " + typeIDColumn_ + " INTEGER, " + timeColumn_ +
		" INTEGER, " + blobColumn_ + " BLOB)";
	sqlite3_exec( pCon_, stmt.c_str(), 0, 0, &errmsg );
}

BW_END_NAMESPACE

// sqlite_database.cpp
