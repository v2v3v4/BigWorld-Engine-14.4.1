#ifndef BW_ENTITY_FACTORY_HPP
#define BW_ENTITY_FACTORY_HPP

#include "entity_extension_factory_manager.hpp"

#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

class BWEntity;
class BWConnection;
class EntityExtensionFactoryBase;


/**
 *	This is the abstract base class for entity factories.
 */
class BWEntityFactory
{
public:
	/** Destructor. */
	virtual ~BWEntityFactory() {}

	/**
	 *	This method creates the appropriate subclass of BWEntity based on the 
	 *	given entity type ID.
	 *
	 *	@param id				The entity ID.
	 *	@param entityTypeID		The entity type ID.
	 *	@param spaceID			The space id the entity is in.
	 *	@param data				The data to initialise the entity.
	 *	@param pConnection		The BWConnection this entity came from.
	 *
	 *	@return A pointer to a new instance of the appropriate subclass of 
	 *			BWEntity, or NULL on error.
	 */
	BWEntity * create( EntityID id, EntityTypeID entityTypeID,
			SpaceID spaceID, BinaryIStream & data, BWConnection * pConnection );

	void addExtensionFactory( EntityExtensionFactoryBase * pFactory )
	{
		entityExtensionFactoryManager_.add( pFactory );
	}

private:
	virtual BWEntity * doCreate( EntityTypeID entityTypeID,
		BWConnection * pConnection ) = 0;

	EntityExtensionFactoryManager entityExtensionFactoryManager_;
};

BW_END_NAMESPACE

#endif // BW_ENTITY_FACTORY_HPP
