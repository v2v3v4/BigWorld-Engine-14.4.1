#ifndef PROPERTY_MAPPINGS_PER_TYPE_HPP
#define PROPERTY_MAPPINGS_PER_TYPE_HPP

#include "property_mapping.hpp"


BW_BEGIN_NAMESPACE

class EntityDefs;
class EntityDescription;
class TableInspector;

/**
 *
 */
class PropertyMappingsPerType
{
public:
	PropertyMappingsPerType();
	bool init( const EntityDefs & entityDefs );

	PropertyMappings & operator[]( size_t typeID )
		{ return container_[ typeID ]; }

	bool visit( const EntityDefs& entityDefs, TableInspector & visitor );

private:
	PropertyMappingsPerType( const PropertyMappingsPerType & );


	static bool initPropertyMappings( PropertyMappings & properties,
		const EntityDescription & entityDescription );

	typedef BW::vector< PropertyMappings > Container;

	Container container_;
};

BW_END_NAMESPACE

#endif // PROPERTY_MAPPINGS_PER_TYPE_HPP
