#include "secondary_database_table.hpp"

#include "consolidation_progress_reporter.hpp"
#include "primary_database_update_queue.hpp"
#include "secondary_database.hpp"

#include "db_storage/entity_key.hpp"

#include "sqlite/sqlite_statement.hpp"

#include <sstream>

DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )

BW_BEGIN_NAMESPACE

namespace // anonymous
{

enum TableColumns
{
	COLUMN_DATABASE_ID,
	COLUMN_TYPE_ID,
	COLUMN_TIME,
	COLUMN_BLOB
};

} // end namespace (anonymous)


/**
 *	Constructor.
 */
SecondaryDatabaseTable::SecondaryDatabaseTable( 
			SecondaryDatabase & database, const BW::string & tableName ):
		database_( database ),
		tableName_( tableName ),
		pGetDataQuery_( NULL ),
		pGetNumRowsQuery_( NULL )
{}


/**
 *	Initialise this instance.
 *
 *	@returns 		true if successful, false otherwise.
 */
bool SecondaryDatabaseTable::init()
{
	// pGetDataQuery
	{
		static const char * SELECT_DATA_QUERY = 
			"SELECT sm_dbID, sm_typeID, sm_time, sm_blob FROM ";

		BW::ostringstream oss;

		oss << SELECT_DATA_QUERY << tableName_;

		int result = 0;
		pGetDataQuery_.reset( new SqliteStatement( database_.connection(),
			oss.str(), result ) );

		if (result != SQLITE_OK)
		{
			pGetDataQuery_.reset();
			ERROR_MSG( "SecondaryDatabaseTable::init: "
					"Could not create select query for table \"%s\"\n",
				tableName_.c_str() );
			return false;
		}
	}

	// pGetNumRowsQuery
	{
		BW::ostringstream oss;
		oss << "SELECT COUNT(*) FROM " << tableName_;

		int result = 0;
		pGetNumRowsQuery_.reset( new SqliteStatement( database_.connection(),
			oss.str(), result ) );

		if (result != SQLITE_OK)
		{
			pGetNumRowsQuery_.reset();
			ERROR_MSG( "SecondaryDatabaseTable::init: "
					"Could not create num rows query for table \"%s\"\n",
				tableName_.c_str() );
			return false;
		}
	}

	return true;
}


/**
 *	Destructor.
 */
SecondaryDatabaseTable::~SecondaryDatabaseTable()
{}


/**
 *	Query for the number of rows in this table.
 */
int SecondaryDatabaseTable::numRows()
{
	pGetNumRowsQuery_->reset();
	MF_VERIFY( pGetNumRowsQuery_->step() == SQLITE_ROW );

	return pGetNumRowsQuery_->intColumn( 0 );
}


/**
 *	Returns the game time value for the first row in this table, or 0 if no
 *	rows were present.
 */
GameTime SecondaryDatabaseTable::firstGameTime()
{
	pGetDataQuery_->reset();

	int result = pGetDataQuery_->step();

	if (result == SQLITE_DONE)
	{
		// No rows, return 0.
		return 0;
	}

	MF_VERIFY( result == SQLITE_ROW );

	return pGetDataQuery_->intColumn( COLUMN_TIME );
}


/**
 *	Consolidate the data in this database table into the primary database.
 */
bool SecondaryDatabaseTable::consolidate( 
		PrimaryDatabaseUpdateQueue & primaryQueue,
		ConsolidationProgressReporter & progressReporter,
		bool & shouldAbort )
{
	pGetDataQuery_->reset();

	int stepRes = SQLITE_ERROR;
	while (!shouldAbort &&
			((stepRes = pGetDataQuery_->step()) == SQLITE_ROW) )
	{
		// Do this at the start because of various "continue" statements.
		progressReporter.onConsolidatedRow();

		// Read row data
		DatabaseID dbID = pGetDataQuery_->int64Column( COLUMN_DATABASE_ID );
		EntityTypeID typeID =
			EntityTypeID( pGetDataQuery_->intColumn( COLUMN_TYPE_ID ) );
		GameTime time = pGetDataQuery_->intColumn( COLUMN_TIME );

		// Check if we've already written a newer version of this entity.
		EntityKey	entityKey( typeID, dbID );
		int dataSize;
		const void * dataBlob = pGetDataQuery_->blobColumn( COLUMN_BLOB, 
			&dataSize );

		MemoryIStream data( dataBlob, dataSize );

		primaryQueue.addUpdate( EntityKey( typeID, dbID ), data, time );

		if (primaryQueue.hasError())
		{
			// Note that this error is most likely not due to the entity we've
			// just added to the queue but due to a previous entity.
			return false;
		}
	}

	bool isOK = (stepRes == SQLITE_DONE);

	if (!isOK && !shouldAbort)
	{
		ERROR_MSG( "ConsolidateDBsApp::consolidateSecondaryDBTable: "
				"SQLite error: %s\n", 
			database_.connection().lastError() );
	}

	return isOK;
}

BW_END_NAMESPACE

// secondary_database_table.cpp
