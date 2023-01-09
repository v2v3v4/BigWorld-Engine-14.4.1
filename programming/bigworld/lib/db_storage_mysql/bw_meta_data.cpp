#include "bw_meta_data.hpp"

#include "constants.hpp"
#include "query.hpp"
#include "result_set.hpp"


BW_BEGIN_NAMESPACE

namespace
{
const Query getEntityTypeIDQuery(
			"SELECT bigworldID FROM bigworldEntityTypes WHERE name=?" );

const Query setEntityTypeIDQuery(
			"UPDATE bigworldEntityTypes SET bigworldID=? WHERE name=?" );

const Query addEntityTypeQuery(
			"INSERT INTO bigworldEntityTypes (typeID, bigworldID, name) "
			"VALUES (NULL, ?, ?)" );

const Query removeEntityTypeQuery(
			"DELETE FROM bigworldEntityTypes WHERE name=?" );

} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: BigWorld meta data
// -----------------------------------------------------------------------------

/**
 *	Constructor. Can only be called after initSpecialBigWorldTables().
 *
 * 	@param	connection	The connection to MySQL server.
 */
BigWorldMetaData::BigWorldMetaData( MySql & connection ) :
	connection_( connection )
{
}


/**
 * 	This method retrieves the EntityTypeID associated with the entity name
 * 	from our meta information.
 *
 *  @param	entityName	The entity type name.
 */
EntityTypeID BigWorldMetaData::getEntityTypeID( const BW::string & entityName )
{
	ResultSet resultSet;
	getEntityTypeIDQuery.execute( connection_, entityName, &resultSet );

	EntityTypeID typeID = INVALID_ENTITY_TYPE_ID;

	resultSet.getResult( typeID );

	return typeID;
}


/**
 * 	This method sets the EntityTypeID associated with the entity name
 * 	into our meta information.
 *
 *  @param	entityName	The entity type name.
 * 	@param	typeID		The entity type ID.
 */
void BigWorldMetaData::setEntityTypeID( const BW::string & entityName,
		EntityTypeID typeID	)
{
	setEntityTypeIDQuery.execute( connection_, typeID, entityName, NULL );
}


/**
 * 	This method adds an EntityTypeID-entity name mapping into
 * 	our meta information.
 *
 *  @param	entityName	The entity type name.
 * 	@param	typeID		The entity type ID.
 */
void BigWorldMetaData::addEntityType( const BW::string & entityName,
		EntityTypeID typeID )
{
	addEntityTypeQuery.execute( connection_, typeID, entityName, NULL );
}


/**
 * 	This method removes an EntityTypeID-entity name mapping from
 * 	our meta information.
 *
 *  @param	entityName	The entity type name.
 */
void BigWorldMetaData::removeEntityType( const BW::string & entityName )
{
	removeEntityTypeQuery.execute( connection_, entityName, NULL );
}

BW_END_NAMESPACE

// mysql_bw_meta_data.cpp
