#include "script/first_include.hpp"

#include "entity_type_mapping.hpp"

#include "blob_mapping.hpp"
#include "string_like_mapping.hpp"
#include "stream_to_query_helper.hpp"
#include "result_to_stream_helper.hpp"
#include "string_mapping.hpp"

#include "../comma_sep_column_names_builder.hpp"
#include "../query.hpp"
#include "../result_set.hpp"
#include "../utils.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/debug.hpp"

#include "network/basictypes.hpp"

#include "entitydef/entity_description.hpp"

#include <arpa/inet.h>

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

namespace
{
const Query getMappedTypeIDQuery(
	"SELECT typeID FROM bigworldEntityTypes WHERE bigworldID= ?" );
}


// -----------------------------------------------------------------------------
// Section: class EntityTypeMappingVisitor
// -----------------------------------------------------------------------------

/**
 *
 */
class EntityTypeMappingVisitor : public IDataDescriptionVisitor
{
public:
	EntityTypeMappingVisitor( const EntityTypeMapping & entityTypeMapping );

	virtual bool visitPropertyMapping(
			const PropertyMapping & propertyMapping ) = 0;

	virtual bool visit( const DataDescription & propDesc );

private:
	const EntityTypeMapping & entityTypeMapping_;
};


/**
 *	Constructor.
 */
EntityTypeMappingVisitor::EntityTypeMappingVisitor(
		const EntityTypeMapping & entityTypeMapping ) :
	entityTypeMapping_( entityTypeMapping )
{
}


/**
 *
 */
bool EntityTypeMappingVisitor::visit( const DataDescription & propDesc )
{
	const PropertyMapping * pPropMapping =
		entityTypeMapping_.getPropMapByName( propDesc.fullName() );

	MF_ASSERT( pPropMapping );

	return this->visitPropertyMapping( *pPropMapping );
}


// -----------------------------------------------------------------------------
// Section: GetFromDBVisitor
// -----------------------------------------------------------------------------

/**
 *
 */
class GetFromDBVisitor : public EntityTypeMappingVisitor
{
public:
	GetFromDBVisitor( MySql & connection,
			const EntityTypeMapping & entityTypeMapping,
			ResultStream & resultStream,
			BinaryOStream & stream ):
		EntityTypeMappingVisitor( entityTypeMapping ),
		helper_( connection ),
		resultStream_( resultStream ),
		stream_( stream )
	{
		resultStream_ >> dbID_;
		helper_.parentID( dbID_ );
	}

	virtual bool visitPropertyMapping( const PropertyMapping & propertyMapping )
	{
		propertyMapping.fromDatabaseToStream( helper_, resultStream_, stream_ );

		return !resultStream_.error();
	}

	DatabaseID databaseID() const	{ return dbID_; }

private:
	ResultToStreamHelper helper_;
	ResultStream resultStream_;
	BinaryOStream & stream_;

	DatabaseID dbID_;
};


// -----------------------------------------------------------------------------
// Section: class EntityTypeMapping
// -----------------------------------------------------------------------------

EntityTypeMapping::EntityTypeMapping( MySql & con,
		const EntityDescription & desc,
		PropertyMappings & properties,
		const BW::string & identifierProperty ) :
	EntityMapping( desc, properties, TABLE_NAME_PREFIX ),

	getByNameQuery_(),
	getByIDQuery_(),

	getNameFromIDQuery_(
		"SELECT sm_" + identifierProperty +
			" FROM " + this->getTableName() + " WHERE id=?" ),
	getIDFromNameQuery_(
		"SELECT id FROM " + this->getTableName() +
			" WHERE sm_" + identifierProperty + "=?" ),

	idExistsQuery_( "SELECT id FROM " +
						this->getTableName() + " WHERE id=?" ),
	hasNewerQuery_( "SELECT gameTime FROM " +
			this->getTableName() + " WHERE id=? AND gameTime >= ?" ),
	insertNewQuery_(),
	insertExplicitQuery_(),
	deleteIDQuery_( "DELETE FROM " + this->getTableName() + " WHERE id=?" ),
	lookUpEntitiesQueryStart_( 
		"SELECT e.id, lo.objectID, lo.ip, lo.port, lo.salt "
		"FROM " + this->getTableName() + " AS e "
			"LEFT JOIN bigworldLogOns AS lo ON e.id = lo.databaseID AND "
				"lo.typeID = ? "
		"WHERE 1 = 1 " ), // 1=1 : support appending AND conditions
	propsNameMap_(),
	identifier_( identifierProperty )
{
	const BW::string &	tableName = this->getTableName();

	if (properties.size() > 0)
	{
		for (PropertyMappings::iterator prop = properties.begin();
			  prop != properties.end(); ++prop)
		{
			(*prop)->prepareSQL();
		}

		// Create prop name to PropertyMapping map
		for (PropertyMappings::const_iterator iter = properties.begin();
			iter != properties.end(); ++iter)
		{
			PropertyMappingPtr pMapping = *iter;
			propsNameMap_[ pMapping->propName() ] = pMapping.getObject();
		}

		// Cache fixed properties so we don't have to always go look for them.
		fixedCellProps_[ CELL_POSITION_INDEX ] =
										this->getPropMapByName( "position" );
		fixedCellProps_[ CELL_DIRECTION_INDEX ] =
										this->getPropMapByName( "direction" );
		fixedCellProps_[ CELL_SPACE_ID_INDEX ] =
										this->getPropMapByName( "spaceID" );

		fixedMetaProps_[ GAME_TIME_INDEX ] =
			this->getPropMapByName( GAME_TIME_COLUMN_NAME );
		fixedMetaProps_[ TIMESTAMP_INDEX ] =
			this->getPropMapByName( TIMESTAMP_COLUMN_NAME );

		insertNewQuery_.init( createInsertStatement( tableName, properties ) );
		insertExplicitQuery_.init( createExplicitInsertStatement( tableName, properties ) );
		updateQuery_.init( createUpdateStatement( tableName, properties ) );

		getByIDQuery_.init(
				createSelectStatement( tableName, properties, "id=?" ) );

		if (!identifier_.empty())
		{
			getByNameQuery_.init( createSelectStatement( tableName, properties,
				"sm_" + identifier_ + "=?" ) );
		}
	}
	else
	{
		for (int i = 0; i < NUM_FIXED_CELL_PROPS; ++i)
		{
			fixedCellProps_[i] = NULL;
		}

		for (int i = 0; i < NUM_FIXED_META_PROPS; ++i)
		{
			fixedMetaProps_[i] = NULL;
		}
	}

	ResultSet resultSet;
	getMappedTypeIDQuery.execute( con, this->getTypeID(), &resultSet );

	resultSet.getResult( mappedType_);
}


/**
 *	This method checks that the entity with the given DBID exists in the
 *	database.
 *
 *	@return	True if it exists.
 */
bool EntityTypeMapping::checkExists( MySql & connection, DatabaseID dbID ) const
{
	ResultSet resultSet;
	idExistsQuery_.execute( connection, dbID, &resultSet );

	return resultSet.numRows() > 0;
}


/**
 *	This method checks whether the entity with the given id has been saved after
 *	the given time.
 */
bool EntityTypeMapping::hasNewerRecord( MySql & connection,
		DatabaseID dbID, GameTime time ) const
{
	ResultSet resultSet;
	hasNewerQuery_.execute( connection, dbID, time, &resultSet );

	return resultSet.numRows() > 0;
}


/**
 *	This method returns the database ID of the entity given its name.
 *
 *	@param	connection	Connection to use when querying the database.
 *	@param	name		The name of the entity.
 *	@return	The database ID of the entity. Returns 0 if the entity
 *		doesn't exists or if the entity doesn't have a name index.
 */
DatabaseID EntityTypeMapping::getDbID( MySql & connection,
	const BW::string & name ) const
{
	DatabaseID dbID = 0;

	if (this->hasIdentifier())
	{
		ResultSet resultSet;
		getIDFromNameQuery_.execute( connection, name, &resultSet );

		if (!resultSet.getResult( dbID ))
		{
			dbID = 0;
		}
	}

	return dbID;
}


/**
 *	This method returns the name of the entity given its database Id.
 *
 *	@param	connection	Connection to use when querying the database.
 *	@param	dbID		The database ID of the entity.
 *	@param	name		Returns the name of the entity here.
 *
 *	@return	True if the entity exists and have a name.
 */
bool EntityTypeMapping::getName( MySql & connection,
									DatabaseID dbID, BW::string & name ) const
{
	if (!this->hasIdentifier())
	{
		return false;
	}

	ResultSet resultSet;
	getNameFromIDQuery_.execute( connection, dbID, &resultSet );

	return resultSet.getResult( name );
}




// Retrieving an entity
bool EntityTypeMapping::getStreamByID( MySql & connection,
	DatabaseID dbID, BinaryOStream & outStream ) const
{
	ResultSet resultSet;
	getByIDQuery_.execute( connection, dbID, &resultSet );

	return this->getStreamFromDBRow( connection, resultSet, outStream ) != 0;
}


/**
 *	This method constructs a suitable look up SQL query for looking up entities
 *	by properties.
 *
 *	@param sortProperty 	The property to sort by.
 *	@param isAscending		Whether to sort in ascending or descending order.
 *	@param limit			The limit to the number of results returned.
 *	@param offset			The offset of the results returned.
 *	@param queries			The property queries to match.
 */
const BW::string EntityTypeMapping::buildLookUpEntitiesQuery( 
	const LookUpEntitiesCriteria & criteria ) const
{
	BW::ostringstream queryBuilder;
	queryBuilder << lookUpEntitiesQueryStart_;

	PropertyQueries::const_iterator iQuery = criteria.propertyQueries().begin();

	while (iQuery != criteria.propertyQueries().end())
	{
		queryBuilder << "AND sm_" << iQuery->first << " = ? ";
		++iQuery;
	}

	if (criteria.filter() == LOOK_UP_ENTITIES_FILTER_ONLINE)
	{
		queryBuilder << "AND NOT lo.objectID IS NULL ";
	}
	else if (criteria.filter() == LOOK_UP_ENTITIES_FILTER_OFFLINE)
	{
		queryBuilder << "AND lo.objectID IS NULL ";
	}

	if (criteria.hasSortProperty())
	{
		queryBuilder << "ORDER BY sm_" << criteria.sortProperty() << 
			(criteria.isAscending() ? " ASC " : " DESC " );
	}

	if (criteria.limit() != -1)
	{
		queryBuilder << "LIMIT " << criteria.limit();
		if (criteria.offset() != -1)
		{
			 queryBuilder << " OFFSET " << criteria.offset();
		}
	}

	return queryBuilder.str();
}


/**
 *	This method implements the search for entities based on the value of a
 *	string property defined on that entity type.
 *
 *	@param connection	A connection to the database.
 *	@param criteria		The set of property queries to match.
 *	@param handler		The handler. Note that this method does not call
 *						onLookUpEntitiesFinish().
 */
bool EntityTypeMapping::lookUpEntitiesByProperties( MySql & connection,
		const LookUpEntitiesCriteria & criteria,
		IDatabase::ILookUpEntitiesHandler & handler ) const
{

	// Check that the properties are valid for doing a look-up.
	PropertyQueries::const_iterator iQuery = criteria.propertyQueries().begin();

	while (iQuery != criteria.propertyQueries().end())
	{
		const BW::string & propertyName = iQuery->first;
		const PropertyMapping * pPropertyMapping = 
			this->getPropMapByName( propertyName );

		if (pPropertyMapping->hasTable())
		{
			ERROR_MSG( "EntityTypeMapping::lookUpEntitiesByProperties: "
					"property %s.%s is not of appropriate type "
					"for string match\n",
				this->typeName().c_str(), propertyName.c_str() );

			return false;
		}

		++iQuery;
	}

	// Check that the sort property is valid.
	const PropertyMapping * sortPropertyMapping = criteria.hasSortProperty() ?
		this->getPropMapByName( criteria.sortProperty() ) :
		NULL;

	if (criteria.hasSortProperty() && 
			(sortPropertyMapping == NULL || sortPropertyMapping->hasTable()))
	{
		ERROR_MSG( "EntityTypeMapping::lookUpEntitiesByProperties: "
				"property %s.%s is not of appropriate type for sorting\n",
			this->typeName().c_str(), criteria.sortProperty().c_str() );

		return false;
	}


	// Construct and run the query.
	Query query( this->buildLookUpEntitiesQuery( criteria ) );

	QueryRunner queryRunner( connection, query );

	queryRunner.pushArg( mappedType_ );

	iQuery = criteria.propertyQueries().begin();
	while (iQuery != criteria.propertyQueries().end())
	{
		queryRunner.pushArg( iQuery->second );
		++iQuery;
	}

	ResultSet resultSet;
	queryRunner.execute( &resultSet );

	ResultRow row;


	// Retrieve the results and call back on the handler with the results.
	handler.onLookUpEntitiesStart( resultSet.numRows() );

	while (row.fetchNextFrom( resultSet ))
	{
		DatabaseID dbID = 0;
		EntityMailBoxRef location;

		row.getField( 0, dbID );

		if (row.getField( 1, location.id ))
		{
			row.getField( 2, location.addr.ip );
			row.getField( 3, location.addr.port );
			row.getField( 4, location.addr.salt );

			location.addr.ip = ntohl( location.addr.ip );
			location.addr.port = ntohs( location.addr.port );
		}

		handler.onLookedUpEntity( dbID, location );
	}

	// onLookUpEntitiesFinish() is called from the calling code.

	return true;
}


/**
 *	This method converts a result row from the database to a BinaryOStream.
 *	This call may trigger sub-queries to other tables.
 */
DatabaseID EntityTypeMapping::getStreamFromDBRow( MySql & connection,
	ResultSet & resultSet, BinaryOStream & strm ) const
{
	ResultRow resultRow;

	if (!resultRow.fetchNextFrom( resultSet ))
	{
		// TODO: Raise exception?
		return 0;
	}

	ResultStream resultStream( resultRow );

	GetFromDBVisitor visitor( connection, *this, resultStream, strm );

	if (!this->visit( visitor, /*shouldVisitMetaProps:*/false ))
	{
		return 0;
	}

	return visitor.databaseID();
}


/**
 * 	Deletes an entity by DBID.
 */
bool EntityTypeMapping::deleteWithID( MySql & connection, DatabaseID id ) const
{
	deleteIDQuery_.execute( connection, id, NULL );

	if (connection.affectedRows() == 0)
	{
		return false;
	}

	MF_ASSERT( connection.affectedRows() == 1 );

	// Delete any child table entries
	const PropertyMappings & properties = this->getPropertyMappings();

	PropertyMappings::const_iterator iter = properties.begin();

	while (iter != properties.end())
	{
		(*iter)->deleteChildren( connection, id );

		++iter;
	}

	return true;
}

namespace
{
const Query removeLogOnQuery(
			"DELETE FROM bigworldLogOns WHERE databaseID = ? AND typeID = ?" );
}

void EntityTypeMapping::removeLogOnRecord( MySql & conn, DatabaseID id ) const
{
	removeLogOnQuery.execute( conn, id, this->getDatabaseTypeID(), NULL );
}


// -----------------------------------------------------------------------------
// Section: class UpdateFromStreamVisitor
// -----------------------------------------------------------------------------

/**
 *
 */
class UpdateFromStreamVisitor : public EntityTypeMappingVisitor
{
public:
	UpdateFromStreamVisitor( MySql & connection,
			const EntityTypeMapping & entityTypeMapping,
			DatabaseID dbID, const Query & query, BinaryIStream & stream );

	bool execute();

	bool visitPropertyMapping( const PropertyMapping & propertyMapping );

	DatabaseID databaseID() const		{ return helper_.parentID(); }

	void pushGameTime( GameTime gameTime )
	{
		queryRunner_.pushArg( gameTime );
	}

private:
	StreamToQueryHelper helper_;
	QueryRunner queryRunner_;
	BinaryIStream & stream_;
	DatabaseID databaseID_;
};


/**
 *	Constructor
 */
UpdateFromStreamVisitor::UpdateFromStreamVisitor( MySql & connection,
			const EntityTypeMapping & entityTypeMapping,
			DatabaseID dbID, const Query & query, BinaryIStream & stream ) :
	EntityTypeMappingVisitor( entityTypeMapping ),
	helper_( connection, dbID ),
	queryRunner_( connection, query ),
	stream_( stream ),
	databaseID_( dbID )
{
}


bool UpdateFromStreamVisitor::execute()
{
	if (databaseID_)
	{
		queryRunner_.pushArg( databaseID_ );
	}

	if (stream_.error())
	{
		// TODO: Should this be an exception?
		return false;
	}

	// Note: Can raise exception
	queryRunner_.execute( NULL );

	if (helper_.parentID() == 0)
	{
		helper_.parentID( helper_.connection().insertID() );
	}
	else
	{
		MF_ASSERT( !helper_.hasBufferedQueries() );
	}

	helper_.executeBufferedQueries( helper_.parentID() );

	return true;
}


/**
 *
 */
bool UpdateFromStreamVisitor::visitPropertyMapping(
		const PropertyMapping & propertyMapping )
{
	propertyMapping.fromStreamToDatabase( helper_, stream_, queryRunner_ );

	bool isOkay = !stream_.error();
	if (!isOkay)
	{
		ERROR_MSG( "UpdateFromStreamVisitor::visitPropertyMapping: "
				"Error encountered while de-streaming property '%s'",
					propertyMapping.propName().c_str() );
	}

	return isOkay;
}


/**
 *
 */
bool EntityTypeMapping::visit( EntityTypeMappingVisitor & visitor,
	   bool shouldVisitMetaProps ) const
{
	const EntityDescription & entityDescription = this->getEntityDescription();

	bool isOkay = entityDescription.visit( 
		EntityDescription::BASE_DATA |
				EntityDescription::CELL_DATA |
				EntityDescription::ONLY_PERSISTENT_DATA,
			visitor );

	if (entityDescription.canBeOnCell())
	{
		for (int i = 0; (i < NUM_FIXED_CELL_PROPS) && isOkay; ++i)
		{
			isOkay &= visitor.visitPropertyMapping( *fixedCellProps_[i] );
		}
	}

	if (shouldVisitMetaProps)
	{
		for (int i = 0; i < NUM_FIXED_META_PROPS && isOkay; ++i )
		{
			isOkay &= visitor.visitPropertyMapping( *fixedMetaProps_[i] );
		}
	}

	return isOkay;
}


/**
 *	This method updates an existing entity's properties in the database.
 *
 *	@param	connection	Connection to use when updating the database.
 *	@param	dbID		The databaseID of the entity to update.
 *	@param	strm		The data to use when updating the entity.
 *	@param	pGameTime	The (optional) game time to use during the update.
 *
 *	@return	Returns true if the entity was updated. False if the entity
 *		doesn't exist.
 */
bool EntityTypeMapping::update( MySql & connection, DatabaseID dbID,
	   BinaryIStream & strm, GameTime * pGameTime ) const
{
	UpdateFromStreamVisitor visitor( connection,
			*this, dbID, updateQuery_, strm );

	const bool isGameTimeOnStream = (pGameTime == NULL);

	this->visit( visitor, /*shouldVisitMetaProps:*/isGameTimeOnStream );

	if (!isGameTimeOnStream)
	{
		visitor.pushGameTime( *pGameTime );
	}

	return visitor.execute();
}


/**
 *	This method inserts a new entity into the database.
 *
 *	@param	connection	Connection to use when updating the database.
 *	@param	strm		The input stream to read entity data from.
 *
 *	@return	The database ID of the newly inserted entity.
 */
DatabaseID EntityTypeMapping::insertNew( MySql & connection,
		BinaryIStream & strm ) const
{

	UpdateFromStreamVisitor visitor( connection,
			*this, 0, insertNewQuery_, strm );

	this->visit( visitor, /*shouldVisitMetaProps:*/true );

	DatabaseID ret = visitor.execute() ? visitor.databaseID() : 0;
	return ret;
}

/**
 *	This method inserts a new entity into the database with the
 *	given database ID.
 *
 *	@param	connection	Connection to use when updating the database.
 *	@param	strm		The input stream to read entity data from.
 *	@param	dbID		The database ID for the record
 *
 *	@return	The database ID of the newly inserted entity.
 */
DatabaseID EntityTypeMapping::insertExplicit( MySql & connection,
		DatabaseID dbID,
		BinaryIStream & strm ) const
{
	UpdateFromStreamVisitor visitor( connection,
			*this, dbID, insertExplicitQuery_, strm );

	this->visit( visitor, /*shouldVisitMetaProps:*/true );

	DatabaseID ret = visitor.execute() ? visitor.databaseID() : 0;
	return ret;
}


namespace
{
const Query addLogOnQuery(
	"INSERT INTO bigworldLogOns "
			"(databaseID, typeID, objectID, ip, port, salt) "
		"VALUES (?,?,?,?,?,?) "
		"ON DUPLICATE KEY "
		"UPDATE "
			"objectID = VALUES(objectID), "
			"ip = VALUES(ip), "
			"port = VALUES(port), "
			"salt = VALUES(salt)" );

const Query updateLogOnAutoLoadQuery(
	"UPDATE bigworldLogOns SET shouldAutoLoad = ? "
		"WHERE databaseID = ? AND typeID = ?" );

const Query getLogOnQuery(
	"SELECT objectID, ip, port, salt FROM bigworldLogOns "
				"WHERE databaseID = ? and typeID = ?" );
}


/**
 *
 */
void EntityTypeMapping::addLogOnRecord( MySql & connection,
			DatabaseID dbID, const EntityMailBoxRef & mailbox ) const
{
	addLogOnQuery.execute( connection,
			dbID, this->getDatabaseTypeID(),
			mailbox.id,
			htonl( mailbox.addr.ip ),
			htons( mailbox.addr.port ),
			mailbox.addr.salt,
			NULL );
}


/**
 *	Update the auto-load status of an entity's log on mapping.
 *
 *	@param connection		The database connection.
 *	@param dbID				The entity's database ID.
 *	@param shouldAutoLoad	The value of the auto-load status.
 */
void EntityTypeMapping::updateAutoLoad( MySql & connection,
			DatabaseID dbID, bool shouldAutoLoad ) const
{
	updateLogOnAutoLoadQuery.execute( connection, 
		shouldAutoLoad, dbID, this->getDatabaseTypeID(), NULL );
}


/**
 *
 */
bool EntityTypeMapping::getLogOnRecord( MySql & connection,
		DatabaseID dbID, EntityMailBoxRef & ref ) const
{
	ResultSet resultSet;
	getLogOnQuery.execute( connection,
			dbID, this->getDatabaseTypeID(), &resultSet );

	bool isOkay = resultSet.getResult( ref.id,
				ref.addr.ip, ref.addr.port, ref.addr.salt );

	ref.addr.ip = ntohl( ref.addr.ip );
	ref.addr.port = ntohs( ref.addr.port );

	return isOkay;
}


/**
 *	This method attempts to return a cached DatabaseID that is associated with
 *	the Identifier in the EntityDBKey.
 *
 *	@param entityKey This structure contains the entity type and Identifier to
 *						use when performing the cache lookup.
 *
 *	@returns The DatabaseID on success, 0 on no cached Identifier mapping.
 */
DatabaseID EntityTypeMapping::findCachedDatabaseID(
	const EntityDBKey & entityKey ) const
{
	MF_ASSERT( entityKey.dbID == 0 );

	IdentifierToDatabaseIDMap::const_iterator keyMapIter =
		identifierToDatabaseID_.find( entityKey.name );

	if (keyMapIter == identifierToDatabaseID_.end())
	{
		return 0;
	}

	return keyMapIter->second;
}


/**
 *	This method adds the Entity Key mapping from Identifier to DatabaseID.
 *
 *	@param entityKey The EntityDBKey containing the Identifier and DBID to add.
 */
void EntityTypeMapping::cacheEntityKey( const EntityDBKey & entityKey )
{
	MF_ASSERT( this->hasIdentifier() );
	MF_ASSERT( entityKey.dbID != 0 );
	MF_ASSERT( entityKey.dbID != PENDING_DATABASE_ID );

	// If two first lookups occur at the same time, this may overwrite an
	// existing entry.
	identifierToDatabaseID_[ entityKey.name ] = entityKey.dbID;
}


/**
 *	This method removes a cached Identifier to DatabaseID map.
 *
 * 	@param entityKey The EntityDBKey containing the Identifier that
 * 		should be removed.
 */
void EntityTypeMapping::removeCachedEntityKey( const EntityDBKey & entityKey )
{
	identifierToDatabaseID_.erase( entityKey.name );
}

BW_END_NAMESPACE

// entity_type_mapping.cpp
