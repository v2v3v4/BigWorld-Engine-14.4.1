#ifndef MYSQL_ENTITY_TYPE_MAPPING_HPP
#define MYSQL_ENTITY_TYPE_MAPPING_HPP

#include "entity_mapping.hpp"

#include "../query.hpp"

#include "db_storage/entity_key.hpp"
#include "db_storage/idatabase.hpp"

#include "network/basictypes.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include <memory>


BW_BEGIN_NAMESPACE

class EntityTypeMappingVisitor;
class StringLikeMapping;
class EntityMailBoxRef;


/**
 *	This class implements the typical BigWorld operations on entities - for
 * 	a single entity type.
 */
class EntityTypeMapping : public EntityMapping
{
public:
	EntityTypeMapping( MySql & connection, const EntityDescription & desc,
		PropertyMappings & properties, const BW::string & identifierProperty );

	// Basic database operations.
	DatabaseID getDbID( MySql & connection, const BW::string & name ) const;
	bool getName( MySql & connection, DatabaseID dbID, BW::string & name ) const;
	bool checkExists( MySql & connection, DatabaseID dbID ) const;
	bool hasNewerRecord( MySql & connection,
			DatabaseID id, GameTime time ) const;

	bool deleteWithID( MySql & connection, DatabaseID id ) const;

	void addLogOnRecord( MySql & connection, DatabaseID dbID,
			const EntityMailBoxRef & mailbox ) const;
	bool getLogOnRecord( MySql & connection, DatabaseID dbID,
			EntityMailBoxRef & mailbox ) const;
	void removeLogOnRecord( MySql & conn, DatabaseID id ) const;
	void updateAutoLoad( MySql & conn, DatabaseID id, 
		bool shouldAutoLoad ) const;

	// Inserting an entity
	DatabaseID insertNew( MySql & connection, BinaryIStream & strm ) const;
	DatabaseID insertExplicit( MySql & connection, DatabaseID dbID,
			BinaryIStream & strm ) const;
	bool update( MySql & connection, DatabaseID dbID,
			BinaryIStream & strm, GameTime * pGameTime ) const;

	// Retrieving an entity
	bool getStreamByID( MySql & connection,
		DatabaseID dbID, BinaryOStream & strm ) const;

	// Looking up entities
	bool lookUpEntitiesByProperties( MySql & connection,
		const LookUpEntitiesCriteria & criteria,
		IDatabase::ILookUpEntitiesHandler & handler ) const;

	// get the index of the entity type as mapped by the database
	int getDatabaseTypeID() const	{ return mappedType_;	}

	bool hasIdentifier() const		{ return !identifier_.empty(); }

	const PropertyMapping * getPropMapByName( const BW::string & name ) const
	{
		NameToPropMap::const_iterator iter = propsNameMap_.find(name);

		return (iter != propsNameMap_.end()) ? iter->second : NULL;
	}

	DatabaseID findCachedDatabaseID( const EntityDBKey & entityKey ) const;

	void cacheEntityKey( const EntityDBKey & entityKey );
	void removeCachedEntityKey( const EntityDBKey & entityKey );

private:
	bool visit( EntityTypeMappingVisitor & visitor,
			bool shouldVisitMetaProps ) const;

	DatabaseID getStreamFromDBRow( MySql & connection, ResultSet & resultSet,
		BinaryOStream & strm ) const;

	const BW::string buildLookUpEntitiesQuery( 
			const LookUpEntitiesCriteria & criteria ) const;


	Query getByNameQuery_;
	Query getByIDQuery_;

	const Query getNameFromIDQuery_;
	const Query getIDFromNameQuery_;

	const Query idExistsQuery_;
	const Query hasNewerQuery_;

	Query insertNewQuery_;
	Query insertExplicitQuery_;
	Query updateQuery_;

	const Query deleteIDQuery_;

	const BW::string lookUpEntitiesQueryStart_;

	typedef BW::map< BW::string, PropertyMapping* > NameToPropMap;
	NameToPropMap propsNameMap_;

	// Non-configurable properties.
	// Enums must be in the order that these properties are stored in the stream.
	enum FixedCellProp
	{
		CELL_POSITION_INDEX,
		CELL_DIRECTION_INDEX,
		CELL_SPACE_ID_INDEX,
		NUM_FIXED_CELL_PROPS
	};

	enum FixedMetaProp
	{
		GAME_TIME_INDEX,
		TIMESTAMP_INDEX,
		NUM_FIXED_META_PROPS
	};

	const PropertyMapping * fixedCellProps_[ NUM_FIXED_CELL_PROPS ];
	const PropertyMapping * fixedMetaProps_[ NUM_FIXED_META_PROPS ];

	// we cache what the EntityTypeID is in the database
	int mappedType_;

	BW::string identifier_;

	typedef BW::map< BW::string, DatabaseID > IdentifierToDatabaseIDMap;
	IdentifierToDatabaseIDMap identifierToDatabaseID_;
};

BW_END_NAMESPACE

#endif // MYSQL_ENTITY_TYPE_MAPPING_HPP
