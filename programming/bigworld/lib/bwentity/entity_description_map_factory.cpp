#include "pch.hpp"

#include "bwentity_types.hpp"
#include "bwresource_helper.hpp"
#include "entity_description_map.hpp"
#include "entity_description_map_factory.hpp"

BWENTITY_BEGIN_NAMESPACE

/**
 * This method will do the following:
 * 1) initialize resource manager with the list of provided resource paths
 * 2) create an instance of BW::EntityDescriptionMap
 * 3) open the certain resource section
 * 4) invoke parse method of BW::EntityDescriptionMap instance
 * 5) clean up the resource manager
 * EntityDescriptionMapPtr smart pointer can be used 
 * to store the pointer returned by this method
 */
EntityDescriptionMap* 
EntityDescriptionMapFactory::create( const char** pathsToEntityDef )
{
	//Create and initialize BWResource. 
	//It will be deleted in resHelper destructor
	BWResourceHelper resHelper;
	if (!resHelper.create( pathsToEntityDef ))
	{
		ERROR_MSG( "Failed to initialise BWResource\n" );
		
		return NULL;
	}

	// Open the section
	BW::string entitiesFile( ::BW::EntityDef::Constants::entitiesFile() );
	BW::DataSectionPtr pEntities = 
			resHelper.openSection( const_cast<char *>( entitiesFile.c_str() ) );
	if (!pEntities)
	{
		ERROR_MSG( "Failed to openSection %s\n", entitiesFile.c_str() );
		return NULL;
	}
	
	// Create an instance of BW::EntityDescriptionMap
	BW::EntityDescriptionMap * pBWEntityDescrMap = new BW::EntityDescriptionMap();
	if (!pBWEntityDescrMap)
	{
		ERROR_MSG( "Failed to create BW::EntityDescriptionMap\n" );
		return NULL;
	}
	
	// Create an instance of EntityDef::EntityDescriptionMap holder
	EntityDescriptionMap * 
		pEntityDescrMap = new EntityDescriptionMap( pBWEntityDescrMap );
	if (!pEntityDescrMap)
	{
		ERROR_MSG( "Failed to create EntityDef::EntityDescriptionMap\n" );
		return NULL;
	}
	
	// Initialise the entity descriptions.
	if (!pBWEntityDescrMap->parse( pEntities,
			&BW::ClientInterface::Range::entityPropertyRange,
			&BW::ClientInterface::Range::entityMethodRange,
			&BW::BaseAppExtInterface::Range::baseEntityMethodRange,
			&BW::BaseAppExtInterface::Range::cellEntityMethodRange ))
	{
		ERROR_MSG( "BW::EntityDescriptionMap::parse failed\n" );
	}
	
	return pEntityDescrMap;	
}

/**
 * This method will delete the previously created instance of
 * BW::EntityDescriptionMap
 */
void 
EntityDescriptionMapFactory::destroy(EntityDescriptionMap*& pEntityDescriptionMap )
{
	delete pEntityDescriptionMap;
}

BWENTITY_END_NAMESPACE
