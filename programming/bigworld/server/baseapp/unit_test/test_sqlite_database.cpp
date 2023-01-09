#include <sys/stat.h>

#include "pch.hpp"

#include "sqlite/sqlite3.h"

#include "sqlite/sqlite_connection.hpp"
#include "sqlite/sqlite_statement.hpp"

#include "cstdmf/bw_util.hpp"
#include "cstdmf/binary_stream.hpp"
#include "cstdmf/memory_stream.hpp"

#include "baseapp/sqlite_database.hpp"

#include <libgen.h>
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

const BW::string temporaryDirectory()
{
	char *tmpStr = strdup( getTempFilePathName().c_str() );
	char *dirName = dirname( tmpStr );
	const BW::string finalDirectory = dirName;
	free( tmpStr );
	return finalDirectory;
}

const BW::string databaseFileWithPid()
{
	pid_t pid = getpid();
	char buf[ 256 ];
	snprintf( buf, sizeof( buf ), "test_sqlite_database_%u.db", pid );
	return buf;
}

const BW::string TEST_DIR( temporaryDirectory() + "/" );
const BW::string TEST_DB( databaseFileWithPid() );
const BW::string TEST_DB_PATH = TEST_DIR + TEST_DB;


class ReplyHandler : public WriteToDBReplyHandler
{
public:
	ReplyHandler( SqliteDatabase & db ):
		db_( db ),
		count_( 0 )
	{
	}

	bool waitForReplies( unsigned int numExpected, unsigned int timeout )
	{
		unsigned int timeTaken = 0;

		while (true)
		{
			db_.tick();

			if (count_ == numExpected)
			{
				return true;
			}
			if (count_ > numExpected || timeTaken >= timeout)
			{
				return false;
			}

			sleep( 1 );
			timeTaken++;
		}
	}

	virtual void onWriteToDBComplete( bool succeeded )
	{
		if (succeeded)
		{
			count_++;
		}
	}

private:
	SqliteDatabase & db_;
	unsigned int count_;
};

WriteToDBReplyStructPtr createReplyStruct( WriteToDBReplyHandler * pHandler )
{
	WriteToDBReplyStructPtr pReplyStruct = new WriteToDBReplyStruct( pHandler );
	// Not doing backup here so just indicate that it was successful
	pReplyStruct->onBackupComplete();

	return pReplyStruct;
}


int getNumRows( const BW::string & tblName )
{
	BW::string sql( "SELECT COUNT(*) FROM " + tblName );

	SqliteConnection con;
	if (!con.open( TEST_DB_PATH ))
	{
		return 0;
	}

	int result;
	SqliteStatement stmt( con, sql.c_str(), result );
	stmt.step();

	return stmt.intColumn( 0 );
}


TEST( SqliteDatabase_testCreate )
{
	struct stat fileInfo;
	SqliteDatabase *db = new SqliteDatabase( TEST_DB, TEST_DIR );
	bool initSuccess = db->init( "" );
	CHECK( initSuccess );

	CHECK( stat( TEST_DB_PATH.c_str(), &fileInfo ) == 0 );
	delete db;
	remove( TEST_DB_PATH.c_str() );
}


TEST( SqliteDatabase_testRemove )
{
	struct stat fileInfo;
	SqliteDatabase *db = new SqliteDatabase( TEST_DB, TEST_DIR );
	bool initSuccess = db->init( "" );
	CHECK( initSuccess );

	// Wasn't registered so sqlite file removed on destruction
	delete db;
	CHECK( stat( TEST_DB_PATH.c_str(), &fileInfo ) != 0 );
	remove( TEST_DB_PATH.c_str() );
}


TEST( SqliteDatabase_testWriteToDB )
{
	SqliteDatabase *db = new SqliteDatabase( TEST_DB, TEST_DIR );
	bool initSuccess = db->init( "" );
	CHECK( initSuccess );

	ReplyHandler handler( *db );

	DatabaseID dbID = 1;
	EntityTypeID entityTypeID = 1;
	GameTime timestamp = 0;
	MemoryOStream stream;

	int numWriteToDBs = 1000;

	for (int i = 0; i < numWriteToDBs; i++)
	{
		stream << "blob";
		db->writeToDB( dbID, entityTypeID, timestamp, stream,
				createReplyStruct( &handler ) );
	}

	db->commit();
	handler.waitForReplies( numWriteToDBs, 60 );

	int rowCount = getNumRows( "tbl_flip" ) + getNumRows( "tbl_flop" );
	CHECK_EQUAL( numWriteToDBs, rowCount );

	delete db;
	remove( TEST_DB_PATH.c_str() );
}


TEST( SqliteDatabase_testWriteToDBAndNoCommit )
{
	SqliteDatabase *db = new SqliteDatabase( TEST_DB, TEST_DIR );
	bool initSuccess = db->init( "" );
	CHECK( initSuccess );

	ReplyHandler handler( *db );

	DatabaseID dbID = 1;
	EntityTypeID entityTypeID = 1;
	GameTime timestamp = 0;
	MemoryOStream stream;

	int numWriteToDBs = 1000;

	for (int i = 0; i < numWriteToDBs; i++)
	{
		stream << "blob";
		db->writeToDB( dbID, entityTypeID, timestamp, stream,
				createReplyStruct( &handler ) );
	}

	// It's going to timeout so use a short one
	handler.waitForReplies( numWriteToDBs, 5 ); 

	int rowCount = getNumRows( "tbl_flip" ) + getNumRows( "tbl_flop" );
	CHECK_EQUAL( 0, rowCount  );

	delete db;
	remove( TEST_DB_PATH.c_str() );
}


TEST( SqliteDatabase_testFlip )
{
	SqliteDatabase *db = new SqliteDatabase( TEST_DB, TEST_DIR );
	bool initSuccess = db->init( "" );
	CHECK( initSuccess );

	ReplyHandler handler( *db );

	DatabaseID dbID = 1;
	EntityTypeID entityTypeID = 1;
	GameTime timestamp = 0;
	MemoryOStream stream;

	int numWriteToDBs = 1000;

	for (int i = 0; i < numWriteToDBs; i++)
	{
		stream << "blob";
		db->writeToDB( dbID, entityTypeID, timestamp, stream,
				createReplyStruct( &handler ) );
	}

	db->commit( true );
	handler.waitForReplies( numWriteToDBs, 60 );

	for (int i = 0; i < numWriteToDBs; i++)
	{
		stream << "blob";
		db->writeToDB( dbID, entityTypeID, timestamp, stream,
				createReplyStruct( &handler ) );
	}

	db->commit();
	handler.waitForReplies( numWriteToDBs*2, 60 );

	CHECK_EQUAL( numWriteToDBs, getNumRows( "tbl_flip" ) );
	CHECK_EQUAL( numWriteToDBs, getNumRows( "tbl_flop" ) );

	delete db;
	remove( TEST_DB_PATH.c_str() );
}


TEST( SqliteDatabase_testReplies )
{
	SqliteDatabase *db = new SqliteDatabase( TEST_DB, TEST_DIR );
	bool initSuccess = db->init( "" );
	CHECK( initSuccess );

	ReplyHandler handler( *db );

	DatabaseID dbID = 1;
	EntityTypeID entityTypeID = 1;
	GameTime timestamp = 0;
	MemoryOStream stream;

	int numWriteToDBs = 1000;

	for (int i = 0; i < numWriteToDBs; i++)
	{
		stream << "blob";
		db->writeToDB( dbID, entityTypeID, timestamp, stream,
				createReplyStruct( &handler ) );
	}

	db->commit();

	CHECK( handler.waitForReplies( numWriteToDBs, 60 ) );
	delete db;
	remove( TEST_DB_PATH.c_str() );
}

BW_END_NAMESPACE

// test_sqlite_database.cpp
