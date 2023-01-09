#include "script/first_include.hpp"

#include "mysql_table_initialiser.hpp"

#include "db_storage_mysql/database_exception.hpp"
#include "db_storage_mysql/helper_types.hpp"

#include <exception>


BW_BEGIN_NAMESPACE

/**
 *	This class is used to help with performing an ALTER TABLE command on an
 *	existing table in the database.
 *
 *	It collects all the column additions, drops and modification requests
 *	and prints debugging information prior to execution. The collection of all
 *	operations into a single SQL command allows for a single fast DB operation
 *	rather than many slow operations.
 */
class AlterTableHelper
{
public:
	AlterTableHelper( const BW::string tableName );

	void addColumn( const BW::string & columnName,
					const BW::string & columnType );

	void dropColumn( const BW::string & columnName );

	void modifyColumn( const BW::string & columnName,
						const BW::string & columnType );

	uint execute( MySql & connection, bool isVerbose = false );

	uint numAlteredItems() const
		{ return numAdditions_ + numDrops_ + numModifications_; }

private:
	BW::string tableName_;

	uint numAdditions_;
	uint numDrops_;
	uint numModifications_;

	BW::stringstream stringStream_;
};


/**
 *	Constructor.
 *
 *	@param tableName  The name of the table the ALTER TABLE command will
 *	                  operate on.
 */
AlterTableHelper::AlterTableHelper( const BW::string tableName ) :
	tableName_( tableName ),
	numAdditions_( 0 ),
	numDrops_( 0 ),
	numModifications_( 0 )
{
	stringStream_ << "ALTER TABLE " << tableName;
}


/**
 *	This method appends an ADD COLUMN request to the ALTER TABLE command.
 *
 *	@param columnName  The name of the column to add.
 *	@param columnType  The DB column type of the column to add (eg, INT)
 */
void AlterTableHelper::addColumn( const BW::string & columnName,
	const BW::string & columnType )
{
	if (this->numAlteredItems())
	{
		stringStream_ << ",";
	}

	stringStream_ << " ADD COLUMN " << columnName << " " << columnType;

	++numAdditions_;
}


/**
 *	This method appends a DROP COLUMN request to the ALTER TABLE command.
 *
 *	@param columnName  The name of the column to drop.
 */
void AlterTableHelper::dropColumn( const BW::string & columnName )
{
	if (this->numAlteredItems())
	{
		stringStream_ << ",";
	}

	stringStream_ << " DROP COLUMN " << columnName;

	++numDrops_;
}


/**
 *	This method appends a MODIFY COLUMN request to the ALTER TABLE command.
 *
 *	@param columnName  The name of the column to modify.
 *	@param columnType  The DB column type the column will be changed to
 *	                   (eg, INT).
 */
void AlterTableHelper::modifyColumn( const BW::string & columnName,
	const BW::string & columnType )
{
	if (this->numAlteredItems())
	{
		stringStream_ << ",";
	}

	stringStream_ << " MODIFY COLUMN " << columnName << " " << columnType;

	++numModifications_;
}


/**
 *	This method performs the ALTER TABLE on the provided MySQL connection.
 *
 *	@param connection  The MySql connection to perform the ALTER TABLE on.
 *	@param isVerbose   Flag indicating whether to log more information or not.
 *
 *	@returns The number of columns that were added, deleted and modified.
 */
uint AlterTableHelper::execute( MySql & connection, bool isVerbose )
{
	if (this->numAlteredItems())
	{
		if (isVerbose)
		{
			INFO_MSG( "\tAdding %u columns into table %s\n",
						numAdditions_, tableName_.c_str() );
			INFO_MSG( "\tDeleting %u columns from table %s\n",
						numDrops_, tableName_.c_str() );
			INFO_MSG( "\tUpdating %u columns from table %s\n",
						numModifications_, tableName_.c_str() );

			DEBUG_MSG( "SQL: %s\n", stringStream_.str().c_str() );
		}
		connection.execute( stringStream_.str() );
	}

	return this->numAlteredItems();
}



/**
 *	Constructor.
 *
 *	@param con       The MySql connection to use for initialising.
 *	@param allowNew  Should we allow new column and tables be created.
 */
TableInitialiser::TableInitialiser( MySql & con, bool allowNew,
		const BW::string & characterSet, const BW::string & collation ) :
	TableInspector( con ),
	allowNew_( allowNew ),
	characterSet_( characterSet ),
	collation_( collation )
{
}


/**
 *	This method generates the SQL statement required to create an index for the
 *	given column.
 */
const BW::string TableInitialiser::generateCreateIndexStatement( 
		const BW::string & tableName, const BW::string & colName, 
		const TableMetaData::ColumnInfo & colInfo )
{
	BW::string indexName = generateIndexName( colName );

	BW::ostringstream oss;
	oss << "CREATE ";

	if (colInfo.indexType == INDEX_TYPE_UNIQUE)
	{
		oss << "UNIQUE ";
	}

	oss << "INDEX " << indexName << " ON " << tableName << 
		" (" << colName;

	// Only need to add index length for blob and string types.
	int indexLength = colInfo.columnType.length;
	int fieldType = colInfo.columnType.fieldType;

	// Note that all indexes on properties that are not VARCHAR fields
	// are only indexed on a maximum of the first 255 characters.
	if (fieldType != MYSQL_TYPE_VAR_STRING)
	{
		// MySQL's indexing length limit for TEXT and BLOB field types.
		static const int MAX_TEXT_BLOB_INDEX_LENGTH = 255;

		if (indexLength > MAX_TEXT_BLOB_INDEX_LENGTH)
		{
			WARNING_MSG( "Column %s on table %s can only be indexed up "
					"to the first %d characters "
					"(column is of length %d).\n",
				colName.c_str(), 
				tableName.c_str(), 
				MAX_TEXT_BLOB_INDEX_LENGTH,
				colInfo.columnType.length );

			indexLength = MAX_TEXT_BLOB_INDEX_LENGTH;
		}
	}

	switch (fieldType)
	{
		case MYSQL_TYPE_VARCHAR:
		case MYSQL_TYPE_TINY_BLOB:
		case MYSQL_TYPE_MEDIUM_BLOB:
		case MYSQL_TYPE_LONG_BLOB:
		case MYSQL_TYPE_BLOB:
		case MYSQL_TYPE_VAR_STRING:
		case MYSQL_TYPE_STRING:

			oss << "(" << indexLength << ")";
			break;
	}

	oss << ")";

	return oss.str();
}


/**
 *	This method creates an index on the given column in the given table
 * 	according to colInfo.indexType. Most of the time this would be
 * 	IndexTypeNone.
 */
void TableInitialiser::createIndex( const BW::string & tableName,
	const BW::string & colName,
	const TableMetaData::ColumnInfo & colInfo )
{
	switch (colInfo.indexType)
	{
	case INDEX_TYPE_NONE:
		break;

	case INDEX_TYPE_PRIMARY:
		// A bit dodgy, but this is created when we create the table
		// and it cannot be added or deleted afterwards.
		break;

	case INDEX_TYPE_UNIQUE:
	case INDEX_TYPE_NON_UNIQUE:
	{
		const BW::string sql = 
			this->generateCreateIndexStatement( tableName, colName, colInfo );

		try
		{
			connection_.execute( sql );
		}
		catch (DatabaseException & dbException)
		{
			if (colInfo.indexType != INDEX_TYPE_NON_UNIQUE)
			{
				ERROR_MSG( "TableInitialiser::createIndex: Failed to "
						"create unique index on column '%s.%s'. Please "
						"ensure that all values in the column are unique "
						"before attempting to create a name index.\n",
					tableName.c_str(), colName.c_str() );
			}
			else
			{
				ERROR_MSG( "TableInitialiser::createIndex: Failed to "
						"create non-unique index on column '%s.%s: %s'.\n",
					tableName.c_str(), colName.c_str(), dbException.what() );
			}
			throw;
		}
	}
	break;

	case INDEX_TYPE_PARENT_ID:
		connection_.execute( "CREATE INDEX " PARENTID_INDEX_NAME " ON " +
				tableName + " (" + colName + ")" );
		break;

	case INDEX_TYPE_EXTERNAL: // Can only be detected not created.
	default:
		CRITICAL_MSG( "createEntityTableIndex: Unknown index type %d\n",
				colInfo.indexType );
		break;
	}
}


/**
 *	This method deletes an index in the given table according to indexType.
 *	This is the evil twin of createEntityTableIndex().
 *
 * 	@param tableName  The name of the table to remove the index from.
 * 	@param colName    The name of the column the index is associated with.
 * 	@param indexType  The type of index to remove.
 */
void TableInitialiser::removeIndex( const BW::string & tableName,
	const BW::string & colName,
	ColumnIndexType indexType )
{
	try
	{
		switch (indexType)
		{
			case INDEX_TYPE_NONE:
				break;
			case INDEX_TYPE_PRIMARY:
				// Can't delete primary index.
				break;
			case INDEX_TYPE_EXTERNAL:
				// Don't delete externally created indexes.
				break;
			case INDEX_TYPE_UNIQUE:
			case INDEX_TYPE_NON_UNIQUE:
				{
					BW::string indexName = generateIndexName( colName );

					connection_.execute( "ALTER TABLE " + tableName +
							" DROP INDEX " + indexName );
				}
				break;
			case INDEX_TYPE_PARENT_ID:
				connection_.execute( "ALTER TABLE " + tableName +
						" DROP INDEX " PARENTID_INDEX_NAME );
				break;
			default:
				CRITICAL_MSG( "TableInitialiser::removeIndex: "
						"Unknown index type %d\n", indexType );
				break;
		}
	}
	catch (std::exception& e)
	{
		// Shouldn't really happen, but it's not fatal so we shouldn't die.
		ERROR_MSG( "TableInitialiser::removeIndex: %s\n", e.what() );
	}
}


/*
 *	Override from TableInspector.
 */
bool TableInitialiser::onNeedNewTable( const BW::string & tableName,
		const TableMetaData::NameToColInfoMap & columns )
{
	if (!allowNew_)
	{
		NOTICE_MSG( "\tWitholding table creation: %s\n", tableName.c_str() );
		return true;
	}

	INFO_MSG("\tCreating table %s\n", tableName.c_str());
	connection_.execute( "CREATE TABLE IF NOT EXISTS " + tableName +
			" (id BIGINT AUTO_INCREMENT, PRIMARY KEY idKey (id)) "
			"ENGINE=" MYSQL_ENGINE_TYPE );

	// __kyl__ (28/7/2005) We can't create a table with no columns so
	// we create one with the id column even though it may not be
	// needed. We'll delete the id column later on.
	bool deleteIDCol = false;
	TableMetaData::NameToColInfoMap newColumns( columns );
	TableMetaData::NameToColInfoMap::iterator idIter =
			newColumns.find( ID_COLUMN_NAME );
	if (idIter != newColumns.end())
	{
		newColumns.erase( idIter );
	}
	else
	{
		deleteIDCol = true;
	}

	AlterTableHelper alterTableHelper( tableName );

	this->addColumns( tableName, newColumns, alterTableHelper, false );

	// delete unnecessary ID column that we created table with.
	if (deleteIDCol)
	{
		alterTableHelper.dropColumn( ID_COLUMN_NAME );
	}

	if (alterTableHelper.execute( connection_ ))
	{
		this->initialiseColumns( tableName, newColumns, true );
	}

	return true;
}


/*
 *	Override from TableInspector.
 */
bool TableInitialiser::onExistingTable( const BW::string & tableName )
{
	if (characterSet_.empty())
	{
		// If not set, leave as-is.
		return true;
	}

	BW::stringstream ss;
	ss << "ALTER TABLE " << tableName <<
			" CONVERT TO CHARACTER SET " <<
			MySqlEscapedString( connection_, characterSet_ );

	if (!collation_.empty())
	{
		ss << " COLLATE " << MySqlEscapedString( connection_, collation_ );
	}

	connection_.execute( ss.str() );

	return true;
}


/*
 *	Override from TableInspector.
 */
bool TableInitialiser::onNeedUpdateTable( const BW::string & tableName,
		const TableMetaData::NameToColInfoMap & obsoleteColumns,
		const TableMetaData::NameToColInfoMap & newColumns,
		const TableMetaData::NameToColInfoMap & updatedColumns,
		const TableMetaData::NameToIdxColInfoMap & indexedColumns )
{
	AlterTableHelper alterTableHelper( tableName );

	if (allowNew_)
	{
		this->addColumns( tableName, newColumns, alterTableHelper, true );
	}
	this->dropColumns( tableName, obsoleteColumns, alterTableHelper, true );
	this->updateColumns( tableName, updatedColumns, alterTableHelper, true );
	alterTableHelper.execute( connection_, /*isVerbose*/ true );

	this->indexColumns( tableName, indexedColumns );

	return true;
}


/**
 *	This method initialises all the columns in the provided list.
 *
 *	@param tableName  The name of the table to initialise.
 *	@param columns    The column descriptions to initialise the table with.
 *	@param shouldApplyDefaultValue  Flag to indicate whether the columns should
 *	                                be initialised with the default type value.
 */
template < class COLUMN_MAP >
void TableInitialiser::initialiseColumns( const BW::string & tableName,
	COLUMN_MAP & columns, bool shouldApplyDefaultValue )
{
	typename COLUMN_MAP::const_iterator iter = columns.begin();
	while (iter != columns.end())
	{
		this->initialiseColumn( tableName, iter->first, iter->second,
									shouldApplyDefaultValue );
		++iter;
	}
}


/**
 *	This method creates any required indexes and sets the default column value
 *	for the column if MySQL doesn't support DEFAULT for that type (eg: BLOB).
 *
 *	@see initialiseColumns.
 *
 *	@param tableName   The name of the table the column to initialise is in.
 *	@param columnName  The name of the column to initialise.
 *	@param columnInfo  The full column description.
 *	@param shouldApplyDefaultValue  Flag to indicate whether the columns should
 *	                                be initialised with the default type value.
 */
void TableInitialiser::initialiseColumn( const BW::string & tableName,
	const BW::string & columnName,
	const TableMetaData::ColumnInfo & columnInfo,
	bool shouldApplyDefaultValue )
{
	this->createIndex( tableName, columnName, columnInfo );

	// For any columns that were unable to have a default value applied
	// to the column description, update the columns now with the
	// default data.
	if (shouldApplyDefaultValue &&
		!columnInfo.columnType.isDefaultValueSupported())
	{
		BW::stringstream ss;
		ss << "UPDATE " << tableName << " SET " << columnName
			<< "='" << MySqlEscapedString( connection_,
						columnInfo.columnType.defaultValue )
			<< '\'';
		connection_.execute( ss.str() );
	}
}


/*
 *	Override from TableInspector.
 */
bool TableInitialiser::onNeedDeleteTables( const StrSet & tableNames )
{
	StrSet::const_iterator iter = tableNames.begin();
	while (iter != tableNames.end())
	{
		INFO_MSG( "\tDeleting table %s\n", iter->c_str() );
		connection_.execute( "DROP TABLE " + *iter );
		++iter;
	}

	return true;
}


/**
 *	This method adds columns to an existing table.
 *
 *	@param tableName  The name of the table to add the columns into.
 *	@param columns    The set of columns to add to the table.
 *	@param helper     The helper to use for optimising the column additions.
 *	@param shouldPrintInfo  Flag indicating how verbose to be while working.
 */
void TableInitialiser::addColumns( const BW::string & tableName,
		const TableMetaData::NameToColInfoMap & columns,
		AlterTableHelper & helper, bool shouldPrintInfo )
{
	TableMetaData::NameToColInfoMap::const_iterator iter = columns.begin();
	while (iter != columns.end())
	{
		if (shouldPrintInfo)
		{
			TRACE_MSG( "\tPreparing to update table %s, adding column %s\n",
						tableName.c_str(), iter->first.c_str() );
		}

		helper.addColumn( iter->first,
					iter->second.columnType.getAsString(
						connection_, iter->second.indexType ) );

		++iter;
	}

	return;
}


/**
 *	This method removes columns from an existing table.
 *
 *	@param tableName  The name of the table to remove the columns from.
 *	@param columns    The set of columns to remove from the table.
 *	@param helper  The helper to use for optimising the column removal.
 *	@param shouldPrintInfo  Flag indicating how verbose to be while working.
 */
void TableInitialiser::dropColumns( const BW::string & tableName,
		const TableMetaData::NameToColInfoMap & columns,
		AlterTableHelper & helper, bool shouldPrintInfo )
{

	// Prepare the columns to be dropped
	TableMetaData::NameToColInfoMap::const_iterator iter = columns.begin();
	while (iter != columns.end())
	{
		if (shouldPrintInfo)
		{
			TRACE_MSG( "\tPreparing to update table %s, deleting column %s\n",
					tableName.c_str(), iter->first.c_str() );
		}

		this->removeIndex( tableName, iter->first, iter->second.indexType );

		helper.dropColumn( iter->first );

		++iter;
	}
}


/**
 *	This method modifies existing columns in a table if the associated entity
 *	definition has changed.
 *
 *	@param tableName  The name of the table to modify columns in.
 *	@param columns    The set of columns to modify.
 *	@param helper  The helper to use for optimising the column modification.
 *	@param shouldPrintInfo  Flag indicating how verbose to be while working.
 */
void TableInitialiser::updateColumns( const BW::string & tableName,
		const TableMetaData::NameToColInfoMap & columns,
		AlterTableHelper & helper, bool shouldPrintInfo )
{
	// Update changed columns
	TableMetaData::NameToColInfoMap::const_iterator iter = columns.begin();
	while (iter != columns.end())
	{
		BW::string columnTypeStr =
				iter->second.columnType.getAsString( connection_,
						iter->second.indexType );

		if (shouldPrintInfo)
		{
			TRACE_MSG( "\tPreparing to update table %s, modifying type of "
						"column %s to %s\n", tableName.c_str(),
				iter->first.c_str(), columnTypeStr.c_str() );
		}

		helper.modifyColumn( iter->first, columnTypeStr );

		++iter;
	}
}


/**
 *	This method indexes columns in a table if the associated entity
 *	definition has changed.
 *
 *	@param tableName  The name of the table to modify columns in.
 *	@param columns    The set of columns to index.
 */
void TableInitialiser::indexColumns( const BW::string & tableName,
		const TableMetaData::NameToIdxColInfoMap & columns )
{
	TableMetaData::NameToIdxColInfoMap::const_iterator iter = columns.begin();

	while (iter != columns.end())
	{
		bool isCreate = iter->second.indexType != INDEX_TYPE_NONE;

		TRACE_MSG( "\tPreparing to %s index for table %s column %s\n",
			isCreate ? "add": "delete", tableName.c_str(),
			iter->first.c_str() );

		if (isCreate)
		{
			this->createIndex( tableName, iter->first, iter->second );
		}
		else
		{
			this->removeIndex( tableName, iter->first, iter->second.oldIndexType );
		}

		++iter;
	}
}

BW_END_NAMESPACE

// mysql_table_initialiser.cpp
