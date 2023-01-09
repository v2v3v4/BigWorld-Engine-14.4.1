#ifndef MYSQL_ENTITY_MAPPING_HPP
#define MYSQL_ENTITY_MAPPING_HPP

#include "../table.hpp" // For TableProvider

#include "property_mapping.hpp" // For PropertyMappings


BW_BEGIN_NAMESPACE

class EntityDescription;


/**
 * 	This class contains the property mappings for an entity type.
 */
class EntityMapping : public TableProvider
{
public:
	EntityMapping( const EntityDescription & entityDesc,
			const PropertyMappings & properties,
			const BW::string & tableNamePrefix = TABLE_NAME_PREFIX );
	virtual ~EntityMapping() {};

	EntityTypeID getTypeID() const;
	const BW::string & typeName() const;

	// TableProvider overrides
	virtual const BW::string & getTableName() const {	return tableName_;	}

	virtual bool visitColumnsWith( ColumnVisitor & visitor );
	virtual bool visitIDColumnWith( ColumnVisitor & visitor );
	virtual bool visitSubTablesWith( TableVisitor & visitor );

protected:
	const PropertyMappings & getPropertyMappings() const
			{ return properties_; }
	const EntityDescription & getEntityDescription() const
			{ return entityDesc_; }

private:
	const EntityDescription & 	entityDesc_;
	BW::string					tableName_;
	PropertyMappings			properties_;
};

BW_END_NAMESPACE

#endif // MYSQL_ENTITY_MAPPING_HPP
