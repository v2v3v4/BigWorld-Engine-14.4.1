#include "script/first_include.hpp"

#include "table_inspector.hpp"

#include "database_exception.hpp"
#include "versions.hpp"

#include "mappings/entity_mapping.hpp"
#include "mappings/property_mappings_per_type.hpp"

#include "query.hpp"
#include "result_set.hpp"
#include "wrapper.hpp"

#include "db_storage/db_entitydefs.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: class TableInspector
// -----------------------------------------------------------------------------

// TableVisitor override.
bool TableInspector::onVisitTable( TableProvider & table )
{
	ColumnsCollector colCol;
	table.visitIDColumnWith( colCol );
	table.visitColumnsWith( colCol );

	// Alias for required columns.
	TableMetaData::NameToColInfoMap & newColumns =  colCol.getColumnsInfo();
	const BW::string& tableName( table.getTableName() );

	// Check it is not duplicate table
	if (!visitedTables_.insert( tableName ).second)
	{
		throw std::runtime_error( ("table " + tableName +
				" requested twice").c_str() );
	}

	// Get existing table columns.
	TableMetaData::NameToColInfoMap oldColumns;
	TableMetaData::getTableColumns( oldColumns, connection_, tableName );

	if (oldColumns.size() == 0)	// No existing table
	{
		isSynced_ &= this->onNeedNewTable( tableName, colCol.getColumnsInfo() );
	}
	else
	{
		isSynced_ &= this->onExistingTable( tableName );

		// Check difference between required and actual columns
		TableMetaData::NameToColInfoMap updatedColumns;
		TableMetaData::NameToIdxColInfoMap indexedColumns;

		this->classifyColumns( oldColumns, newColumns, updatedColumns,
			indexedColumns );

		if (!oldColumns.empty() || !newColumns.empty() ||
			!updatedColumns.empty() || !indexedColumns.empty())
		{
			isSynced_ &= this->onNeedUpdateTable(
				tableName, oldColumns, newColumns, updatedColumns, indexedColumns );
		}
	}

	return true;	// Continue to visit the next table.
}


/**
 *	This method removes the tables in the database that are were not visited.
 *
 *	@returns true if the unvisited tables have been deleted, false otherwise.
 */
bool TableInspector::deleteUnvisitedTables()
{
	StrSet obsoleteTables;
	{
		StrSet existingTables;
		TableMetaData::getEntityTables( existingTables, connection_ );

		std::set_difference( existingTables.begin(), existingTables.end(),
					visitedTables_.begin(), visitedTables_.end(),
					std::insert_iterator<StrSet>( obsoleteTables,
							obsoleteTables.begin() ) );
	}

	bool isDeleted = true;

	if (obsoleteTables.size() > 0)
	{
		isDeleted = this->onNeedDeleteTables( obsoleteTables );
		if (!isDeleted)
		{
			isSynced_ = false;
		}
	}

	return isDeleted;
}


/**
 * 	This function separate columns into new	(needs to be added), old
 * 	(needs to be deleted) or out of date (needs	to be updated).
 *
 * 	@param	oldColumns	On input, this parameter contains the existing
 * 	columns. On output, this parameter will contain those columns that
 * 	require removal.
 * 	@param	newColumns	On input, this parameter contains the desired
 * 	columns. On output, this parameter will contain those columns that
 * 	require addition.
 * 	@param	updatedColumns	On input, this parameter should be empty. On
 * 	output, this parameter contains the columns whose type needs changing.
 * 	@param	indexedColumns	On input, this parameter should be empty. On
 * 	output, this parameter contains the columns whose index needs changing.
 */
void TableInspector::classifyColumns(
	TableMetaData::NameToColInfoMap & oldColumns,
	TableMetaData::NameToColInfoMap & newColumns,
	TableMetaData::NameToColInfoMap & updatedColumns,
	TableMetaData::NameToIdxColInfoMap & indexedColumns )
{
	typedef TableMetaData::NameToColInfoMap 	Columns;
	typedef TableMetaData::NameToIdxColInfoMap 	IndexedColumns;

	Columns::iterator oldIter = oldColumns.begin();

	while (oldIter != oldColumns.end())
	{
		Columns::iterator newIter = newColumns.find( oldIter->first );

		if (newIter == newColumns.end())
		{
			++oldIter;
			continue;
		}

		const TableMetaData::ColumnInfo & newCol = newIter->second;
		const TableMetaData::ColumnInfo & oldCol = oldIter->second;

		if (newCol.columnType != oldCol.columnType)
		{
			updatedColumns.insert(
				Columns::value_type( newIter->first,
					TableMetaData::ColumnInfo( newCol ) ) );
		}

		if (!newCol.isIndexEqual( oldCol ))
		{
			indexedColumns.insert(
				IndexedColumns::value_type( newIter->first,
					TableMetaData::IndexedColumnInfo( newCol, oldCol ) ) );
		}

		TableMetaData::NameToColInfoMap::iterator curOldIter = oldIter;
		++oldIter;
		oldColumns.erase( curOldIter );
		newColumns.erase( newIter );
	}
}


// -----------------------------------------------------------------------------
// Section: class TableValidator
// -----------------------------------------------------------------------------

/**
 *	TableInspector override.
 */
bool TableValidator::onNeedNewTable( const BW::string& tableName,
		const TableMetaData::NameToColInfoMap& columns )
{
	INFO_MSG( "\tRequire table %s\n", tableName.c_str() );

	return false;	// We didn't create the table.
}


/**
 *	TableInspector override.
 */
bool TableValidator::onNeedUpdateTable(
		const BW::string & tableName,
		const TableMetaData::NameToColInfoMap & obsoleteColumns,
		const TableMetaData::NameToColInfoMap & newColumns,
		const TableMetaData::NameToColInfoMap & updatedColumns,
		const TableMetaData::NameToIdxColInfoMap & indexedColumns )
{
	for ( TableMetaData::NameToColInfoMap::const_iterator i =
			newColumns.begin(); i != newColumns.end(); ++i )
	{
		INFO_MSG( "\tNeed to add column %s into table %s\n",
				i->first.c_str(), tableName.c_str() );
	}

	for ( TableMetaData::NameToColInfoMap::const_iterator i =
		obsoleteColumns.begin(); i != obsoleteColumns.end(); ++i )
	{
		INFO_MSG( "\tNeed to delete column %s from table %s\n",
				i->first.c_str(), tableName.c_str() );
	}

	for ( TableMetaData::NameToColInfoMap::const_iterator i =
			updatedColumns.begin(); i != updatedColumns.end(); ++i )
	{
		INFO_MSG( "\tNeed to update column %s in table %s to %s\n",
				i->first.c_str(), tableName.c_str(),
				i->second.columnType.getAsString( connection_,
						i->second.indexType ).c_str() );
	}

	for ( TableMetaData::NameToIdxColInfoMap::const_iterator i =
			indexedColumns.begin(); i != indexedColumns.end(); ++i )
	{
		INFO_MSG( "\tNeed to %s index to column %s in table %s\n",
			(i->second.indexType == INDEX_TYPE_NONE) ? "delete" : "add",
				i->first.c_str(), tableName.c_str() );
	}

	// We didn't update the table
	return false;
}


/**
 *	TableInspector override.
 */
bool TableValidator::onNeedDeleteTables( const StrSet& tableNames )
{
	for ( StrSet::const_iterator i = tableNames.begin();
			i != tableNames.end(); ++i )
	{
		INFO_MSG( "Need to remove table %s\n", i->c_str() );
	}

	// We didn't delete the tables.
	return false;
}


// -----------------------------------------------------------------------------
// Section: class TypesCollector
// -----------------------------------------------------------------------------

/**
 *	Let us know about an entity type in the entity definitions.
 */
void TypesCollector::addType( EntityTypeID bigworldID,
		const BW::string& name )
{
	bool inserted = types_.insert( name ).second;

	if (!inserted)
	{
		throw std::runtime_error( ("type " + name + " requested twice").c_str() );
	}

	EntityTypeID typeID = metaData_.getEntityTypeID( name );

	if (typeID == INVALID_ENTITY_TYPE_ID)
	{
		metaData_.addEntityType( name, bigworldID );
	}
	else if (typeID != bigworldID)
	{
		metaData_.setEntityTypeID( name, bigworldID );
	}
}


/**
 *	Remove rows in bigworldEntityTypes that are no longer in the current
 * 	entity definitions. i.e. those that were not added using addType().
 */
void TypesCollector::deleteUnwantedTypes()
{
	BW::string cleanupTypes = "DELETE FROM bigworldEntityTypes WHERE 1=1";
	for ( StrSet::const_iterator i = types_.begin(); i != types_.end(); ++i )
	{
		cleanupTypes += " AND name != '" + *i + "'";
	}
	metaData_.connection().execute( cleanupTypes );
}


// -----------------------------------------------------------------------------
// Section: Utility functions
// -----------------------------------------------------------------------------

/**
 *	This function returns the database version from the bigworldInfo table.
 */
uint32 getBigWorldDBVersion( MySql & connection )
{
	const Query getBigWorldInfoQuery( "SELECT version FROM bigworldInfo" );

	ResultSet resultSet;
	getBigWorldInfoQuery.execute( connection, &resultSet );

	uint32 version = DBAPP_CURRENT_VERSION;

	if (!resultSet.getResult( version ))
	{
		// Must be freshly created and hence current version
		BW::stringstream ss;
		ss << "INSERT INTO bigworldInfo (version) VALUES (" << version << ")";
		connection.execute( ss.str() );
	}

	return version;
}


/**
 *	This function returns the database version from the bigworldInfo table.
 */
bool getIsPasswordHashed( MySql & connection )
{
	const Query getBigWorldInfoQuery(
					"SELECT isPasswordHashed FROM bigworldInfo" );

	ResultSet resultSet;
	getBigWorldInfoQuery.execute( connection, &resultSet );

	MF_ASSERT( resultSet.numRows() > 0 );

	uint8			boundIsPasswordHashed = 0;
	resultSet.getResult( boundIsPasswordHashed );

	return (boundIsPasswordHashed != 0);
}


/**
 * 	This function returns the number of rows in the
 * 	bigworldSecondaryDatabases table.
 */
uint32 numSecondaryDBs( MySql & connection )
{
	const Query getCountQuery(
			"SELECT COUNT(*) FROM bigworldSecondaryDatabases" );

	ResultSet resultSet;
	getCountQuery.execute( connection, &resultSet );

	uint32 count = 0;
	resultSet.getResult( count );

	return count;
}


/**
 *	This function checks that all the entity tables required by DBApp exists
 * 	and have the correct columns.
 */
bool isEntityTablesInSync( MySql & connection, const EntityDefs & entityDefs )
{
	PropertyMappingsPerType types;
	if (!types.init( entityDefs ))
	{
		return false;
	}

	TableValidator tableValidator( connection );

	return types.visit( entityDefs, tableValidator );
}


/**
 *	This function checks that all the non-entity tables required by DBApp exists
 * 	and have the correct columns.
 */
bool isSpecialBigWorldTablesInSync( MySql & connection, bool isPasswordHashed )
{
	// Check version
	uint32 version;
	bool wasPasswordHashed;

	try
	{
		version = getBigWorldDBVersion( connection );
		wasPasswordHashed = getIsPasswordHashed( connection );
	}
	catch (DatabaseException & e)
	{
		ERROR_MSG( "isSpecialBigWorldTablesInSync: %s\n", e.what() );
		return false;
	}

	if (isPasswordHashed != wasPasswordHashed)
	{
		NOTICE_MSG( "Passwords were %s hashed but are now %s hashed.\n"
					"Please upgrade by running sync_db\n",
					wasPasswordHashed ? "" : "not",
					isPasswordHashed ? "" : "not" );
		return false;
	}

	if (version < DBAPP_OLDEST_SUPPORTED_VERSION)
	{
		ERROR_MSG( "Database version %u is too old. Oldest supported version "
						"is %u. Please start with a new database\n",
					version, DBAPP_OLDEST_SUPPORTED_VERSION );
		return false;
	}
	else if (version < DBAPP_CURRENT_VERSION)
	{
		NOTICE_MSG( "BigWorld database has version %u and needs to be upgraded to "
					"%u via sync_db\n",
				version, DBAPP_CURRENT_VERSION );
		return false;
	}
	else if (version > DBAPP_CURRENT_VERSION)
	{
		ERROR_MSG( "Cannot use database from newer version of BigWorld. "
						"Database version: %u. Expected %u\n",
					version, DBAPP_CURRENT_VERSION );
		return false;
	}

	// We cheat a bit. We only check that the number of columns match
	// requirements. We also allow a deprecated number of columns
	struct CheckList
	{
		BW::string		tableName;
		unsigned int	numColumns;
		unsigned int	numColumnsDeprecated;

		bool checkColumns( unsigned int count ) const
		{
			return count == numColumns || count == numColumnsDeprecated;
		}
	};

	// This checklist needs to match the tables created in the 
	// createSpecialBigWorldTables() function in sync_db.
	static const CheckList checkList[] =
	{
		{ "bigworldEntityTypes", 3, 3 },
		{ "bigworldLogOns", 7, 7 },
		{ "bigworldLogOnMapping", 4, 4 },
		{ "bigworldNewID", 1, 1 },
		{ "bigworldUsedIDs", 1, 1 },
		{ "bigworldGameTime", 1, 1 },
		{ "bigworldSpaces", 1, 1 },
		{ "bigworldSpaceData", 4, 4 },
		{ "bigworldSecondaryDatabases", 3, 4 },
		{ "bigworldEntityDefsChecksum", 1, 1 }
	};

	for (size_t i = 0; i < sizeof(checkList)/sizeof(CheckList); ++i)
	{
		MySqlTableMetadata tableMetadata( connection, checkList[i].tableName );

		if (!tableMetadata.isValid() ||
			!checkList[i].checkColumns( tableMetadata.getNumFields() ))
		{
			ERROR_MSG( "Cannot use database. Table %s should have %u columns\n",
					checkList[i].tableName.c_str(),
					checkList[i].numColumns );

			return false;
		}
	}

	return true;
}

BW_END_NAMESPACE

// mysql_table_inspector.cpp
