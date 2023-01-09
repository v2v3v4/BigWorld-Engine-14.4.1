#include "pch.hpp"

#include "bw_entities.hpp"

#include "bw_connection.hpp"
#include "bw_entities_listener.hpp"
#include "bw_entity_factory.hpp"
#include "entity_entry_blocker.hpp"

#include "cstdmf/binary_stream.hpp"
#if ENABLE_WATCHERS
#include "cstdmf/watcher.hpp"
#endif // ENABLE_WATCHERS

#include "connection/movement_filter.hpp"

namespace
{

inline bool isLocalEntityID( BW::EntityID entityID )
{
	return entityID >= BW::FIRST_LOCAL_ENTITY_ID;
}

}

/// C++ Housekeeping

#ifdef _MSC_VER
#pragma warning( push )
// C4355: 'this' : used in base member initializer list
#pragma warning( disable: 4355 )
#endif // _MSC_VER

BW_BEGIN_NAMESPACE


EntityID BWEntities::s_nextLocalEntityID_ = FIRST_LOCAL_ENTITY_ID;


/**
 *	Constructor.
 */
BWEntities::BWEntities( BWConnection & connection,
			BWEntityFactory & entityFactory ):
		connection_( connection ),
		entityFactory_( entityFactory ),
		activeEntities_( *this ),
		pendingPassengers_( *this ),
		appPendingEntities_( *this ),
		pPlayer_( NULL )
{
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif // _MSC_VER


/**
 *	Destructor.
 */
BWEntities::~BWEntities()
{
	// Simulate a full server reset so that any still-referenced entities
	// are in the correct (i.e. destroyed and out-of-world) state.
	this->clear( /* keepPlayerBaseEntity */ false,
		/* keepLocalEntities */ false );

	MF_ASSERT( pPlayer_ == NULL );
	MF_ASSERT( activeEntities_.empty() );
	MF_ASSERT( appPendingEntities_.empty() );
	MF_ASSERT( pendingPassengers_.empty() );
}


/**
 *
 */
void BWEntities::handleBasePlayerCreate( EntityID id,
		EntityTypeID entityTypeID, BinaryIStream & data )
{
	MF_ASSERT( !isLocalEntityID( id ) );

	BWEntityPtr pEntity = entityFactory_.create( id, entityTypeID,
			NULL_SPACE_ID, data, &connection_ );

	if (!pEntity)
	{
		ERROR_MSG( "BWEntities::handleBasePlayerCreate: Failed.\n" );
		return;
	}

	// We should have been totally reset before seeing this
	// TODO: Client-only entities might be okay here...
	MF_ASSERT( pPlayer_ == NULL );
	MF_ASSERT( activeEntities_.empty() );
	MF_ASSERT( appPendingEntities_.empty() );
	MF_ASSERT( pendingPassengers_.empty() );

	pPlayer_ = pEntity;
	pPlayer_->triggerOnBecomePlayer();
}


/*
 *	Override from ServerMessageHandler.
 */
void BWEntities::handleCellPlayerCreate( EntityID id, SpaceID spaceID,
		EntityID vehicleID, const Position3D & pos, 
		float yaw, float pitch, float roll, 
		BinaryIStream & data )
{
	MF_ASSERT( !isLocalEntityID( id ) );

	BWEntity * pEntity = this->pPlayer();
	MF_ASSERT( pEntity != NULL );
	MF_ASSERT( pEntity->entityID() == id );

	// TODO: Error handling.
	MF_VERIFY( pEntity->updateCellPlayerProperties( data ) );

	Direction3D direction( Vector3( roll, pitch, yaw ) );

	this->addEntity( pEntity, spaceID, vehicleID, pos, direction );
}


/*
 *	@see ServerMessageHandler::onEntityCreate
 */
void BWEntities::handleEntityCreate( EntityID id, 
		EntityTypeID entityTypeID,
		SpaceID spaceID, EntityID vehicleID, const Position3D & position,
		float yaw, float pitch, float roll, BinaryIStream & data )
{
	MF_ASSERT( !isLocalEntityID( id ) );

	BWEntityPtr pEntity = entityFactory_.create( id, entityTypeID,
			spaceID, data, &connection_ );

	if (!pEntity)
	{
		ERROR_MSG( "BWServerMessageHandler::handleEntityCreate: "
					"Failed for entity %d\n",
				id );
		return;
	}

	Direction3D direction( Vector3( roll, pitch, yaw ) );

	this->addEntity( pEntity.get(), spaceID, vehicleID, position,
		direction );
}


/**
 *	This method adds an entity to the collection of in-space objects.
 *
 *	Entities added here may not immediately be visible in the entity
 *	list if they are deferred for any reason.
 *
 *	@param pEntity 	The entity to add.
 *	@param spaceID	The space ID to use for this entity addition.
 *	@param vehicleID The vehicle ID to apply (if any).
 *	@param position The position of the entity.
 *	@param direction The direction of the entity.
 */
void BWEntities::addEntity( BWEntity * pEntity,
	const SpaceID & spaceID, const EntityID & vehicleID,
	const Position3D & position, const Direction3D & direction )
{
	const EntityID entityID = pEntity->entityID();
	MF_ASSERT( pendingPassengers_.findPassenger( entityID ) == NULL );
	MF_ASSERT( appPendingEntities_.findEntity( entityID ) == NULL );
	MF_ASSERT( activeEntities_.findEntity( entityID ) == NULL );

	MF_ASSERT( pEntity->pFilter() == NULL );

	pEntity->onMoveFromServer( connection_.lastMessageTime(), position,
		vehicleID, spaceID, direction, Vector3::zero(),
		/* isTeleport */ true );

	// If this doesn't need to be pending, it should call
	// back immediately into BWEntities::onEntityConsistent
	pendingPassengers_.addPassenger( pEntity, vehicleID );
}


/**
 *	This method notifies us that an Entity is consistent.
 *
 *	Consistency means the entity has a valid and rational
 *	world position, and its vehicle is in-world.
 *
 *	@param pEntity 	The entity to add.
 */
void BWEntities::onEntityConsistent( BWEntity * pEntity )
{
	const EntityID entityID = pEntity->entityID();
	// The caller is responsible for taking this entity out of its container
	MF_ASSERT( pendingPassengers_.findPassenger( entityID ) == NULL );
	MF_ASSERT( appPendingEntities_.findEntity( entityID ) == NULL );
	MF_ASSERT( activeEntities_.findEntity( entityID ) == NULL );

	// At this point, the "current" position is correct and valid

	EntityEntryBlocker blocker;

	pEntity->triggerOnEnterAoI( blocker );

	EntityEntryBlockingConditionImplPtr pImpl = blocker.getImpl();

	if (pImpl == NULL)
	{
		// Not blocked
		activeEntities_.addEntityToWorld( pEntity );
	}
	else
	{
		appPendingEntities_.addEntity( pEntity, pImpl );
	}

}


/**
 *	This method notifies us that an Entity is no longer consistent.
 *
 *	Consistency means the entity has a valid and rational
 *	world position, and its vehicle is in-world.
 *
 *	@param pEntity 				The entity to remove.
 *	@param pFormerPassengers	A PassengersVector to populate with passengers
 *								of this entity, or NULL to just destroy the
 *								the passengers (except the player) and forget
 *								the BWEntity reference.
 */
void BWEntities::onEntityInconsistent( BWEntity * pEntity,
	PassengersVector * pFormerPassengers )
{
	const EntityID entityID = pEntity->entityID();
	// The caller is responsible for taking this entity out of these containers
	MF_ASSERT( pendingPassengers_.findPassenger( entityID ) == NULL );
	MF_ASSERT( appPendingEntities_.findEntity( entityID ) == NULL );

	// All we do with inconsistent entities is ensure their passengers are also
	// marked as inconsistent, and that everything is added to
	// pFormerPassengers if necessary.
	appPendingEntities_.erasePassengers( entityID, pFormerPassengers );

	if (pFormerPassengers)
	{
		pFormerPassengers->reserve(
			pEntity->passengers().size() + pFormerPassengers->size() );
	}

	while (!pEntity->passengers().empty())
	{
		BWEntity::Passengers::const_iterator iPassenger =
			pEntity->passengers().begin();

		BWEntityPtr pPassenger = *iPassenger;

		MF_ASSERT( pPassenger->vehicleID() == entityID );
		MF_ASSERT( pPassenger->pVehicle() == pEntity );

		if (pFormerPassengers)
		{
			pFormerPassengers->push_back(
				std::make_pair( pPassenger, entityID ) );
		}

		MF_VERIFY( activeEntities_.removeEntityFromWorld(
			pPassenger->entityID(), pFormerPassengers ) );

		if (!pFormerPassengers)
		{
			// In this case, pPassenger is our last reference, so we expect it
			// to be deleted when pPassenger leaves scope.
			pPassenger->destroyNonPlayer();
		}
	}
}


/**
 *	This method notifies us that an Entity is ready to enter the world
 *
 *	This means the entity is consistent and the application is not
 *	blocking entry of the entity.
 *
 *	@param pEntity 	The entity to add.
 */
void BWEntities::onEntityUnblocked( BWEntity * pEntity )
{
	const EntityID entityID = pEntity->entityID();
	// The caller is responsible for taking this entity out of its container
	MF_ASSERT( pendingPassengers_.findPassenger( entityID ) == NULL );
	MF_ASSERT( appPendingEntities_.findEntity( entityID ) == NULL );
	MF_ASSERT( activeEntities_.findEntity( entityID ) == NULL );

	MF_ASSERT( pEntity->spaceID() != NULL_SPACE_ID );

	activeEntities_.addEntityToWorld( pEntity );
}


/**
 *	This method removes an entity from this collection.
 *
 *	@param entityID	The ID of the entity to remove.
 *	@return True if id was a known Entity, false otherwise
 */
bool BWEntities::handleEntityLeave( EntityID entityID )
{
	// We should never be asked to remove the player.
	// Use BWEntities::clear() if you're doing that.
	MF_ASSERT( pPlayer_ == NULL || pPlayer_->entityID() != entityID );

	BWEntityPtr pEntity = this->findAny( entityID );

	if (pEntity == NULL)
	{
		// This may happen if an entity entered and left without being
		// created in between
		return false;
	}

	PassengersVector vPassengers;

	if (activeEntities_.removeEntityFromWorld( entityID, &vPassengers ))
	{
		for (PassengersVector::iterator iPassenger = vPassengers.begin();
			iPassenger != vPassengers.end(); ++iPassenger )
		{
			BWEntity * pPassenger = iPassenger->first.get();
			EntityID vehicleID = iPassenger->second;

			MF_ASSERT( vehicleID == pPassenger->vehicleID() );

			pendingPassengers_.addPassenger( pPassenger, vehicleID );
		}
	}
	else
	{
		MF_VERIFY( appPendingEntities_.eraseEntity( entityID ) ||
			pendingPassengers_.erasePassenger( entityID ) );
	}

	pEntity->destroyNonPlayer();

	return true;
}


/**
 *	This method clears out the known entity list.
 *
 *	The relevant callbacks will still be called.
 *
 *	@param keepPlayerBaseEntity If true, the player will be kept as a base-only
 *								entity. If false, the player will be erased as
 *								well.
 *	@param keepLocalEntities	If true, any locally created entities will be
 *								kept.
 */
void BWEntities::clear( bool keepPlayerBaseEntity /* = false */,
	bool keepLocalEntities /* = false */ )
{
	appPendingEntities_.clear( keepLocalEntities );

	pendingPassengers_.clear( keepLocalEntities );

	activeEntities_.clearEntities( keepLocalEntities );

	if (!keepPlayerBaseEntity && pPlayer_)
	{
		pPlayer_->triggerOnBecomeNonPlayer();
		MF_ASSERT( !pPlayer_->isPlayer() );
		pPlayer_->destroyNonPlayer();
		pPlayer_ = NULL;
	}

	activeEntities_.notifyListenersOfEntitiesReset(); 
}


/**
 *	This method notifies us that our connected entity has been rolled back
 *	to an earlier state, and that we will soon receive all the other Cell
 *	entities again.
 *
 *	@param entityID The connected entity ID that has been rolled back.
 *	@param spaceID The space that the connected entity lives in.
 *	@param vehicleID The vehicle that the connected entity is riding, or
 *			NULL_ENTITY_ID if not on a vehicle.
 *	@param pos The current local position of the Entity
 *	@param dir The current local direction of the Entity
 *	@param data A stream of all the entity's client-base properties followed by
 *			all the entity's client-cell properties.
 */
void BWEntities::handleRestoreClient( EntityID entityID, SpaceID spaceID,
	EntityID vehicleID, const Position3D & pos, const Direction3D & dir,
	BinaryIStream & data)
{
	// This method should result in the same system state as the following:
	// this->handleEntitiesReset( /* keepPlayerBaseEntity */ false );
	// this->handleBasePlayerCreate( entityID, pPlayer_->entityTypeID(), data );
	// this->handleCellPlayerCreate( entityID, spaceID, vehicleID, pos,
	// 		dir.yaw, dir.pitch, dir.roll, data );

	// However, since we know it's all coming at once, we can be smarter, and
	// both let the app-layer know what's happened, and also avoid destroying
	// and recreating the Player entity.
	// This adds an extra complication that the player may have just lost its
	// vehicle.

	MF_ASSERT( pPlayer_ != NULL );
	MF_ASSERT( entityID == pPlayer_->entityID() );

	// If we control the Entity, we assume we're in the right place, and will
	// send our current position next tick anyway, letting the server's
	// movement checks override us if necessary.
	if (!pPlayer_->isControlled())
	{
		// We know where the server considers the player entity to be, so get
		// it in place, which also updates the vehicle.
		pPlayer_->onMoveFromServer( connection_.lastMessageTime(), pos,
			vehicleID, spaceID, dir, Vector3::zero(), /* isTeleport */ true );
	}
	else
	{
		// Ensure our opinion of where the player is is up-to-date.
		pPlayer_->applyFilter( connection_.lastMessageTime() );
	}

	// Reset everything except the given EntityID
	BWEntityPtr pLostException =
		activeEntities_.clearEntitiesWithException( entityID );
	appPendingEntities_.clear();
	pendingPassengers_.clear();

	if (pLostException != NULL)
	{
		// The app layer has already been notified of player-vehicle-loss by
		// BWEntities::onEntityInconsistent from
		// ActiveEntities:clearEntitiesWithException:
		pPlayer_->setSpaceVehiclePositionAndDirection( spaceID, vehicleID, pos, dir );

		MF_ASSERT( pLostException == pPlayer_ );
		pendingPassengers_.addPassenger( pLostException.get(), vehicleID );
		MF_ASSERT( pendingPassengers_.findPassenger( entityID ) == pPlayer_ );
	}

	// Let the Entity and its extensions deal with the rolled-back properties.
	// TODO: Error handling.
	MF_VERIFY( pPlayer_->onRestoreProperties( data ) );
}


/**
 *	This method creates a Local Entity, i.e. a locally-controlled Entity that
 *	the BWConnection does not know about. It is up to the client application to
 *	make any calls that the server would normally make for this entity, if
 *	necessary, and will otherwise be treated as an Entity the server doesn't
 *	ever bother sending any updates for.
 *
 *	Local Entities may board server Entities or other local Entities, but
 *	server Entities may not board local Entities.
 *
 *	It will not leave AoI due to distance, but may leave if mounted on a
 *	vehicle that is not in AoI.
 *
 *	This method is roughly equivalent to onEntityCreate()
 *
 *	@param entityTypeID	The typeID of the new Local Entity
 *	@param spaceID		The spaceID in which the new Local Entity is present
 *	@param vehicleID	The vehicleID the new Local Entity is upon, or
 *						NULL_ENTITY_ID if not on a vehicle
 *	@param position		The new Local Entity's position relative to the
 *						vehicle if any, or the space if not.
 *	@param direction	The new Local Entity's yaw, pitch and roll relative to
 *						the vehicle if any, or the space if not.
 *	@param data			A BinaryIStream reference to a stream containing the
 *						initial values of the Cell-side properties in the
 *						format used by BWEntity::onProperties
 *
 *	@return				A pointer to the new BWEntity, or NULL if creation
 *						failed.
 */
BWEntity * BWEntities::createLocalEntity( EntityTypeID entityTypeID,
	SpaceID spaceID, EntityID vehicleID, const Position3D & position,
	const Direction3D & direction, BinaryIStream & data )
{
	// TODO: Wrap if someone creates a billion Local Entities, instead of
	// asserting.
	EntityID newID = s_nextLocalEntityID_++;
	MF_ASSERT( isLocalEntityID( newID ) );

	BWEntityPtr pNewEntity = entityFactory_.create( newID, entityTypeID,
		spaceID, data, &connection_ );

	if (!pNewEntity)
	{
		ERROR_MSG( "BWEntities::createLocalEntity: Failed for entity %d\n",
			newID );
		return NULL;
	}

	// Local entities are always locally-controlled
	pNewEntity->triggerOnChangeControl( /* isControlling */ true,
		/* isInitialising */ true );

	this->addEntity( pNewEntity.get(), spaceID, vehicleID, position,
		direction );

	return pNewEntity.get();
}


/**
 *	This static method checks if the give EntityID is for a Local Entity or not.
 *
 *	@param entityID	The EntityID to test for being a Local Entity ID.
 *
 *	@return			True if the EntityID is a LocalEntity ID, false otherwise.
 */
/* static */ bool BWEntities::isLocalEntity( EntityID entityID )
{
	return isLocalEntityID( entityID );
}


/**
 *	This method performs the periodic updates for each entity
 *
 *	@param gameTime 	The current game time.
 *	@param deltaTime 	The time delta since the last tick.
 */
void BWEntities::tick( double gameTime, double deltaTime )
{
	BW::list< BWEntity * > entitiesLostVehicles;

	appPendingEntities_.processEntityFilters( gameTime, entitiesLostVehicles );
	activeEntities_.processEntityFilters( gameTime, entitiesLostVehicles );

	PassengersVector vPassengers;

	for (BW::list< BWEntity * >::iterator iEntityLostVehicle =
		entitiesLostVehicles.begin();
		iEntityLostVehicle != entitiesLostVehicles.end();
		++iEntityLostVehicle )
	{
		BWEntityPtr pEntity = *iEntityLostVehicle;
		const EntityID entityID = pEntity->entityID();

		vPassengers.push_back( std::make_pair( pEntity,
			pEntity->vehicleID() ) );

		if (!activeEntities_.removeEntityFromWorld( entityID, &vPassengers ) )
		{
			MF_VERIFY( appPendingEntities_.eraseEntity( entityID ) );
		}
	}

	for (PassengersVector::iterator iPassenger = vPassengers.begin();
		iPassenger != vPassengers.end(); ++iPassenger )
	{
		BWEntity * pPassenger = iPassenger->first.get();
		EntityID vehicleID = iPassenger->second;

		MF_ASSERT( vehicleID == pPassenger->vehicleID() );

		pendingPassengers_.addPassenger( pPassenger, vehicleID );
	}
}


/**
 *	This method periodically sends state updates to the server
 *
 *	@param gameTime 	The current game time.
 */
void BWEntities::updateServer( double gameTime )
{
	activeEntities_.sendControlledEntitiesUpdates( gameTime );
}


/// Entity message interface

/**
 *	@see ServerMessageHandler::onEntityControl
 */
void BWEntities::handleEntityControl( EntityID entityID, bool isControlling )
{
	MF_ASSERT( !isLocalEntityID( entityID ) );

	BWEntityPtr pEntity = this->findAny( entityID );

	if (!pEntity)
	{
		ERROR_MSG( "BWEntities::handleEntityControl: "
				"No such entity %d. isControlling = %d\n", 
			entityID, isControlling );
		return;
	}

	bool shouldCallCallback = pEntity->isPlayer() || pEntity->isInWorld();

	pEntity->triggerOnChangeControl( isControlling, !shouldCallCallback );
}


/**
 *	@see ServerMessageHandler::onEntityMethod
 */
void BWEntities::handleEntityMethod( EntityID entityID, int methodID,
	BinaryIStream & data )
{
	BWEntityPtr pEntity = this->findAny( entityID );

	if (!pEntity)
	{
		ERROR_MSG( "BWEntities::handleEntityMethod: "
				"No such entity %d. methodID = %d\n", 
			entityID, methodID );
		data.finish();
		return;
	}

	bool shouldCallCallback = pEntity->isPlayer() || pEntity->isInWorld();

	if (shouldCallCallback)
	{
		pEntity->onMethod( methodID, data );
	}
	else
	{
		data.finish();
	}
}


/**
 *	@see ServerMessageHandler::onEntityProperties
 */
void BWEntities::handleEntityProperties( EntityID entityID,
	BinaryIStream & data )
{
	BWEntityPtr pEntity = this->findAny( entityID );

	if (!pEntity)
	{
		ERROR_MSG( "BWServerMessageHandler::handleEntityProperties: "
				"No such entity %d.\n", 
			entityID );
		data.finish();
		return;
	}

	bool shouldCallCallback = pEntity->isPlayer() || pEntity->isInWorld();

	pEntity->onProperties( data, !shouldCallCallback );
}


/**
 *	@see ServerMessageHandler::onEntityProperty
 */
void BWEntities::handleEntityProperty( EntityID entityID, int propertyID,
	BinaryIStream & data )
{
	BWEntityPtr pEntity = this->findAny( entityID );

	if (!pEntity)
	{
		ERROR_MSG( "BWEntities::handleEntityProperty: "
				"No such entity %d. propertyID = %d\n", 
			entityID, propertyID );
		data.finish();
		return;
	}

	bool shouldCallCallback = pEntity->isPlayer() || pEntity->isInWorld();

	pEntity->onProperty( propertyID, data, !shouldCallCallback );
}


/**
 *	@see ServerMessageHandler::onNestedEntityProperty
 */
void BWEntities::handleNestedEntityProperty( EntityID entityID,
	BinaryIStream & data, bool isSlice )
{
	BWEntityPtr pEntity = this->findAny( entityID );

	if (!pEntity)
	{
		ERROR_MSG( "BWEntities::handleNestedEntityProperty: "
				"No such entity %d. isSlice = %d\n", 
			entityID, isSlice );
		data.finish();
		return;
	}

	bool shouldCallCallback = pEntity->isPlayer() || pEntity->isInWorld();

	pEntity->onNestedProperty( data, isSlice, !shouldCallCallback );
}


/**
 *	@see ServerMessageHandler::getEntityMethodStreamSize
 */
int BWEntities::getEntityMethodStreamSize( EntityID entityID,
	int methodID, bool isFailAllowed ) const
{
	BWEntityPtr pEntity = this->findAny( entityID );
	MF_ASSERT( isFailAllowed || pEntity );

	return pEntity ? pEntity->getMethodStreamSize( methodID ) :
		Mercury::INVALID_STREAM_SIZE;
}


/**
 *	@see ServerMessageHandler::getEntityPropertyStreamSize
 */
int BWEntities::getEntityPropertyStreamSize( EntityID entityID,
	int propertyID, bool isFailAllowed ) const
{
	BWEntityPtr pEntity = this->findAny( entityID );
	MF_ASSERT( isFailAllowed || pEntity );

	return pEntity ? pEntity->getPropertyStreamSize( propertyID ) :
		Mercury::INVALID_STREAM_SIZE;
}


/**
 *	@see ServerMessageHandler::onEntityMoveWithError
 */
void BWEntities::handleEntityMoveWithError( const EntityID & entityID,
		const SpaceID & spaceID, const EntityID & vehicleID,
		const Position3D & position, const Vector3 & posError, 
		float yaw, float pitch, float roll, bool isVolatile )
{
	BWEntityPtr pEntity = this->findAny( entityID );

	if (!pEntity)
	{
		ERROR_MSG( "BWEntities::handleEntityMoveWithError: "
				"No such entity %d.\n", 
			entityID );
		return;
	}

	// This relies on entities that're in the pendingPassengers_ collection
	// not having filters, so we immediately see any vehicleID change.

	MF_ASSERT( pEntity->pFilter() == NULL ||
		pendingPassengers_.findPassenger( entityID ) == NULL );

	EntityID oldVehicleID = pEntity->vehicleID();

	// Client-controlled entities cannot receive volatile position updates.
	MF_ASSERT( !pEntity->isControlled() || !isVolatile );

	// If controlled, this is a forced position, i.e. a physics correction
	// If server-controlled, a non-volatile update is a teleport
	const bool isTeleport = pEntity->isControlled() || !isVolatile;

	pEntity->onMoveFromServer( connection_.lastMessageTime(), position,
		vehicleID, spaceID, Direction3D( Vector3( roll, pitch, yaw ) ),
		posError, isTeleport );
	
	if (pEntity->isControlled())
	{
		pEntity->setPhysicsCorrected( connection_.lastSentMessageTime() );
	}

	EntityID newVehicleID = pEntity->vehicleID();

	if (newVehicleID != oldVehicleID)
	{
		// Any other entities will be handled by BWEntities::tick()
		pendingPassengers_.changeVehicleForPassenger( entityID, newVehicleID);
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
void BWEntities::addListener( BWEntitiesListener * pListener )
{
	activeEntities_.addListener( pListener );
}


/**
 *	This method removes a registered listener to this object.
 *
 *	@param pListener 	The listner to remove.
 *
 *	@return 			true if the listener was removed, false if no such
 *						listener was registered.
 *
 *	@see addListener
 */
bool BWEntities::removeListener( BWEntitiesListener * pListener )
{
	return activeEntities_.removeListener( pListener );
}


/// InWorld Entity-list operations and accessors

/**
 *	This method return the entity associated with a given entity id.
 *
 *	Only entities that'd had onEnter called without the paired onLeave will
 *	show up in this search.
 *
 *	@param entityID 	The ID of the entity to find.
 * 
 *	@return 	The entity with the given ID, or NULL if no such entity exists.
 * 
 *	@see findAny
 */
BWEntity * BWEntities::find( EntityID entityID ) const
{
	return activeEntities_.findEntity( entityID );
}


/**
 *	This locates a BWEntity by EntityID in any of our entity lists
 *
 *	@param entityID 	The ID of the desired BWEntity
 *	@return	A BWEntity * to the BWEntity with the desired EntityID, or NULL
 *			if we know of no such BWEntity.
 */
BWEntity * BWEntities::findAny( EntityID entityID ) const
{
	if (pPlayer_ != NULL && pPlayer_->entityID() == entityID)
	{
		return pPlayer_.get();
	}

	BWEntity * pEntity = activeEntities_.findEntity( entityID );

	if (pEntity != NULL)
	{
		return pEntity;
	}

	pEntity = appPendingEntities_.findEntity( entityID );

	if (pEntity != NULL)
	{
		return pEntity;
	}

	pEntity = pendingPassengers_.findPassenger( entityID );

	if (pEntity != NULL)
	{
		return pEntity;
	}

	return NULL;
}

// ----------------------------------------------------------------------------
// Section: BWEntitiesWatcher
// ----------------------------------------------------------------------------


#if ENABLE_WATCHERS


/**
 *	This is a watcher for exposing a BWEntities instance to watchers.
 *
 *	This resembles the MapWatcher in how it's implemented, but allows for
 *	use against the BWEntities instance which doesn't quite look like a
 *	BW::map.
 */
class BWEntitiesWatcher : public Watcher
{
public:
	typedef BWEntities Map;
	typedef EntityID MAP_key;
	typedef BWEntityPtr MAP_value;


	/**
	 *	Constructor.
	 *
	 *	@param toWatch	The map to watch (or NULL if relying on a parent 
	 *					watcher).
	 *	@param pKeyStringConverter
	 *					The converter for converting keys into strings for 
	 *					presentation.
	 */
	BWEntitiesWatcher( const Map & toWatch = *(Map*)NULL, 
			IMapKeyStringConverter< MAP_key > * pKeyStringConverter = NULL ) :
		toWatch_( toWatch ),
		pKeyStringConverter_( pKeyStringConverter ),
		child_( NULL ),
		subBase_( 0 )
	{
	}


	/* Override from Watcher. */
	bool getAsString( const void * base, const char * path,
			BW::string & result, BW::string & desc, 
			Watcher::Mode & mode ) const /* override */
	{
		if (isEmptyPath(path))
		{
			result = "<DIR>";
			mode = WT_DIRECTORY;
			desc = comment_;
			return true;
		}

		try
		{
			MAP_value rChild = this->findChild( base, path );

			return child_->getAsString( (void*)(subBase_ + (uintptr)&rChild),
				this->tail( path ), result, desc, mode );
		}
		catch (NoSuchChild &)
		{
			return false;
		}
	}


	/* Override from Watcher. */
	bool setFromString( void * base, const char * path,
			const char * valueStr ) /* override */
	{
		return false;
	}


	/* Override from Watcher. */
	bool getAsStream( const void * base, const char * path,
			WatcherPathRequestV2 & pathRequest ) const /* override */
	{
		if (Watcher::isEmptyPath(path))
		{
			Watcher::Mode mode = WT_DIRECTORY;
			watcherValueToStream( pathRequest.getResultStream(), "<DIR>", mode );
			pathRequest.setResult( comment_, mode, this, base );
			return true;
		}
		else if (Watcher::isDocPath( path ))
		{
			Watcher::Mode mode = Watcher::WT_READ_ONLY;
			watcherValueToStream( pathRequest.getResultStream(), comment_, mode );
			pathRequest.setResult( comment_, mode, this, base );
			return true;
		}

		try
		{
			MAP_value rChild = this->findChild( base, path );

			return child_->getAsStream( (void*)(subBase_ + (uintptr)&rChild),
				this->tail( path ), pathRequest );
		}
		catch (NoSuchChild &)
		{
			return false;
		}
	}


	/* Override from Watcher. */
	bool setFromStream( void * base, const char * path,
		WatcherPathRequestV2 & pathRequest ) /* override */
	{
		return false;
	}


	/* Override from Watcher */
	bool visitChildren( const void * base, const char * path,
		WatcherPathRequest & pathRequest ) /* override */
	{
		if (isEmptyPath(path))
		{
			const Map & useMap = *(Map*)( ((uintptr)&toWatch_) +
				((uintptr)base) );

			// Notify the path request of the map element count
			MF_ASSERT( useMap.size() <= INT_MAX );
			pathRequest.addWatcherCount( ( int )useMap.size() );

			int count = 0;
			Map::const_iterator iter = useMap.begin();
			while (iter != useMap.end())
			{
				BW::string callLabel( this->keyToString( (*iter).first ) );

				MAP_value reference( (*iter).second );

				uintptr offset = uintptr( &reference );

				if (!pathRequest.addWatcherPath(
						(void*)(subBase_ + offset ),
						this->tail( path ), callLabel, *child_ ))
				{
					break;
				}

				iter++;
				count++;
			}

			return true;
		}
		else
		{
			try
			{
				MAP_value rChild = this->findChild( base, path );

				return child_->visitChildren(
					(void*)(subBase_ + (uintptr)&rChild),
					this->tail( path ), pathRequest );
			}
			catch (NoSuchChild &)
			{
				return false;
			}
		}
	}


	/* Override from Watcher. */
	bool addChild( const char * path, WatcherPtr pChild,
			void * withBase = NULL ) /* override */
	{
		if (isEmptyPath( path ))
		{
			return false;
		}
		else if (strchr(path,'/') == NULL)
		{
			// we're a one-child family here
			if (child_ != NULL) return false;

			// ok, add away
			child_ = pChild;
			subBase_ = (uintptr)withBase;
			return true;
		}
		else
		{
			// we don't/can't check that our component of 'path'
			// exists here either.
			return child_->addChild( this->tail ( path ),
				pChild, withBase );
		}
	}


	/**
	 *	This function is a helper. It strips the path from the input string and
	 *	returns a string containing the tail value.
	 *
	 *	@see DirectoryWatcher::tail
	 */
	static const char * tail( const char * path )
	{
		return DirectoryWatcher::tail( path );
	}

private:
	const BW::string keyToString( const MAP_key & key ) const
	{
		if (pKeyStringConverter_)
		{
			return pKeyStringConverter_->valueToString( key );
		}
		else
		{
			return watcherValueToString( key );
		}
	}


	bool stringToKey( const BW::string & string, MAP_key & out ) const
	{
		if (pKeyStringConverter_)
		{
			return pKeyStringConverter_->stringToValue( string, out );
		}
		else
		{
			return watcherStringToValue( string.c_str(), out );
		}
	}


	class NoSuchChild
	{
	public:
		NoSuchChild( const char * path ): path_( path ) {}

		const char * path_;
	};

	MAP_value findChild( const void * base, const char * path ) const
	{
		const Map & useMap = *(Map*)( ((uintptr)&toWatch_) +
			((uintptr)base) );

		// find out what we're looking for
		char * pSep = strchr( (char*)path, WATCHER_SEPARATOR );
		BW::string lookingFor = pSep ?
			BW::string( path, pSep - path ) : BW::string ( path );

		MAP_key key;

		if (this->stringToKey( lookingFor, key ))
		{
			// see if that's in there anywhere
			MAP_value value = useMap.find( key );
			if (value != NULL)
			{
				return value;
			}
		}

		// don't have it, give up then
		throw NoSuchChild( path );
	}

	const Map &	toWatch_;
	IMapKeyStringConverter< MAP_key > * pKeyStringConverter_;
	WatcherPtr	child_;
	uintptr		subBase_;
};


WatcherPtr BWEntities::pWatcher()
{
	return new BWEntitiesWatcher();
}

#endif // ENABLE_WATCHERS

BW_END_NAMESPACE

// bw_entities.cpp
