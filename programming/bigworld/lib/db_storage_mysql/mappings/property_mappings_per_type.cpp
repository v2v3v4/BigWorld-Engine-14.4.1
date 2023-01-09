#include "script/first_include.hpp"

#include "property_mappings_per_type.hpp"

#include "entity_type_mapping.hpp"
#include "num_mapping.hpp"
#include "timestamp_mapping.hpp"
#include "vector_mapping.hpp"

#include "../namer.hpp"
#include "../table_inspector.hpp"

#include "cstdmf/debug.hpp"
#include "db_storage/db_entitydefs.hpp"
#include "resmgr/xml_section.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: free functions
// -----------------------------------------------------------------------------

namespace
{

/**
 * 	This method gets the default value section for the DataDescription.
 */
DataSectionPtr getDefaultSection( const DataDescription & dataDesc )
{
	DataSectionPtr pDefaultSection = dataDesc.pDefaultSection();

	if (!pDefaultSection)
	{
		pDefaultSection = dataDesc.dataType()->pDefaultSection();
	}

	return pDefaultSection;
}

} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: PropertyMappingsPerType
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PropertyMappingsPerType::PropertyMappingsPerType()
{
}


/**
 *	This method initialises this object.
 *
 *	@return True on success, otherwise false.
 */
bool PropertyMappingsPerType::init( const EntityDefs & entityDefs )
{
	bool isOkay = true;

	// walk through the properties of each entity type and create a property mapping
	// class instance for it... we'll use these to generate the statements that we'll
	// need later on
	for (EntityTypeID typeID = 0;
			typeID < entityDefs.getNumEntityTypes();
			++typeID)
	{
		// Note that even for invalid entity types we need an blank entry since
		// we access by offset into the array.
		container_.push_back( PropertyMappings() );

		if (entityDefs.isValidEntityType( typeID ))
		{
			const EntityDescription & description =
					entityDefs.getEntityDescription( typeID );

			if (!this->initPropertyMappings( container_.back(), description ))
			{
				ERROR_MSG( "PropertyMappings::init: "
								"Could not create mapping for entity type %s\n",
							description.name().c_str() );
				isOkay = false;
			}
		}
	}

	return isOkay;
}


namespace
{

class PropertyMappingsIniter : public IDataDescriptionVisitor
{
public:
	PropertyMappingsIniter( PropertyMappings & properties, Namer & namer ) :
		properties_( properties ),
		namer_( namer )
	{
	}

	virtual bool visit( const DataDescription & dataDesc )
	{
		PropertyMappingPtr prop = PropertyMapping::create( namer_,
					dataDesc.fullName(),
					*dataDesc.dataType(),
					dataDesc.databaseLength(),
					getDefaultSection( dataDesc ),
					dataDesc.databaseIndexingType() );

		if (!prop.exists())
		{
			return false;
		}

		properties_.push_back( prop );
		return true;
	}

private:
	PropertyMappings & properties_;
	Namer & namer_;
};

} // anonymous namespace

bool PropertyMappingsPerType::initPropertyMappings(
		PropertyMappings & properties, const EntityDescription & entDes )
{
	Namer namer( entDes.name(), TABLE_NAME_PREFIX );

	// First create mappings for properties in EntityDescription.
	PropertyMappingsIniter initer( properties, namer );
	bool isOkay = entDes.visit( EntityDescription::BASE_DATA |
					EntityDescription::CELL_DATA |
					EntityDescription::ONLY_PERSISTENT_DATA,
				initer );

	if (entDes.canBeOnCell())
	{
		//setup proper default value for position and direction
		Vector3 defaultVec(0,0,0);

		DataSectionPtr pDefaultValue = new XMLSection( "position" );
		pDefaultValue->setVector3(defaultVec);

		properties.push_back(
			new VectorMapping<Vector3,3>( namer, "position", pDefaultValue ) );

		pDefaultValue = new XMLSection( "direction" );
		pDefaultValue->setVector3(defaultVec);
		properties.push_back(
			new VectorMapping<Vector3,3>( namer, "direction", pDefaultValue ) );

		pDefaultValue = new XMLSection( "spaceID" );
		pDefaultValue->setInt( 0 );
		properties.push_back(
			new NumMapping<int32>( namer, "spaceID", pDefaultValue ) );
	}

	DataSectionPtr pDefaultValue = new XMLSection( GAME_TIME_COLUMN_NAME );
	pDefaultValue->setInt( 0 );
	properties.push_back(
		new NumMapping<GameTime>( GAME_TIME_COLUMN_NAME, pDefaultValue ) );

	properties.push_back( new TimestampMapping() );

	return isOkay;
}


/**
 *	Visits all entity tables with visitor and collects list of current entity
 * 	types.
 */
bool PropertyMappingsPerType::visit( const EntityDefs & entityDefs,
		TableInspector & visitor )
{
	TypesCollector typesCollector( visitor.connection() );

	for (EntityTypeID typeID = 0;
			typeID < entityDefs.getNumEntityTypes();
			++typeID)
	{
		// Skip over "invalid" entity types e.g. client-only entities.
		if (entityDefs.isValidEntityType( typeID ))
		{
			PropertyMappings& properties = container_[ typeID ];
			const EntityDescription & entDes =
							entityDefs.getEntityDescription( typeID );

			// Create/check tables for this entity type
			EntityMapping entityMapping( entDes, properties );
			entityMapping.visitSubTablesRecursively( visitor );

			typesCollector.addType( typeID, entDes.name() );

			if (properties.empty())
			{
				INFO_MSG( "%s does not have persistent properties.\n",
						  entDes.name().c_str() );
			}
		}
	}

	if (visitor.deleteUnvisitedTables())
	{
		typesCollector.deleteUnwantedTypes();
	}

	return visitor.isSynced();
}

BW_END_NAMESPACE

// property_mappings_per_type.cpp
