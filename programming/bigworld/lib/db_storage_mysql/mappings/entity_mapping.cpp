#include "script/first_include.hpp"

#include "entity_mapping.hpp"

#include "entitydef/entity_description.hpp"


BW_BEGIN_NAMESPACE

/**
 * 	Constructor.
 */
EntityMapping::EntityMapping( const EntityDescription & entityDesc,
		const PropertyMappings & properties, const BW::string & tableNamePrefix ) :
	entityDesc_( entityDesc ),
	tableName_( tableNamePrefix + "_" + entityDesc.name() ),
	properties_( properties )
{
}


/**
 * 	Gets the type ID of the entity type associated with this entity mapping.
 */
EntityTypeID EntityMapping::getTypeID() const
{
	return entityDesc_.index();
}


/*
 * 	Gets the type name of the entity type associated with this entity mapping.
 */
const BW::string & EntityMapping::typeName() const
{
	return entityDesc_.name();
}


/*
 * 	Override from TableProvider. Visit all our columns, except the ID column.
 */
bool EntityMapping::visitColumnsWith( ColumnVisitor & visitor )
{
	for (PropertyMappings::iterator iter = properties_.begin();
			iter != properties_.end(); ++iter)
	{
		if (!(*iter)->visitParentColumns( visitor ))
		{
			return false;
		}
	}

	return true;
}


/*
 * 	Override from TableProvider. Visit our ID column.
 */
bool EntityMapping::visitIDColumnWith(	ColumnVisitor & visitor )
{
	ColumnDescription idColumn( ID_COLUMN_NAME_STR, ID_COLUMN_TYPE,
			INDEX_TYPE_PRIMARY );

	return visitor.onVisitColumn( idColumn );
}


/*
 * 	Override from TableProvider. Visit all our sub-tables.
 */
bool EntityMapping::visitSubTablesWith( TableVisitor & visitor )
{
	for ( PropertyMappings::iterator iter = properties_.begin();
			iter != properties_.end(); ++iter )
	{
		if (!(*iter)->visitTables( visitor ))
		{
			return false;
		}
	}

	return true;
}

BW_END_NAMESPACE

// entity_mapping.cpp
