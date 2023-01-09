#ifndef MYSQL_ENTITY_TYPE_MAPPINGS_HPP
#define MYSQL_ENTITY_TYPE_MAPPINGS_HPP

#include "cstdmf/bw_namespace.hpp"

#include <cstddef>
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class EntityDefs;
class EntityTypeMapping;
class MySql;


/**
 *	This class is a collection of EntityTypeMapping instances for each entity
 *	type.
 */
class EntityTypeMappings
{
public:
	EntityTypeMappings();
	EntityTypeMappings( const EntityDefs & entityDefs,
			MySql & connection );
	~EntityTypeMappings();

	bool init( const EntityDefs & entityDefs,
			MySql & connection );

	const EntityTypeMapping * operator[]( size_t typeID ) const
	{
		return container_[ typeID ];
	}

private:
	typedef BW::vector< EntityTypeMapping * > Container;
	Container container_;

	void clearContainer();
};

BW_END_NAMESPACE

#endif // MYSQL_ENTITY_TYPE_MAPPINGS_HPP
