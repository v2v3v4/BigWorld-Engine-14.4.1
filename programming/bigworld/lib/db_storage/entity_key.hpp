#ifndef ENTITY_KEY_HPP
#define ENTITY_KEY_HPP

#include "network/basictypes.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class represents a key to an entity record in the database.
 */
class EntityKey
{
public:
	EntityKey( EntityTypeID type, DatabaseID id ) :
		typeID( type ),
		dbID( id )
	{
	}

	bool operator<( const EntityKey & other ) const
	{
		return (typeID < other.typeID) ||
				((typeID == other.typeID) && (dbID < other.dbID));
	}

	EntityTypeID	typeID;
	DatabaseID 		dbID;
};


/**
 *	This class represents a key to an entity record in the database that can
 *	use either a DatabaseID or the entity's identifier string.
 */
class EntityDBKey : public EntityKey
{
public:
	EntityDBKey( EntityTypeID typeID, DatabaseID dbID,
			const BW::string & s = BW::string() ) :
		EntityKey( typeID, dbID ),
		name( s )
	{
	}

	explicit EntityDBKey( const EntityKey & key ) :
		EntityKey( key ),
		name()
	{
	}

	BW::string		name;	///< used if dbID is zero
};

BW_END_NAMESPACE

#endif // ENTITY_KEY_HPP

