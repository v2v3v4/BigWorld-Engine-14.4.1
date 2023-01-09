#include "secondary_database.hpp"

#include "primary_database_update_queue.hpp"
#include "secondary_database_table.hpp"

#include "sqlite/sqlite_connection.hpp"
#include "sqlite/sqlite_statement.hpp"

#define CHECKSUM_TABLE_NAME		"tbl_checksum"
#define CHECKSUM_COLUMN_NAME 	"sm_checksum"

#include <sstream>

DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )

BW_BEGIN_NAMESPACE

namespace // anonymous
{
bool tableAgeComparator( const shared_ptr< SecondaryDatabaseTable > & t1, 
		const shared_ptr< SecondaryDatabaseTable > & t2 )
{
	return t1->firstGameTime() < t2->firstGameTime();
}

} // end namespace (anonymous)

/**
 *	Constructor.
 */
SecondaryDatabase::SecondaryDatabase() :
		path_(),
		pConnection_( NULL ),
		tables_(),
		numEntities_( 0 )
{

}

/**
 *	Destructor.
 */
SecondaryDatabase::~SecondaryDatabase()
{}


/**
 *	Initialise the instance.
 */
bool SecondaryDatabase::init( const BW::string & path )
{
	pConnection_.reset( new SqliteConnection );

	if (!pConnection_->open( path ))
	{
		ERROR_MSG( "SecondaryDatabase::init: "
				"Could not open secondary database file'%s'\n",
			path.c_str() );
		return false;
	}

	if (!this->readTables())
	{
		ERROR_MSG( "SecondaryDatabase::init: "
			"Unable to read tables from '%s'\n", path.c_str() );
		return false;	
	}

	path_ = path;

	return true;
}


/**
 *	Get the checksum digest from the secondary DB.
 */
bool SecondaryDatabase::getChecksumDigest( BW::string & digest )
{
	int result = 0;
	SqliteStatement query( *pConnection_,
			"SELECT " CHECKSUM_COLUMN_NAME " FROM " CHECKSUM_TABLE_NAME, 
		result );
	if (result != SQLITE_OK)
	{
		ERROR_MSG( "SecondaryDatabase::getChecksumDigest: "
				"Failed to open checksum table\n" );
		return false;
	}

	if (query.step() != SQLITE_ROW)
	{
		ERROR_MSG( "SecondaryDatabase::getChecksumDigest: "
				"Checksum table is empty\n" );
		return false;
	}

	digest.assign( reinterpret_cast< const char * >( query.textColumn( 0 ) ) );

	return true;
}


/**
 *	Read in the tables from the database.
 */
bool SecondaryDatabase::readTables()
{
	static const char * TABLE_NAMES[] = { "tbl_flip", "tbl_flop" };

	numEntities_ = 0;

	for (size_t i = 0; 
			i < sizeof( TABLE_NAMES ) / sizeof( TABLE_NAMES[0] ); 
			++i)
	{
		if (this->tableExists( TABLE_NAMES[i] ))
		{
			shared_ptr< SecondaryDatabaseTable > pTable(
				new SecondaryDatabaseTable( *this, TABLE_NAMES[i] ) );

			if (!pTable->init())
			{
				ERROR_MSG( "SecondaryDatabase::readTables: "
						"Failed to initialise table %s in %s\n", 
					TABLE_NAMES[i], path_.c_str() );
				return false;
			}

			tables_.push_back( pTable );

			int numRows = pTable->numRows();
			if (numRows == -1)
			{
				ERROR_MSG( "SecondaryDatabase::readTables: "
					"Invalid row set returned from secondary database '%s'.\n",
					path_.c_str() );
				return false;
			}

			numEntities_ += numRows;
		}
	}

	this->sortTablesByAge();

	return true;
}


/**
 *	Check whether a table exists in the secondary database.
 */
bool SecondaryDatabase::tableExists( const BW::string & tableName )
{
	std::ostringstream oss;

	oss << "SELECT COUNT(*) FROM " << tableName << " WHERE 0";

	int result = sqlite3_exec( pConnection_->get(), oss.str().c_str(),
		NULL, NULL, NULL );

	return result == SQLITE_OK;
}


/**
 *	Sort the statements so that the table that has older data comes first.
 */
void SecondaryDatabase::sortTablesByAge()
{
	// Have a good guess about which table is older and do the younger one
	// first.
	std::sort( tables_.begin(), tables_.end(), tableAgeComparator );
}


/**
 *	Consolidate this secondary database into the primary database.
 *
 *	@param primaryDBQueue		The primary database update queue.
 *	@param progressReporter		The consolidation progress reporter.
 */
bool SecondaryDatabase::consolidate(
		PrimaryDatabaseUpdateQueue & primaryDBQueue,
		ConsolidationProgressReporter & progressReporter,
		bool shouldIgnoreErrors,
		bool & shouldAbort )
{
	bool hasError = false;

	Tables::iterator iTable = tables_.begin();

	while ((!shouldIgnoreErrors || !hasError) && 
			iTable != tables_.end())
	{
		SecondaryDatabaseTable & table = **iTable;

		if (!table.consolidate( primaryDBQueue, progressReporter, shouldAbort ))
		{
			ERROR_MSG( "SecondaryDatabase::consolidate: "
					"Failed to consolidate table \"%s\"\n",
				table.tableName().c_str() );
			hasError = true;
		}

		++iTable;
	}

	primaryDBQueue.waitForUpdatesCompletion();

	if (primaryDBQueue.hasError())
	{
		hasError = true;
	}

	if (hasError)
	{
		ERROR_MSG( "SecondaryDatabase::consolidate: "
				"Error while consolidating '%s'\n", 
			path_.c_str() );
	}
	else
	{
		TRACE_MSG( "SecondaryDatabase::consolidate: "
				"Consolidated '%s'\n",
			path_.c_str() );
	}


	return !hasError;
}

BW_END_NAMESPACE

// secondary_database.cpp
