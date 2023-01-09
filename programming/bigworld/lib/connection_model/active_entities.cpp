#include "pch.hpp"

#include "active_entities.hpp"

#include "bw_entities.hpp"
#include "bw_entity.hpp"
#include "connection/movement_filter.hpp"

BW_BEGIN_NAMESPACE

// ----------------------------------------------------------------------------
// Section: EntityEntryBlockingConditionHandler
// ----------------------------------------------------------------------------

/// C++ Housekeeping
/**
 *	Constructor
 */
ActiveEntities::ActiveEntities( BWEntities & entities ) :
	entities_( entities ),
	listeners_()
{

}


/**
 *	Destructor
 */
ActiveEntities::~ActiveEntities()
{

}


/// Entity List Management
/**
 *	This method searches our list of entities for a given EntityID and returns
 *	the BWEntity * if we know of it, and NULL otherwise.
 *
 *	@param entityID	The EntityID to search for.
 *	@return The BWEntity * of the found entity, or NULL if not known
 */
BWEntity * ActiveEntities::findEntity( EntityID entityID ) const
{
	Collection::const_iterator iEntity = collection_.find( entityID );

	return (iEntity != collection_.end()) ? iEntity->second.get() : NULL;
}


/**
 *	This method add the given BWEntity to our list of known entities, and
 *	being ready to enter the World.
 *
 *	This will trigger the onEnter listener callback if the entity was known.
 *
 *	@param pEntity	The BWEntity to add
 *	@see ActiveEntities::eraseCellEntity
 */
void ActiveEntities::addEntityToWorld( BWEntity * pEntity )
{
	MF_ASSERT( collection_.find( pEntity->entityID() ) ==
		collection_.end() );

	collection_[ pEntity->entityID() ] = pEntity;

	pEntity->triggerOnEnterWorld();

	this->callListeners( &BWEntitiesListener::onEntityEnter, pEntity );

}


/**
 *	This method erases an entity from our collection of in-world entities.
 *
 *	This will trigger the onLeave listener callback if the entity was known.
 *
 *	This method will be recursively called for passengers of this entity.
 *
 *	@param entityID	The EntityID of the entity to attempt to erase.
 *	@param pFormerPassengers An optional pointer to a BW::vector of
 *			BWEntityPtr, EntityID pairs to be populated with the passengers and
 *			their vehicleIDs that were on the removed Entity. They will also be
 *			removed from the world before this function returns. This vector
 *			does not include the entity being initially removed.
 *			If this is NULL, the passenger entities will be removed from world,
 *			destroyed if not a player, and no references will be held.
 *
 *	@return true if the entity was in our collection and in-world.
 *	@see ActiveEntities::addCellEntity
 */
bool ActiveEntities::removeEntityFromWorld( EntityID entityID,
	PassengersVector * pFormerPassengers )
{
	Collection::iterator iEntity = collection_.find( entityID );

	if (iEntity == collection_.end())
	{
		return false;
	}

	this->doRemoveEntityFromWorld( iEntity, pFormerPassengers );		
	return true;
}


/**
 *	This method erases an entry from the in-world entity collection.
 *
 *	@param iter		The iterator pointing to the entry to remove.
 *	@param pFormerPassengers An optional pointer to a BW::vector of
 *			BWEntityPtr, EntityID pairs to be populated with the passengers and
 *			their vehicleIDs that were on the removed Entity. They will also be
 *			removed from the world before this function returns. This vector
 *			does not include the entity being initially removed.
 *			If this is NULL, the passenger entities will be removed from world,
 *			destroyed if not a player, and no references will be held.
 */
void ActiveEntities::doRemoveEntityFromWorld( Collection::iterator iter,
		PassengersVector * pFormerPassengers /* = NULL */ )
{
	BWEntityPtr pEntity = iter->second;

	entities_.onEntityInconsistent( pEntity.get(), pFormerPassengers );

	// Notify the entity that it's left the world
	pEntity->triggerOnLeaveWorld();

	// Now take it out of our inWorld entities list
	collection_.erase( iter );

	MF_ASSERT( pEntity->passengers().empty() );

	this->callListeners( &BWEntitiesListener::onEntityLeave, pEntity.get() );

	pEntity->triggerOnLeaveAoI();
}


/**
 *	This method clears our list of in-world entities, optionally leaving
 *	locally created entities.
 *
 *	After calling this, we will be in the state of having just been
 *	initialised.
 *
 *	@param keepLocalEntities 	If true, locally created entities are not
 *								cleared.
 */
void ActiveEntities::clearEntities( bool keepLocalEntities )
{
	// We make a copy of the collection to iterate over because 
	// removeEntityFromWorld() will mutate collection_, possibly multiple
	// additional times for each passenger, one of which might end up being 
	// the next element to be iterated over, and so iterating over collection_
	// directly in the usual increment-then-erase fashion is unsafe. 
	// This might be very big, but clearEntities() shouldn't be called that
	// regularly.

	Collection collectionCopy( collection_ );

	Collection::iterator iEntity = collectionCopy.begin();
	while (iEntity != collectionCopy.end())
	{
		BWEntityPtr pEntity = iEntity->second;
		
		if (!keepLocalEntities || !pEntity->isLocalEntity())
		{
			if (this->removeEntityFromWorld( iEntity->first, NULL ))
			{
				// If we failed to remove this entity from world, then it's 
				// likely we have removed it earlier as the passenger of a
				// vehicle.
				pEntity->destroyNonPlayer();
			}
		}

		++iEntity; 
	}
}


/**
 *	This method clears our list of in-world entities, except for the given
 *	Entity.
 *
 *	No callbacks will be called for the exception Entity, unless it is forced
 *	out of the world due to a different Entity leaving.
 *
 *	@param exceptEntityID The EntityID of the BWEntity we want to keep active.
 *	@return BWEntity that we wanted to keep active, if that was not possible,
 *		or NULL if that entity is still in our active list, or was already not
 *		active.
 */
BWEntityPtr ActiveEntities::clearEntitiesWithException(
	EntityID exceptEntityID )
{
	Collection::iterator iEntity = collection_.find( exceptEntityID );

	if (iEntity == collection_.end())
	{
		// Shortcut case: The exception is not in our list.
		this->clearEntities();
		return NULL;
	}

	// This protects against accidental deletion further down.
	BWEntityPtr pException = iEntity->second;

	// TODO: This is "assuming" that the only reason pException may be forced
	// out of the world is if it has a vehicle.
	if (pException->pVehicle() != NULL)
	{
		// Take the vehicle out of the world.
		BWEntityPtr pVehicle = pException->pVehicle();

		// TODO: Don't waste the map lookup, we know it's there.
		MF_VERIFY( this->removeEntityFromWorld( pVehicle->entityID(), NULL ) );
		pVehicle->destroyNonPlayer();

		// If the exception is no longer in world, this is really a full-reset.
		if (!pException->isInWorld())
		{
			this->clearEntities();
			return pException;
		}
	}

	// Take the exception out of the list while we reset it, then put it back
	// in.
	collection_.erase( iEntity );
	this->clearEntities();
	collection_[ pException->entityID() ] = pException;

	return NULL;
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
void ActiveEntities::processEntityFilters( double gameTime,
	BW::list< BWEntity * > & inconsistentEntities )
{
	for (Collection::iterator iEntity = collection_.begin();
		iEntity != collection_.end(); ++iEntity)
	{
		BWEntity & entity = *(iEntity->second);

		if (!entity.applyFilter( gameTime ))
		{
			inconsistentEntities.push_back( &entity );
		}
	}
}


/**
 *	This method moves through all our entities, and sends the current
 *	position of any locally-controlled Entities to the server.
 *
 *	@param gameTime	The client-based game time at which we are sending
 */
void ActiveEntities::sendControlledEntitiesUpdates( double gameTime )
{
	for (Collection::iterator iEntity = collection_.begin();
		iEntity != collection_.end(); ++iEntity)
	{
		BWEntity & entity = *(iEntity->second);

		if (!entity.isControlled() || entity.isLocalEntity() ||
			entity.isPhysicsCorrected())
		{
			continue;
		}

		entity.sendLatestLocalMove();
	}
}


/// BWEntitiesListener registration
/**
 *	This method adds a listener to this object. It will be informed whenever an
 *	entity is added or removed from this collection.
 *
 *	@param pListener 	The listener to add.
 *
 *	@see removeListener
 */
void ActiveEntities::addListener( BWEntitiesListener * pListener )
{
	listeners_.push_back( pListener );
}


/**
 *	This method removes a registered listener to this object.
 *
 *	@param pListener 	The listener to remove.
 *
 *	@return 			true if the listener was removed, false if no such
 *						listener was registered.
 *
 *	@see addListener
 */
bool ActiveEntities::removeListener( BWEntitiesListener * pListener )
{
	Listeners::iterator iter = std::find( listeners_.begin(), listeners_.end(),
			pListener );

	if (iter == listeners_.end())
	{
		WARNING_MSG( "ActiveEntities::removeListener: "
				"Unable to remove listener.\n" );
		return false;
	}

	listeners_.erase( iter );

	return true;
}


/// BWEntitiesListener support


/**
 *	This method notifies listeners that entities have been reset.
 */
void ActiveEntities::notifyListenersOfEntitiesReset() const
{
	Listeners::const_iterator iter = listeners_.begin();

	while (iter != listeners_.end())
	{
		BWEntitiesListener * pListener = *iter;
		++iter;

		pListener->onEntitiesReset();
	}
}


/**
 *	This method informs the listeners that an event has occurred.
 *
 *	@param func 	The callback to call.
 *	@param pEntity 	The entity to pass to the callback function.
 */
void ActiveEntities::callListeners( ActiveEntities::ListenerFunc func,
	   BWEntity * pEntity ) const
{
	Listeners::const_iterator iter = listeners_.begin();

	while (iter != listeners_.end())
	{
		BWEntitiesListener * pListener = *iter;
		++iter;

		(pListener->*func)( pEntity );
	}
}

BW_END_NAMESPACE

// bw_entities.cpp
