#ifndef BWENTITY_ENTITY_DESCRIPTION_MAP_HPP
#define BWENTITY_ENTITY_DESCRIPTION_MAP_HPP

#include "bwentity_api.hpp"
#include "reference_count.hpp"

BWENTITY_BEGIN_NAMESPACE

/**
 * 	This class parses the entities.xml file, and stores a map of entity
 * 	descriptions.  It provides access to entity descriptions by their name, or
 * 	by index. It also provides fast mapping from name to index.
 *  @see BW::EntityDescriptionMap
 *  
 */
class EntityDescriptionMap: public ReferenceCount< EntityDescriptionMap >
{
	friend class EntityDescriptionMapFactory;
public:
	~EntityDescriptionMap();
	
	BWENTITY_API bool
	nameToIndex( const BW::string & name, EntityTypeID & index ) const;
	BWENTITY_API int		size() const;

	BWENTITY_API
	const BW::EntityDescription & entityDescription( EntityTypeID index ) const;

	BWENTITY_API const BW::EntityDefConstants& 	connectionConstants();
private:
	/**
	 * The object can be constructed by EntityDescriptionMapFactory factory only
	 */
	EntityDescriptionMap( const BW::EntityDescriptionMap * ptr );
	
	const BW::EntityDescriptionMap * pimpl_;
	const BW::EntityDefConstants * pConstants_;
};

BWENTITY_END_NAMESPACE

#endif /* BWENTITY_ENTITY_DESCRIPTION_MAP_HPP */
