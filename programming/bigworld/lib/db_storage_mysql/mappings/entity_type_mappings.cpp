#include "script/first_include.hpp"

#include "entity_type_mappings.hpp"

#include "entity_type_mapping.hpp"
#include "property_mappings_per_type.hpp"

#include "db_storage/db_entitydefs.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Default constructor.
 */
EntityTypeMappings::EntityTypeMappings()
{
}


/**
 *	Constructor.
 */
EntityTypeMappings::EntityTypeMappings( const EntityDefs & entityDefs,
	MySql & connection )
{
	this->init( entityDefs, connection );
}


/**
 *	Destructor.
 */
EntityTypeMappings::~EntityTypeMappings()
{
	this->clearContainer();
}


/**
 * 	This function creates EntityTypeMappings from the given
 * 	PropertyMappings.
 */
bool EntityTypeMappings::init( const EntityDefs & entityDefs,
	MySql & connection )
{
	PropertyMappingsPerType propMappingsPerType;
	MF_VERIFY( propMappingsPerType.init( entityDefs ) );

	for (EntityTypeID typeID = 0;
			typeID < entityDefs.getNumEntityTypes();
			++typeID)
	{
		if (entityDefs.isValidEntityType( typeID ))
		{
			const EntityDescription & entityDesc =
				entityDefs.getEntityDescription( typeID );
			const DataDescription *pIdentifier = entityDesc.pIdentifier();

			BW::string identiferProperty;
			if (pIdentifier)
			{
				identiferProperty = pIdentifier->name();
			}

			try 
			{
				container_.push_back( new EntityTypeMapping( connection,
										entityDesc,
										propMappingsPerType[ typeID ],
										identiferProperty ) );
			}
			catch (std::exception & e)
			{
				this->clearContainer();
				return false;	
			}
		}
		else
		{
			container_.push_back( NULL );
		}
	}

	return true;
}

void EntityTypeMappings::clearContainer()
{
	Container::iterator iter = container_.begin();

	while (iter != container_.end())
	{
		delete *iter;

		++iter;
	}
}

BW_END_NAMESPACE


// entity_type_mappings.cpp
