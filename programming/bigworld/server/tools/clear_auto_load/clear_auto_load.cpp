#include "clear_auto_load.hpp"

#include "db_storage_mysql/result_set.hpp"
#include "db_storage_mysql/transaction.hpp"
#include "db_storage_mysql/wrapper.hpp"

#include "db_storage_mysql/mappings/entity_type_mapping.hpp"

#include "server/bwconfig.hpp"


BW_BEGIN_NAMESPACE

extern int force_link_UDO_REF;
extern int ResMgr_token;
extern int PyScript_token;

namespace // anonymous
{

int s_moduleTokens = ResMgr_token | force_link_UDO_REF | PyScript_token;

} // end namespace (anonymous)


/**
 *	Constructor.
 */
ClearAutoLoad::ClearAutoLoad():
	DatabaseToolApp()
{
}


/**
 *	Destructor.
 */
ClearAutoLoad::~ClearAutoLoad()
{
}


/**
 *	Initialise the ClearAutoLoad instance.
 */
bool ClearAutoLoad::init( bool isVerbose )
{
	if (!this->DatabaseToolApp::init( "ClearAutoLoad", "clear_auto_load",
				isVerbose, /*shouldLock=*/ true ))
	{
		return false;
	}

	if (!entityTypeMappings_.init( this->entityDefs(), this->connection() ))
	{
		ERROR_MSG( "Failed to initialise Entity Type Mappings" );
		return false;
	}

	if (!this->checkTablesExist())
	{
		return false;
	}

	return true;
}


/**
 *	This method deletes the auto-load data in the database.
 */
bool ClearAutoLoad::run()
{
	try
	{
		MySql & connection = this->connection();

		MySqlTransaction transaction( connection );

		connection.execute( "DELETE FROM bigworldSpaces" );
		connection.execute( "DELETE FROM bigworldSpaceData" );
		connection.execute( "UPDATE bigworldGameTime SET time = 0" );

		this->deleteAutoLoadEntities();

		transaction.commit();
		INFO_MSG( "Cleared auto-load data\n" );
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "Failed to clear auto-load data: %s\n", e.what() );
		return false;
	}

	return true;
}


/**
 *	Check if the requisite tables exist.
 */
bool ClearAutoLoad::checkTablesExist()
{
	static const uint NUM_REQUISITE_TABLES = 4;

	try
	{
		ResultSet resultSet;
		this->connection().execute( "SHOW TABLES", &resultSet );

		uint left = NUM_REQUISITE_TABLES;

		BW::string tableName;
		while (resultSet.getResult( tableName ))
		{
			if (tableName == "bigworldLogOns" ||
					tableName == "bigworldSpaceData" ||
					tableName == "bigworldSpaces" ||
					tableName == "bigworldGameTime")
			{
				--left;
			}
		}

		if (left > 0)
		{
			ERROR_MSG( "Missing a required table in the database, "
					"please run sync_db.\n" );
		}

		return (left == 0);
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "Could not enumerate tables in database: %s\n",
			e.what() );
	}

	return false;
}


/**
 *	For each auto-loaded entity, delete that entity from the database.
 */ 
void ClearAutoLoad::deleteAutoLoadEntities()
{
	const EntityDefs & entityDefs = this->entityDefs();
	MySql & connection = this->connection();
	try
	{
		ResultSet resultSet;
		connection.execute( "SELECT et.bigworldID, lo.databaseID "
				"FROM bigworldLogOns lo, bigworldEntityTypes et "
				"WHERE lo.typeID = et.typeID AND lo.shouldAutoLoad", 
			&resultSet );

		EntityTypeID entityTypeID;
		DatabaseID databaseID;

		while (resultSet.getResult( entityTypeID, databaseID ))
		{
			const EntityTypeMapping * pEntityTypeMapping = 
				entityTypeMappings_[ entityTypeID ];

			const EntityDescription & entityDescription =
				entityDefs.getEntityDescription( entityTypeID );

			if (!pEntityTypeMapping->deleteWithID( connection, databaseID ))
			{
				WARNING_MSG( "ClearAutoLoad::deleteAutoLoadEntities: "
						"Could not delete entity %s with DBID=%" FMT_DBID "\n",
					entityDescription.name().c_str(),
					databaseID );
			}
		}

		connection.query( "DELETE FROM bigworldLogOns" );
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "ClearAutoLoad::deleteAutoLoadEnitites: "
				"An exception occurred while removing auto-loading entities "
				"from the database: %s\n",
			e.what() );
	}
}

BW_END_NAMESPACE

// clear_auto_load.cpp

