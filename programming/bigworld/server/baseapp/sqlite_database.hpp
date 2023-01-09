#ifndef SQLITE_DATABASE_HPP
#define SQLITE_DATABASE_HPP

#include "write_to_db_reply.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "network/basictypes.hpp"

struct sqlite3;
struct sqlite3_stmt;


BW_BEGIN_NAMESPACE

class MemoryOStream;


/**
 *	This class is a SQLite database row.
*/
class Row: public ReferenceCount
{
public:
	Row( DatabaseID dbID, EntityTypeID typeID, GameTime time,
			MemoryOStream & stream,	WriteToDBReplyStructPtr pReplyStruct );
	~Row();

	void writeToDB( sqlite3_stmt & stmt ) const;
	void onWriteToDBComplete();

private:
	DatabaseID				dbID_;
	EntityTypeID			typeID_;
	GameTime				time_;
	char *					pBlob_;
	unsigned int			blobSize_;
	WriteToDBReplyStructPtr	pReplyStruct_;
};

typedef SmartPointer<Row> RowPtr;


/**
 *	This class represents a transaction.
*/
class Transaction
{
public:
	Transaction() {};
	~Transaction();

	void writeToDB( RowPtr pRow );
	void commit( sqlite3 & con, const BW::string & table );
	void onWriteToDBComplete();

	bool empty() const { return rows_.empty(); }

private:
	typedef BW::vector<RowPtr> Rows;

	Rows rows_;
};


/**
 *	This class is a pool of transactions.
*/
class TransactionPool
{
public:
	TransactionPool();
	~TransactionPool();

	Transaction * acquire();
	void release( Transaction * pTrans, bool okayIfNotEmpty = false );

	unsigned int size() const { return pool_.size(); }

private:
	typedef BW::vector< Transaction * > Transactions;

	Transactions pool_;
	size_t poolSize_; //< Includes free and used transactions.
};


/**
 *	This class is an interface to a SQLite databases.
 */
class SqliteDatabase
{
public:
	SqliteDatabase( const BW::string & filename, const BW::string & dir );
	~SqliteDatabase();

	bool init( const BW::string & checksum );

	const BW::string& dbFilePath() const 	{ return path_; }

	void writeToDB( DatabaseID & dbID, EntityTypeID typeID, GameTime & time,
			MemoryOStream & stream,	WriteToDBReplyStructPtr pReplyStruct ) const;

	void commit( bool shouldFlip = false );
	void commitBgTask( Transaction * pTrans, bool shouldFlip );
	void commitFgTask( Transaction * pTrans );

	void isRegistered( bool flag )	{ isRegistered_ = flag; }

	void tick();

private:
	bool open();
	void close();

	bool writeChecksumTable( const BW::string & checksum ) const;

	void flipTable();

	bool checkFileSystemDevice();

	bool					isRegistered_;

	sqlite3 *				pCon_;

	const BW::string		path_;

	const BW::string *		pCurrTable_;

	TransactionPool			transPool_;
	Transaction *			pTrans_;

	TaskManager				taskMgr_;

	const BW::string		flipTable_;
	const BW::string		flopTable_;
	const BW::string		dbIDColumn_;
	const BW::string		typeIDColumn_;
	const BW::string		timeColumn_;
	const BW::string		blobColumn_;
	const BW::string		checksumTable_;
	const BW::string		checksumColumn_;

	const BW::string		cpCmd_;
	const BW::string		rmCmd_;
};

BW_END_NAMESPACE

#endif // SQLITE_DATABASE_HPP
