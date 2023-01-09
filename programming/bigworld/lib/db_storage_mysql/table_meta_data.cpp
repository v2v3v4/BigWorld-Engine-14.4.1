#include "table_meta_data.hpp"

#include "query.hpp"
#include "result_set.hpp"

#include "cstdmf/debug.hpp"

#include <sstream>

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

/**
 *	This function generates an index name based on column name. The name
 * 	of the index isn't really all that important but it's nice to have some
 * 	consistency.
 */
BW::string generateIndexName( const BW::string & colName )
{
	BW::string::size_type underscorePos = colName.find( '_' );

	return ((underscorePos == BW::string::npos) ?
			colName :
			colName.substr( underscorePos + 1 )) + 
		"Index";
}


// -----------------------------------------------------------------------------
// Section: Table meta data
// -----------------------------------------------------------------------------


/**
 *	This class represents an column index.
 */
class TableIndex
{
public:

	/**
	 *	Constructor.
	 */
	TableIndex():
			columnName_(),
			isUnique_( false )
	{}


	/**
	 *	Constructor.
	 *
	 *	@param columnName 	The name of the column this index refers to.
	 *	@param isUnique 	Whether this is a unique index or a non-unique index.
	 */ 
	TableIndex( const BW::string & columnName, bool isUnique ):
			columnName_( columnName ),
			isUnique_( isUnique )
	{}


	/**
	 *	This method returns this index's column name.
	 *
	 *	@return The index's column name.
	 */
	const BW::string & columnName() const
		{ return columnName_; }


	/**
	 *	This method returns whether this index is unique or non-unique.
	 *
	 *	@return True if this index is unique, false otherwise.
 	 */
	bool isUnique() const 
		{ return isUnique_; }

private:
	BW::string columnName_;
	bool isUnique_;
};



/**
 *	This class executes the "SHOW INDEX" query on a table and stores the
 * 	list of index names
 */
// NOTE: Can't be in anonymous namespace because forward declared in header
class TableIndices
{
public:
	TableIndices( MySql & connection, const BW::string & tableName );

	const BW::string * getIndexName( const BW::string & colName ) const
	{
		ColumnToIndexMap::const_iterator found = colToIndexMap_.find( colName );

		return (found != colToIndexMap_.end()) ? 
			&found->second.columnName() : NULL;
	}

private:
	// Maps column names to index name.

	typedef BW::map< BW::string, TableIndex > ColumnToIndexMap;
	ColumnToIndexMap	colToIndexMap_;
};


/**
 *	Constructor. Retrieves the index information for the given table.
 */
TableIndices::TableIndices( MySql & connection, const BW::string & tableName )
{
	// Get index info
	BW::stringstream queryStr;
	queryStr << "SHOW INDEX FROM " << tableName;
	Query query( queryStr.str() );

	ResultSet resultSet;
	query.execute( connection, &resultSet );

	ResultRow resultRow;

	while (resultRow.fetchNextFrom( resultSet ))
	{
		BW::string nonUnique;
		BW::string columnName;
		BW::string keyName;

		resultRow.getField( 1, nonUnique );
		resultRow.getField( 2, keyName );
		resultRow.getField( 4, columnName );

		bool isUnique = (nonUnique == "0");

		// Build column name to index name map. Assume no multi-column index.
		colToIndexMap_[ columnName ] = TableIndex( keyName, isUnique );
	}
}


/**
 * 	Constructor. Initialises from a MYSQL_FIELD and TableIndices.
 */
TableMetaData::ColumnInfo::ColumnInfo( const MYSQL_FIELD & field,
		const TableIndices & indices ) :
	columnType( field ), 
	indexType( deriveIndexType( field, indices ) )
{
}


/**
 * Default constructor. Required for insertion into BW::map
 */
TableMetaData::ColumnInfo::ColumnInfo() :
	columnType( MYSQL_TYPE_NULL, false, 0, BW::string() ),
	indexType( INDEX_TYPE_NONE )
{}


/**
 *	Returns true if indices are equivalent.
 */
bool TableMetaData::ColumnInfo::isIndexEqual(
	const TableMetaData::ColumnInfo & other ) const
{
	// Consider externally created indexes the same as BW generated indexes.
	// Presume it was a necessity and this is a production DB.
	if (this->indexType == INDEX_TYPE_EXTERNAL)
	{
		return other.indexType == INDEX_TYPE_UNIQUE ||
			other.indexType == INDEX_TYPE_NON_UNIQUE;
	}

	if (other.indexType == INDEX_TYPE_EXTERNAL)
	{
		return this->indexType == INDEX_TYPE_UNIQUE ||
			this->indexType == INDEX_TYPE_NON_UNIQUE;
	}

	return this->indexType == other.indexType;
}


/**
 *	Returns the correct IMySqlColumnMapping::IndexType based on the information
 * 	in MYSQL_FIELD and TableIndices.
 */
ColumnIndexType TableMetaData::ColumnInfo::deriveIndexType(
		const MYSQL_FIELD & field, const TableIndices & indices )
{
	if (field.flags & PRI_KEY_FLAG)
	{
		return INDEX_TYPE_PRIMARY;
	}
	else if (field.flags & UNIQUE_KEY_FLAG)
	{
		const BW::string * pIndexName = indices.getIndexName( field.name );
		MF_ASSERT( pIndexName );

		if (*pIndexName == generateIndexName( field.name ))
		{
			return INDEX_TYPE_UNIQUE;
		}
		else
		{
			WARNING_MSG( "TableMetaData::ColumnInfo::deriveIndexType: "
					"Found unknown unique index %s for column %s\n",
					pIndexName->c_str(), field.name );

			return INDEX_TYPE_EXTERNAL;
		}
	}
	else if (field.flags & MULTIPLE_KEY_FLAG)
	{
		const BW::string * pIndexName = indices.getIndexName( field.name );
		MF_ASSERT( pIndexName );

		if (*pIndexName == PARENTID_INDEX_NAME)
		{
			return INDEX_TYPE_PARENT_ID;
		}
		else if (*pIndexName == generateIndexName( field.name ))
		{
			return INDEX_TYPE_NON_UNIQUE;
		}
		else
		{
			WARNING_MSG( "TableMetaData::ColumnInfo::deriveIndexType: "
					"Found unknown multiple key index %s for column %s\n",
					pIndexName->c_str(), field.name );

			return INDEX_TYPE_EXTERNAL;
		}
	}

	return INDEX_TYPE_NONE;
}


/**
 *	Retrieves all the names of entity tables currently in the database.
 *
 * 	@param tables	This parameter receives the list of tables.
 * 	@param connection	The MySQL connection to query on.
 */
void TableMetaData::getEntityTables( StrSet & tables, MySql & connection )
{
	const Query query( "SHOW TABLES LIKE '" TABLE_NAME_PREFIX "_%'" );

	ResultSet resultSet;
	query.execute( connection, &resultSet );

	BW::string tableName;

	while (resultSet.getResult( tableName ))
	{
		tables.insert( tableName );
	}
}


/**
 * 	Retrieves meta data of all the columns for a given table.
 *
 * 	@param	columns	This output parameter receives the list of columns.
 * 		The key is the column name and the data is the column type.
 * 	@param connection	The MySQL connection to use for retrieving meta data.
 * 	@param	tableName	The name of the table to get the columns for.
 */
void TableMetaData::getTableColumns( TableMetaData::NameToColInfoMap & columns,
		MySql & connection, const BW::string & tableName )
{
	MySqlTableMetadata	tableMetadata( connection, tableName );
	if (tableMetadata.isValid())	// table exists
	{
		TableIndices tableIndices( connection, tableName );
		for (unsigned int i = 0; i < tableMetadata.getNumFields(); i++)
		{
			const MYSQL_FIELD& field = tableMetadata.getField( i );
			columns[ field.name ] =
				TableMetaData::ColumnInfo( field, tableIndices );
		}
	}
}


// -----------------------------------------------------------------------------
// Section: class MySqlTableMetadata
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
MySqlTableMetadata::MySqlTableMetadata( MySql & connection, 
		const BW::string tableName ) :
	result_( mysql_list_fields( connection.get(), tableName.c_str(), NULL ) )
{
	if (result_)
	{
		numFields_ = mysql_num_fields( result_ );
		fields_ = mysql_fetch_fields( result_ );
	}
	else
	{
		numFields_ = 0;
		fields_ = NULL;
	}
}


/**
 *	Destructor.
 */
MySqlTableMetadata::~MySqlTableMetadata()
{
	if (result_)
	{
		mysql_free_result( result_ );
	}
}

BW_END_NAMESPACE

// mysql_table_meta_data.cpp
