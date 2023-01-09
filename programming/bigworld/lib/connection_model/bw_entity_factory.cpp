#include "pch.hpp"

#include "bw_entity_factory.hpp"

#include "bw_entity.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This method creates the appropriate subclass of BWEntity based on the 
 *	given entity type ID.
 *
 *	@param entityTypeID		The entity type ID.
 *
 *	@return A pointer to a new instance of the appropriate subclass of 
 *			BWEntity, or NULL on error.
 */
BWEntity * BWEntityFactory::create( EntityID id, EntityTypeID entityTypeID,
		SpaceID spaceID, BinaryIStream & data, BWConnection * pConnection )
{
	BWEntity * pNewEntity = this->doCreate( entityTypeID, pConnection );

	if (!pNewEntity)
	{
		ERROR_MSG( "BWEntityFactory::create: Failed for entity type %d\n",
				entityTypeID );
		return NULL;
	}

	if (!pNewEntity->init( id, entityTypeID, spaceID, data ))
	{
		ERROR_MSG( "BWEntityFactory::create: "
					"init failed for %d. entityTypeID = %d\n",
				id, entityTypeID );

		// Delete the entity.
		MF_ASSERT( !pNewEntity->isPlayer() );
		pNewEntity->destroyNonPlayer();
		BWEntityPtr pDeleted( pNewEntity );
		return NULL;
	}

	pNewEntity->createExtensions( entityExtensionFactoryManager_ );

	return pNewEntity;
}

BW_END_NAMESPACE

// bw_entity_factory.cpp
