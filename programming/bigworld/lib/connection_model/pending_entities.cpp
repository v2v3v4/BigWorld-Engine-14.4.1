#include "pch.hpp"

#include "pending_entities.hpp"

#include "bw_entities.hpp"

BW_BEGIN_NAMESPACE

// ----------------------------------------------------------------------------
// Section: PendingEntities
// ----------------------------------------------------------------------------

/**
 *	Constructor
 */
PendingEntities::PendingEntities( BWEntities & entities ) :
	entities_( entities ),
	collection_()
{
}


/**
 *	Destructor
 */
PendingEntities::~PendingEntities()
{
	MF_ASSERT( collection_.empty() );
}


/**
 *	This method searches our list of pending entities for a given EntityID
 *	and returns the BWEntity * if we know of it or NULL otherwise.
 *
 *	@param entityID	The EntityID to search for.
 *	@return The BWEntity * of the found entity, or NULL if not known
 */
BWEntity * PendingEntities::findEntity( EntityID entityID ) const
{
	Collection::const_iterator iter = collection_.find( entityID );

	if (iter == collection_.end())
	{
		return NULL;
	}

	return iter->second.first.get();
}


/**
 *	This method adds an entity to our list of pending entities, to later
 *	trigger BWEntities::onEntityUnblocked once it is no longer blocked.
 */
void PendingEntities::addEntity( BWEntity * pEntity,
	EntityEntryBlockingConditionImplPtr pBlockingConditionImpl )
{
	EntityID entityID = pEntity->entityID();

	MF_ASSERT( collection_.find( entityID ) == collection_.end() );

	PendingEntitiesBlockingConditionHandlerPtr pHandler(
		new PendingEntitiesBlockingConditionHandler(
			pBlockingConditionImpl.get(), *this, pEntity->entityID() ) );

	collection_[ pEntity->entityID() ] = std::make_pair( pEntity, pHandler );
}


/**
 *	This method erases an entity from our list of pending entities. We do not
 *	trigger BWEntities::onEntityUnblocked as this is used to indicate that
 *	an entity is no longer interesting to us.
 *
 *	@return true if the entity was in our collection.
 */
bool PendingEntities::eraseEntity( EntityID entityID )
{
	Collection::iterator iter = collection_.find( entityID );

	if (iter == collection_.end())
	{
		return false;
	}

	BWEntityPtr & pEntity = iter->second.first;
	PendingEntitiesBlockingConditionHandlerPtr & pHandler =
		iter->second.second;

	// Notify the handler that we no longer care about its result, and are
	// going to cut it free.
	pHandler->abort();

	pEntity->triggerOnLeaveAoI();

	collection_.erase( iter );

	return true;
}


/**
 *	This method erases any entities from our list of pending entities which
 *	are passengers on the given vehicle, and stores them in the given
 *	PassengersVector if supplied. We do not trigger
 *	BWEntities::onEntityUnblocked as this is used to indicate that an entity is
 *	no longer interesting to us.
 *
 *	@param vehicleID			The EntityID of the vehicle whose passengers we
 *								are to erase
 *	@param	pFormerPassengers	A PassengersVector to insert the deleted
 *								BWEntity's pointer into if present. Otherwise
 *								passengers will be destroyed if not a player.
 */
void PendingEntities::erasePassengers( EntityID vehicleID,
	PassengersVector * pFormerPassengers )
{
	Collection::iterator iter = collection_.begin();

	while (iter != collection_.end())
	{
		Collection::iterator iCandidate = iter++;
		BWEntityPtr & pEntity = iCandidate->second.first;

		if (pEntity->vehicleID() != vehicleID)
		{
			continue;
		}

		PendingEntitiesBlockingConditionHandlerPtr & pHandler =
			iCandidate->second.second;

		// Notify the handler that we no longer care about its result, and are
		// going to cut it free.
		pHandler->abort();

		pEntity->triggerOnLeaveAoI();

		MF_ASSERT( pEntity->vehicleID() == vehicleID );

		if (pFormerPassengers != NULL)
		{
			pFormerPassengers->push_back(
				std::make_pair( pEntity, vehicleID ) );
		}
		else
		{
			// In this case, pEntity is our last reference, so we expect it
			// to be deleted when iCandidate is erased
			pEntity->destroyNonPlayer();
		}

		collection_.erase( iCandidate );
	}
}


/**
 *	This method clears pending entities, optionally leaving local entities.
 *
 *	@param keepLocalEntities	If true, then locally created entities are
 *								not cleared.
 */
void PendingEntities::clear( bool keepLocalEntities /* = false */ )
{
	Collection::iterator iter = collection_.begin();

	while (iter != collection_.end())
	{
		PendingEntitiesBlockingConditionHandlerPtr pCondition =
			iter->second.second;
		BWEntityPtr pEntity = iter->second.first;

		pCondition->abort();

		pEntity->triggerOnLeaveAoI();

		pEntity->destroyNonPlayer();

		Collection::iterator iDelete = iter;
		++iter;
		collection_.erase( iDelete );
	}
}


/**
 *	This method moves through all our entities, processes their filters
 *	and provides the caller a list of entities which no longer have a
 *	consistent in-world representation.
 *
 *	This method does not take entities out-of-world, that's the caller's
 *	responsibility.
 *
 *	@param gameTime	The client-based game time to process the filters for
 *	@param inconsistentEntities A list of BWEntity * to populate with entities
 *			which do not have a consistent world-space representation.
 */
void PendingEntities::processEntityFilters( double gameTime,
	BW::list< BWEntity * > & inconsistentEntities )
{
	for (Collection::iterator iter = collection_.begin();
		iter != collection_.end(); ++iter)
	{
		BWEntity & entity = *(iter->second.first.get());

		if (!entity.applyFilter( gameTime ))
		{
			inconsistentEntities.push_back( &entity );
		}
	}
}


/**
 *	This method releases an entity back to the system as it is no longer
 *	blocked.
 *
 *	@param entityID	The EntityID of the entity to release
 */
void PendingEntities::releaseEntity( EntityID entityID )
{
	Collection::iterator iter = collection_.find( entityID );
	MF_ASSERT( iter != collection_.end() );

	// Avoid accidental deletion
	BWEntityPtr pEntity = iter->second.first;

	collection_.erase( iter );

	entities_.onEntityUnblocked( pEntity.get() );
}

BW_END_NAMESPACE

// pending_entities.cpp
