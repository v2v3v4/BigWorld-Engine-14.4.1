#ifndef BWENTITY_ENTITY_DESCRIPTION_MAP_FACTORY_HPP
#define BWENTITY_ENTITY_DESCRIPTION_MAP_FACTORY_HPP

BWENTITY_BEGIN_NAMESPACE
/**
 * This call is the factory to create and free EntityDescriptionMap instances
 */
class BWENTITY_API EntityDescriptionMapFactory
{
public:
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
	static EntityDescriptionMap* create( const char** pathsToEntityDef );
	
	/**
	 * This method will delete the previously created instance of
	 * BW::EntityDescriptionMap
	 */
	static void destroy( EntityDescriptionMap*& pEntityDescriptionMap );
};

BWENTITY_END_NAMESPACE

#endif /* BWENTITY_ENTITY_DESCRIPTION_MAP_FACTORY_HPP */
