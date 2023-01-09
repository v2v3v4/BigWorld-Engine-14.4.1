#include "script/first_include.hpp"

#include "mysql_upgrade_database.hpp"

#include "mysql_synchronise.hpp"

#include "mysql_table_initialiser.hpp"

#include "db_storage_mysql/namer.hpp"
#include "db_storage_mysql/query.hpp"
#include "db_storage_mysql/result_set.hpp"
#include "db_storage_mysql/table_inspector.hpp"
#include "db_storage_mysql/transaction.hpp"
#include "db_storage_mysql/versions.hpp"

#include "db_storage_mysql/mappings/entity_mapping.hpp"
#include "db_storage_mysql/mappings/property_mappings_per_type.hpp"


BW_BEGIN_NAMESPACE

namespace // anonymous
{


// -----------------------------------------------------------------------------
// Section: class SimpleTableCollector
// -----------------------------------------------------------------------------

/**
 *	This class is used to visit all the entity properties so that it can
 * 	collect all the tables and columns needed. It simply accumulates all the
 * 	required tables.
 *
 *  This class is only required by upgradeDatabase1_9Snapshot.
 */
class SimpleTableCollector : public TableVisitor
{
public:
	typedef BW::map< BW::string, TableMetaData::NameToColInfoMap >
			NewTableDataMap;
	NewTableDataMap	tables_;

	virtual ~SimpleTableCollector()	{}

	const NewTableDataMap& getTables() const
	{
		return tables_;
	}

	SimpleTableCollector& operator+=( const SimpleTableCollector& rhs );

	// TableVisitor override.
	virtual bool onVisitTable( TableProvider & table );
};


/**
 *	TableVisitor override.
 */
bool SimpleTableCollector::onVisitTable( TableProvider & table )
{
	ColumnsCollector colCol;
	table.visitIDColumnWith( colCol );
	table.visitColumnsWith( colCol );

	tables_[ table.getTableName() ] = colCol.getColumnsInfo();

	return true;
}


/**
 * 	Adds the tables from the other SimpleTableCollector into this one.
 */
SimpleTableCollector & SimpleTableCollector::operator+=(
		const SimpleTableCollector & rhs )
{
	for ( NewTableDataMap::const_iterator i = rhs.tables_.begin();
			i != rhs.tables_.end(); ++i )
	{
		tables_.insert( *i );
	}
	return *this;
}


} // end namespace (anonymous)


/**
 *	Constructor.
 */
MySqlUpgradeDatabase::MySqlUpgradeDatabase( MySqlSynchronise & synchronise ):
		synchronise_( synchronise )
{
}


/**
 * 	Upgrades the database from a previous version.
 */
bool MySqlUpgradeDatabase::run( uint32 version )
{
	if (version == DBAPP_VERSION_1_7)
	{
		WARNING_MSG( "Upgrading databases from BigWorld 1.7 is no longer "
						"supported. Please use a BigWorld 1.9 release if you "
						"wish to migrate old data.\n" );
		return false;
	}

	if (version == DBAPP_VERSION_1_8)
	{
		this->upgradeDatabase1_8();
		version = DBAPP_VERSION_1_9_SNAPSHOT;
	}

	if (version == DBAPP_VERSION_1_9_SNAPSHOT)
	{
		this->upgradeDatabase1_9Snapshot();
		version = DBAPP_VERSION_1_9_NON_NULL;
	}

	if (version == DBAPP_VERSION_1_9_NON_NULL)
	{
		this->upgradeDatabase1_9NonNull();
		version = DBAPP_VERSION_SECONDARYDB;
	}

	if (version == DBAPP_VERSION_SECONDARYDB)
	{
		if (!this->upgradeDatabaseBinaryStrings())
		{
			return false;
		}

		version = DBAPP_VERSION_BINARY_STRINGS;
	}

	if (version == DBAPP_VERSION_BINARY_STRINGS)
	{
		if (!this->upgradeDatabaseLogOnMapping())
		{
			return false;
		}

		version = DBAPP_VERSION_LOGON_MAPPING;
	}

	if (version == DBAPP_VERSION_LOGON_MAPPING)
	{
		if (!this->upgradeDatabaseShouldAutoLoad())
		{
			return false;
		}

		version = DBAPP_VERSION_SHOULD_AUTO_LOAD;
	}

	if (version == DBAPP_VERSION_SHOULD_AUTO_LOAD)
	{
		if (!this->upgradeDatabaseLogOnMapping2())
		{
			return false;
		}

		version = DBAPP_VERSION_LOGON_MAPPING_2;
	}

	return true;
}


/**
 *	
 */
bool MySqlUpgradeDatabase::upgradeDatabaseLogOnMapping2()
{
	MySql & connection = synchronise_.connection();

	INFO_MSG( "Updating database's bigworldLogOnMapping table.\n" );

	connection.execute( "ALTER TABLE bigworldLogOnMapping "
			"MODIFY COLUMN entityType INT" );
	connection.execute( "ALTER TABLE bigworldLogOnMapping "
			"MODIFY COLUMN entityID BIGINT" );

	this->upgradeVersionNumber( DBAPP_VERSION_LOGON_MAPPING_2 );

	return true;
}


/**
 *	Modifies bigworldLogOns to add an extra column for auto-loading
 *	entities.
 */
bool MySqlUpgradeDatabase::upgradeDatabaseShouldAutoLoad()
{
	MySql & connection = synchronise_.connection();

	connection.execute( "ALTER TABLE bigworldLogOns "
			"ADD COLUMN shouldAutoLoad BOOL NOT NULL DEFAULT FALSE" );

	// For backwards compatibility, set the shouldAutoLoad state for each row
	// that currently exists.

	connection.execute( "UPDATE bigworldLogOns SET shouldAutoLoad = FALSE" );

	this->upgradeVersionNumber( DBAPP_VERSION_SHOULD_AUTO_LOAD );
	return true;
}


/**
 *	Upgrade the bigworldLogOnMapping table from using entity identifier strings
 *	to using the entity ID to uniquely identifier entities.
 */
bool MySqlUpgradeDatabase::upgradeDatabaseLogOnMapping()
{
	MySql & connection = synchronise_.connection();

	INFO_MSG( "Updating database's bigworldLogOnMapping table.\n" );

	bool isOkay = true;

	// Add entityID column
	connection.execute(
		"ALTER TABLE bigworldLogOnMapping "
			"ADD COLUMN entityID BIGINT NOT NULL" );

	// Find distinct entityType values
	const Query getTypesQuery(
		"SELECT DISTINCT t.bigworldID, t.typeID "
			"FROM bigworldLogOnMapping m "
				"JOIN bigworldEntityTypes t "
				"ON m.typeID = t.typeID" );

	int bwEntityTypeID;
	int dbEntityTypeID;

	ResultSet resultSet;
	getTypesQuery.execute( connection, &resultSet );

	// For each entityType, UPDATE based on JOIN with bigworldEntityTypes

	while (resultSet.getResult( bwEntityTypeID, dbEntityTypeID ))
	{
		isOkay &= this->convertRecordNameToDBID( bwEntityTypeID, dbEntityTypeID );
	}

	if (!isOkay)
	{
		connection.execute( "ALTER TABLE bigworldLogOnMapping "
								"DROP COLUMN entityID" );
		return false;
	}

	// ALTER TABLE DROP COLUMN recordName
	connection.execute( "ALTER TABLE bigworldLogOnMapping "
			"DROP COLUMN recordName" );

	// Rename typeID to entityType
	connection.execute( "ALTER TABLE bigworldLogOnMapping "
							"CHANGE COLUMN typeID entityType INT NOT NULL" );

	connection.execute( "ALTER TABLE bigworldInfo "
			"ADD COLUMN isPasswordHashed BOOL NOT NULL DEFAULT FALSE" );

	// Finally we can be satisfied that we have been upgraded.
	if (isOkay)
	{
		this->upgradeVersionNumber( DBAPP_VERSION_LOGON_MAPPING );
	}

	return isOkay;
}


/**
 *	Upgrades the database from 1.9 to unicode character encoding support.
 *
 *	Nothing needs to be done here besides bumping the version number which
 *	we do to avoid any compatibility issues.
 */
bool MySqlUpgradeDatabase::upgradeDatabaseBinaryStrings()
{
	MySql & connection = synchronise_.connection();

	INFO_MSG( "Updating database character encodings.\n" );

	// Enforce the specified charset encoding on the database. This will
	// ensure any new tables created during the entity def sync will have
	// the correct character set
	connection.execute( "ALTER DATABASE DEFAULT CHARACTER SET 'utf8'" );

	// Update entity defs. This is performed now to modify any text columns
	// from TEXT, CHAR, VARCHAR -> BLOB, BINARY, VARBINARY. This must be done
	// prior to converting tables default character set which will otherwise
	// modify any existing column data.
	bool status = synchronise_.synchroniseEntityDefinitions( false );

	// Modify all the entity tables to now have a default character set of UTF8
	typedef BW::vector< BW::string > TableList;
	TableList tables;
	char buffer[ 512 ];

	connection.getTableNames( tables, "tbl_%" );

	TableList::const_iterator iter = tables.begin();
	while (iter != tables.end())
	{
		DEBUG_MSG( "  converting to utf8: %s\n", iter->c_str() );
		bw_snprintf( buffer, sizeof( buffer ),
				 "ALTER TABLE %s CONVERT TO CHARACTER SET utf8",
			iter->c_str() );
		connection.execute( buffer );
		++iter;
	}

	bw_snprintf( buffer, sizeof( buffer ),
			"ALTER TABLE bigworldLogOnMapping MODIFY COLUMN recordName "
				"VARBINARY(%d)",
			BWMySQLMaxNamePropertyLen );
	connection.execute( buffer );

	bw_snprintf( buffer, sizeof( buffer ),
			"ALTER TABLE bigworldLogOnMapping MODIFY COLUMN logOnName "
				"VARBINARY(%d)",
			BWMySQLMaxLogOnNameLen );
	connection.execute( buffer );

	bw_snprintf( buffer, sizeof( buffer ),
			"ALTER TABLE bigworldLogOnMapping MODIFY COLUMN password "
				"VARBINARY(%d)",
			BWMySQLMaxLogOnPasswordLen );
	connection.execute( buffer );

	// Finally we can be satisfied that we have been upgraded.
	if (status)
	{
		this->upgradeVersionNumber( DBAPP_VERSION_BINARY_STRINGS );
	}

	return status;
}


/**
 *	Upgrades the database from 1.9 pre-release to 1.9.
 */
void MySqlUpgradeDatabase::upgradeDatabase1_9NonNull()
{
	MySql & connection = synchronise_.connection();

	// Don't print out something to confuse customers. 99% will go directly
	// from 1.8 to 1.9
	// INFO_MSG( "Upgrading database tables from 1.9 pre-release to 1.9\n" );

	// Drop version column from space data related tables.
	INFO_MSG( "Dropping column 'version' from tables bigworldSpaces and "
			"bigworldSpaceData\n" );
	connection.execute( "ALTER TABLE bigworldSpaces DROP COLUMN version" );
	connection.execute( "ALTER TABLE bigworldSpaceData DROP COLUMN version" );

	// Convert some tables to InnoDB
	INFO_MSG( "Converting tables bigworldSpaces, bigworldSpaceData and "
			"bigworldGameTime tables to use InnoDB engine\n" );
	connection.execute( "ALTER TABLE bigworldSpaces ENGINE=InnoDB" );
	connection.execute( "ALTER TABLE bigworldSpaceData ENGINE=InnoDB" );
	connection.execute( "ALTER TABLE bigworldGameTime ENGINE=InnoDB" );

	// Adding index to id column of bigworldSpaceData
	INFO_MSG( "Adding index to id column of bigworldSpaceData\n" );
	connection.execute( "ALTER TABLE bigworldSpaceData ADD INDEX (id)" );

	// Now done in upgradeDatabaseBinaryStrings()
	//this->upgradeVersionNumber( DBAPP_VERSION_1_9_NON_NULL );
}



/**
 *	Upgrades the database from 1.9 pre-pre-release to pre-release-1.9.
 */
void MySqlUpgradeDatabase::upgradeDatabase1_9Snapshot()
{
	// Don't print out something to confuse customers. 99% will go directly
	// from 1.8 to 1.9
	// INFO_MSG( "Upgrading database tables from 1.9 pre-release to 1.9\n" );
	MySql & connection = synchronise_.connection();
	const EntityDefs & entityDefs = synchronise_.entityDefs();

	MySqlTransaction transaction( connection );

	// Get table meta data from entity definition.
	PropertyMappingsPerType propertyMappingsPerType;
	propertyMappingsPerType.init( entityDefs );

	SimpleTableCollector entityTableCollector;
	for ( EntityTypeID e = 0; e < entityDefs.getNumEntityTypes(); ++e )
	{
		if (!entityDefs.isValidEntityType( e ))
		{
			continue;
		}

		const PropertyMappings& properties = propertyMappingsPerType[ e ];
		const EntityDescription& entDes = entityDefs.getEntityDescription( e );

		EntityMapping entityMapping( entDes, properties );

		// Collect tables for this entity type.
		entityMapping.visitSubTablesRecursively( entityTableCollector );
	}
	const SimpleTableCollector::NewTableDataMap& entityTables =
			entityTableCollector.getTables();

	// Get list of entity tables from database
	StrSet tableNames;
	TableMetaData::getEntityTables( tableNames, connection );

	// Add "NOT NULL" specification to all entity data columns.
	for (StrSet::const_iterator pTblName = tableNames.begin();
			pTblName != tableNames.end(); ++pTblName )
	{
		INFO_MSG( "Adding \"NOT NULL\" specification to columns in "
				"table %s\n", pTblName->c_str() );

		// Get column meta data from database.
		TableMetaData::NameToColInfoMap columns;
		getTableColumns( columns, connection, *pTblName );

		// Get column meta data from entity definition
		SimpleTableCollector::NewTableDataMap::const_iterator itColumnsDef =
				entityTables.find( *pTblName );
		const TableMetaData::NameToColInfoMap* pColumnsDef =
				(itColumnsDef != entityTables.end()) ?
						&(itColumnsDef->second) : NULL;
		if (!pColumnsDef)
		{
			WARNING_MSG( "upgradeDatabase1_9Snapshot: Cannot find matching "
					"entity definition for table %s. Default values for "
					"columns won't be set correctly\n", pTblName->c_str() );
		}

		BW::stringstream ss;
		ss << "ALTER TABLE " << *pTblName;
		for ( TableMetaData::NameToColInfoMap::const_iterator pColInfo =
				columns.begin(); pColInfo != columns.end(); ++pColInfo )
		{
			// Modifying id column will fail, so skip it.
			if (pColInfo->first == ID_COLUMN_NAME_STR)
			{
				continue;
			}

			// Firstly, set all existing NULL values to the default value.
			// If we don't do this, they will default to 0 or empty string.
			// But can only do this if we have default value from
			// entity definition.
			const TableMetaData::ColumnInfo* pDefColInfo = NULL;
			if (pColumnsDef)
			{
				TableMetaData::NameToColInfoMap::const_iterator itDefColInfo = 
					pColumnsDef->find( pColInfo->first );

				if (itDefColInfo != pColumnsDef->end())
				{
					pDefColInfo = &itDefColInfo->second;
				}
			}

			if (pDefColInfo)
			{
				BW::string	defaultValue = pDefColInfo->columnType.
						getDefaultValueAsString( connection );

				if (!defaultValue.empty())
				{
					BW::stringstream	setNull;
					setNull << "UPDATE " << *pTblName << " SET "
						<< pColInfo->first << '=' << defaultValue
						<< " WHERE " << pColInfo->first << " IS NULL";
					// Set NULL values to default value
					// DEBUG_MSG( "%s\n", setNull.str().c_str() );
					connection.execute( setNull.str() );
				}
			}
			// Don't issue warning if the entire table definition is not
			// found.
			else if (pColumnsDef)
			{
				WARNING_MSG( "upgradeDatabase1_9Snapshot: Cannot find "
						"default value for column %s.%s. Existing NULL "
						"values will be set to default value of MySQL "
						"type (not BigWorld type)\n",
						pTblName->c_str(), pColInfo->first.c_str() );
			}

			if (pColInfo != columns.begin())
			{
				ss << ',';
			}
			ss << " MODIFY COLUMN " << pColInfo->first << ' ';

			// Use entity definition if possible.
			if (pDefColInfo)
			{
				ss << pDefColInfo->columnType.getAsString( connection,
						pDefColInfo->indexType );
			}
			else
			{
				ss << pColInfo->second.columnType.getAsString( connection,
						pColInfo->second.indexType );
			}
		}

		// Finally, update table definition with "NON-NULL" and
		// default value.
		// DEBUG_MSG( "%s\n", ss.str().c_str() );
		connection.execute( ss.str() );
	}

	// Remove bigworldTableMetadata table.
	INFO_MSG( "\tRemoving bigworldTableMetadata table\n" );
	connection.execute( "DROP TABLE bigworldTableMetadata" );

	// Now done in upgradeDatabase1_9NonNull()
	//this->upgradeVersionNumber( DBAPP_VERSION_1_9_SNAPSHOT );

	transaction.commit();
}


/**
 *	Upgrades the database from 1.8 to 1.9 pre-release.
 */
void MySqlUpgradeDatabase::upgradeDatabase1_8()
{
	MySql & connection = synchronise_.connection();
	MySqlTransaction transaction( connection );

	INFO_MSG( "Upgrading database tables from 1.8 to 1.9\n" );

	// Update version number stored in database.
	// Now done in upgradeDatabase1_9Snapshot()
//		INFO_MSG( "\tUpdating version number\n" );
//		BW::stringstream	ss;
//		ss << "UPDATE bigworldInfo SET version=" << DBAPP_CURRENT_VERSION;
//		transaction.execute( ss.str() );

	// We've added a column to bigworldInfo
	INFO_MSG( "\tAdding snapshotTime column to bigworldInfo\n" );
	connection.execute( "ALTER TABLE bigworldInfo ADD COLUMN "
			"(snapshotTime TIMESTAMP NULL)" );

	transaction.commit();
}


/**
 *	Generic upgrade version number.
 */
void MySqlUpgradeDatabase::upgradeVersionNumber( const uint32 newVersion )
{
	// Update version number
	INFO_MSG( "\tUpdating version number\n" );

	BW::stringstream ss;
	ss << "UPDATE bigworldInfo SET version=" << newVersion;
	synchronise_.connection().execute( ss.str() );
}


/**
 *	Convert a row in the bigworldLogOnMappings table from using the identifier
 *	string for an entity type to using an entity ID.
 */
bool MySqlUpgradeDatabase::convertRecordNameToDBID( int bwEntityTypeID, 
		int dbEntityTypeID )
{
	MySql & connection = synchronise_.connection();
	const EntityDefs & entityDefs = synchronise_.entityDefs();

	const EntityDescription & entityDescription =
		entityDefs.getEntityDescription( bwEntityTypeID );

	Namer namer( entityDescription.name() );

	const DataDescription * pDataDesc = entityDescription.pIdentifier();

	if (!pDataDesc)
	{
		ERROR_MSG( "convertRecordNameToDBID: No identifier for %s\n",
				entityDescription.name().c_str() );

		return false;
	}

	BW::string identifier = pDataDesc->name();

	BW::string tableName( TABLE_NAME_PREFIX );
	tableName += '_' + entityDescription.name();

	BW::string propName = namer.buildColumnName( "sm", identifier );

	INFO_MSG( "Updating bigworldLogOnMapping for %s\n",
			entityDescription.name().c_str() );

	char buffer[ 512 ];

	bw_snprintf( buffer, sizeof( buffer ),
		"UPDATE bigworldLogOnMapping "
			"LEFT JOIN %s "
				"ON bigworldLogOnMapping.recordName = %s.%s "
			"SET bigworldLogOnMapping.entityID = %s.id "
			"WHERE bigworldLogOnMapping.typeID = %d",
		tableName.c_str(),
		tableName.c_str(),
		propName.c_str(),
		tableName.c_str(),
		dbEntityTypeID );

	connection.execute( buffer );

	return true;
}

BW_END_NAMESPACE

// mysql_upgrade_database.cpp
