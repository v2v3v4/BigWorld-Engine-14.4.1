#include "pch.hpp"

#include "bw_entity.hpp"

#include "bw_connection.hpp"
#include "entity_extension.hpp"

#include "connection/movement_filter.hpp"
#include "cstdmf/binary_stream.hpp"
#include "math/matrix.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
BWEntity::BWEntity( BWConnection * pBWConnection ):
		SafeReferenceCount(),
		entityExtensions_(),
		entityID_( NULL_ENTITY_ID ),
		entityTypeID_( INVALID_ENTITY_TYPE_ID ),
		spaceID_( NULL_SPACE_ID ),
		vehicleID_( NULL_ENTITY_ID ),
		pVehicle_( NULL ),
		pBWConnection_( pBWConnection ),
		isPlayer_( false ),
		pFilter_( NULL ),
		localPosition_( 0.f, 0.f, 0.f ),
		localDirection_(),
		isInAoI_( false ),
		isInWorld_( false ),
		isDestroyed_( false ),
		isControlled_( false ),
		isReceivingVolatileUpdates_( false ),
		physicsCorrectionAckTime_( 0. ),
		latestLocalMoveTime_( 0. ),
		latestLocalMoveIs2D_( false )
{
}


/**
 *	Destructor.
 */
BWEntity::~BWEntity()
{
	MF_ASSERT( isDestroyed_ );
	MF_ASSERT( pFilter_ == NULL );
}


/**
 *	This method creates the entity extensions associated with this entity.
 */
void BWEntity::createExtensions( EntityExtensionFactoryManager & factories )
{
	entityExtensions_.init( *this, factories );
}


/**
 *	This method is called to initialise this entity.
 *
 *	@param entityID 		The ID of the entity.
 *	@param entityTypeID 	The entity Type ID of this entity.	
 *	@param spaceID			The ID of the space this entity's cell entity
 *							resides in, or NULL_SPACE_ID if this is a base
 *							entity.
 *	@param data 			The property data stream.
 */
bool BWEntity::init( EntityID entityID, EntityTypeID entityTypeID,
		SpaceID spaceID, BinaryIStream & data )
{
	entityID_ = entityID;
	entityTypeID_ = entityTypeID;
	spaceID_ = spaceID;

	bool isOkay = (spaceID == NULL_SPACE_ID) ?
		this->initBasePlayerFromStream( data ) :
		this->initCellEntityFromStream( data );

	return isOkay;
}


/**
 *	This method is called to restore this entity after a Cell failure
 *
 *	@param data 			A stream containing the current properties from the
 *							Base, then the Cell, as visible to the client.
 */
bool BWEntity::onRestoreProperties( BinaryIStream & data )
{
	return this->restorePlayerFromStream( data );
}


/**
 *	This method initialises this entity's properties from a batch property
 *	update.
 *
 *	The default implementation passes the update to BWEntity::onProperties
 *
 *	@param stream 	The property data stream.
 *
 *	@see BWEntity::onProperties
 */
bool BWEntity::initCellEntityFromStream( BinaryIStream & stream )
{
	this->onProperties( stream, /* isInitialising */ true );
	return !stream.error();
}


/**
 *	This method is called when the server sends a batch of property changes.
 *
 *	This is usually at initialisation, or when there is a change in detail
 *	levels and a batch of property changes are sent as a result.
 *
 *	@param data				The property data stream.
 *	@param isInitialising	Whether these properties are meant to be for
 *							initialising the entity, or for an entity that has
 *							already been initialised. Typically used to decide
 *							whether or not to call a set_ callback.
 */
void BWEntity::onProperties( BinaryIStream & data, bool isInitialising )
{
	if (!data.remainingLength())
	{
		return;
	}

	uint8 size;
	data >> size;

	for (uint8 i = 0; i < size; i++)
	{
		uint8 index;
		data >> index;
		this->onProperty( index, data, isInitialising );
	}
}


/**
 *	This method indicates if this entity is a local entity, rather than being
 *	a server-owned entity.
 */
bool BWEntity::isLocalEntity() const
{
	return BWEntities::isLocalEntity( entityID_ );
}


/**
 *	This method tests whether we're currently physics-corrected, which means
 *	we will reject local movement data.
 */
bool BWEntity::isPhysicsCorrected() const
{
	if (isZero( physicsCorrectionAckTime_ ))
	{
		return false;
	}

	// We are physics-corrected until after we have sent a packet containing
	// the physics-correction acknowledgement.
	// physicsCorrectionAckTime_ is the timestamp of the packet we sent before
	// the physics correction arrived.
	if (isEqual( physicsCorrectionAckTime_,
		pBWConnection_->lastSentMessageTime() ))
	{
		return true;
	}

	IF_NOT_MF_ASSERT_DEV( physicsCorrectionAckTime_ <
		pBWConnection_->lastSentMessageTime())
	{
		ERROR_MSG( "BWEntity::isPhysicsCorrected: Physics corrected after "
				"packet sent at %g, but most recent packet sent at %g\n",
			physicsCorrectionAckTime_, pBWConnection_->lastSentMessageTime() );
		// We've somehow gone backwards in time... Stay physics-corrected until
		// we catch up with reality again.
		return true;
	}

	// This is mutable, to avoid the lastSentMessageTime() lookup every time
	// something calls this.
	physicsCorrectionAckTime_ = 0.;

	return false;
}


/**
 *	This method sets the physics correction state for this entity.
 *
 *	@param The time of the last server message send.
 */
void BWEntity::setPhysicsCorrected( double time )
{
	physicsCorrectionAckTime_ = time;
}


/** 
 *	This method sets the filter for this entity.
 *
 *	After this, this entity owns the filter, and will delete it when it is
 *	done with it.
 * 
 *	This method cannot be called on an Entity that has not yet entered, or
 *	has already left, AoI.
 * 
 *	@param pFilter 	The filter to set, or NULL to clear the filter.
 */
void BWEntity::pFilter( MovementFilter * pNewFilter )
{
	// Early-out if we're not changing the filter
	if (pNewFilter == pFilter_)
	{
		return;
	}

	// Early-out if we're removing the filter.
	if (pNewFilter == NULL)
	{
		// TODO: Should we output the top of the filter at this point?
		bw_safe_delete( pFilter_ );
		return;
	}

	// We cannot set a filter if the entity is not in AoI
	IF_NOT_MF_ASSERT_DEV( isInAoI_ )
	{
		ERROR_MSG( "BWEntity::pFilter: Ignoring attempt to set a filter on "
			"entity %d which is not in our AoI\n", this->entityID() );
		return;
	}

	if (pFilter_ != NULL)
	{
		pNewFilter->copyState( *pFilter_ );
		bw_safe_delete( pFilter_ );
	}
	else
	{
		double time = this->pBWConnection()->clientTime();

		pNewFilter->reset( time );
		pNewFilter->input( time, spaceID_, vehicleID_, localPosition_,
			Vector3::zero(), localDirection_ );
	}

	pFilter_ = pNewFilter;
}


/**
 *	This method applies any attached filter at the given time to the
 *	entity's position/direction. It returns false if the position or
 *	direction has become invalid. (e.g., attempted to board a non-existent
 *	vehicle)
 *
 *	@param gameTime The gameTime at which to apply the filter
 *
 *	@return true unless the entity's state has become invalid
 */
bool BWEntity::applyFilter( double gameTime )
{
	if (pFilter_ != NULL)
	{
		pFilter_->output( gameTime, *this );
	}

	return (vehicleID_ == NULL_ENTITY_ID || pVehicle_ != NULL);
}


/**
 *	This method is used to update the cell properties of a player whose base
 *	has already been created.
 *
 *	@param data 	The data stream containing the cell property data.
 */
bool BWEntity::updateCellPlayerProperties( BinaryIStream & data )
{
	// Workaround for bug 32285
	isControlled_ = true;
	latestLocalMoveTime_ = 0.;

	return this->initCellPlayerFromStream( data );
}


/**
 *	This method calculates the position of the entity in world coordinates.
 */
const Position3D BWEntity::position() const
{
	if (pVehicle_ == NULL)
	{
		return localPosition_;
	}
	else
	{
		Matrix m;
		const Direction3D vehicleDirection = pVehicle_->direction();

		m.setRotate( vehicleDirection.yaw, vehicleDirection.pitch, vehicleDirection.roll );

		return m.applyVector( localPosition_ ) + pVehicle_->position();
	}
}


/**
 *	This method calculates the direction of the entity in world coordinates.
 */
const Direction3D BWEntity::direction() const
{
	if (pVehicle_ == NULL)
	{
		return localDirection_;
	}
	else
	{
		Direction3D out = localDirection_;
		const Direction3D vehicleDirection = pVehicle_->direction();

		out.yaw += vehicleDirection.yaw;

		// TODO: These are commented out in BWClient's
		// transformVehicleToCommon() method. Not sure why yet.

		// out.pitch += vehicleDirection.pitch;
		// out.roll += vehicleDirection.roll;

		return out;
	}
}


/**
 *	This method is an override from MovementFilterTarget
 */
void BWEntity::setSpaceVehiclePositionAndDirection( const SpaceID & spaceID,
		const EntityID & vehicleID, const Position3D & position, 
		const Direction3D & direction )
{
	// Only the player Entity can actually see a space change while in-world.
	MF_ASSERT( spaceID == spaceID_ || this->isPlayer() );

	// We don't count the player getting a cell-entity as a space change for
	// callback purposes.
	bool isSpaceChange = spaceID_ != spaceID && spaceID_ != NULL_SPACE_ID;

	// An entity cannot leave a space via this method.
	MF_ASSERT( spaceID != NULL_SPACE_ID );

	spaceID_ = spaceID;

	if (vehicleID != vehicleID_)
	{
		this->onChangeVehicleID( vehicleID );
		MF_ASSERT( vehicleID == vehicleID_ );
	}

	localPosition_ = position;
	localDirection_ = direction;

	if (isSpaceChange)
	{
		// This implies either a MovementFilter bug, or the server state is bad.
		MF_ASSERT( this->pVehicle() == NULL ||
			this->spaceID() == this->pVehicle()->spaceID() );

		this->triggerOnChangeSpace();
	}

	if (this->isInWorld())
	{
		this->onPositionUpdated();
	}
}


/**
 *	This method transforms the given position and direction from world
 *	coordinates to the coordinate system relative to this entity. 
 *
 *	@param position 	The position to convert in world coordinates.
 *	@param direction 	The direction to convert in world coordinates.
 */
void BWEntity::transformCommonToVehicle( Position3D & position, 
	Direction3D & direction ) const
{
	Matrix m;
	const Direction3D vehicleDirection = this->direction();

	m.setRotateInverse( vehicleDirection.yaw, vehicleDirection.pitch, 
		vehicleDirection.roll );

	position = m.applyVector( position - this->position() );

	direction.yaw -= vehicleDirection.yaw;

	// TODO: These were commented out in BWClient's
	// transformCommonToVehicle() method.
	// direction.pitch -= vehicleDirection.pitch;
	// direction.roll -= vehicleDirection.roll;
}


/**
 *	This method transforms the given position and direction from the coordinate
 *	system relative to this entity to world coordinates.
 *
 *	@param position 	The position in entity-space coordinates to convert.
 *	@param direction 	The direction in entity-space coordinates to convert.
 */
void BWEntity::transformVehicleToCommon( Position3D & position, 
	Direction3D & direction ) const
{
	Matrix m;
	const Direction3D vehicleDirection = this->direction();

	m.setRotate( vehicleDirection.yaw, vehicleDirection.pitch,
		vehicleDirection.roll );

	position = m.applyVector( position ) + this->position();

	direction.yaw += vehicleDirection.yaw;

	// TODO: These were commented out in BWClient's
	// transformVehicleToCommon() method.
	// direction.pitch += vehicleDirection.pitch;
	// direction.roll += vehicleDirection.roll;
}


/**
 *	This method tells the entity about a position/direction update to either
 *	apply or pass to the active filter.
 *
 *	@param time	The client time of the update
 *	@param position The position of the update
 *	@param vehicleID The vehicleID of the position, or EntityID( -1 ) to not
 *		change vehicles
 *	@param spaceID The spaceID of the position, or NULL_SPACE_ID to not change
 *		spaces
 *	@param direction The direction of the update. Any
 *		ServerConnection::NO_DIRECTION component will be treated as unchanged
 *	@param positionError The position error of the update, if any.
 *	@param isTeleport Whether this update is a teleport (non-volatile update)
 *
 *	@see BWEntity::onMoveLocally
 *	@see BWEntity::onMoveInternal
 */
void BWEntity::onMoveFromServer( double time, const Position3D & position,
	EntityID vehicleID,	SpaceID spaceID, const Direction3D & direction,
	const Vector3 & positionError, bool isTeleport )
{
	Position3D defaultPosition = position;
	Direction3D defaultDirection = direction;
	Vector3 defaultPositionError = positionError;

	if (isReceivingVolatileUpdates_ != !isTeleport)
	{
		isReceivingVolatileUpdates_ = !isTeleport;

		// Locally-controlled entities trigger this callback in
		// BWEntity::triggerOnChangeControl, when they are granted control,
		// and locally controlled entities can only receive non-volatile
		// updates.
		MF_ASSERT( !this->isControlled() );
		this->triggerOnChangeReceivingVolatileUpdates();
	}

	this->populateMoveDefaults( defaultPosition, vehicleID, spaceID,
		defaultDirection, defaultPositionError );

	this->onMoveInternal( time, defaultPosition, vehicleID, spaceID,
		defaultDirection, defaultPositionError, /* isReset */ isTeleport );

	latestLocalMoveTime_ = 0.;
}


/**
 *	This method retrieves the latest values give to onMoveFromServer or
 *	onMoveLocally
 *
 *	This lets you look into the future of a historical filter, for example.
 *
 *	@param position The position of the most recent update
 *	@param vehicleID The vehicleID of the position
 *	@param spaceID The spaceID of the position
 *	@param direction The direction of the most recent update
 *	@param positionError The position error of the most recent update
 */
void BWEntity::getLatestMove( Position3D & position, EntityID & vehicleID,
	SpaceID & spaceID, Direction3D & direction, Vector3 & positionError ) const
{
	if (pFilter_ != NULL)
	{
		double dummyTime;

		pFilter_->getLastInput( dummyTime, spaceID, vehicleID,
			position, positionError, direction );
	}
	else
	{
		position = localPosition_;
		vehicleID = vehicleID_;
		spaceID = spaceID_;
		positionError = Vector3::zero();
		direction = localDirection_;
	}
}


/**
 *	This method creates a locally-provided position update for
 *	locally-controlled entities. Such moves will be sent to the server for
 *	distribution to other clients.
 *
 *	@param time	The client time of this update
 *	@param position The position of this update
 *	@param vehicleID The vehicleID of the position of this update
 *	@param is2DPosition Whether the y-value of the Entity's position is
 *						interesting to other clients or not.
 *	@param direction The facing direction of this update
 */
void BWEntity::onMoveLocally( double time, const Position3D & position,
	EntityID vehicleID, bool is2DPosition, const Direction3D & direction )
{
	if (!this->isControlled())
	{
		ERROR_MSG( "BWEntity::onMoveLocally: Attempting to provide local "
			"movement for server-controlled Entity %d\n", this->entityID() );
		return;
	}

	if (this->isPhysicsCorrected())
	{
		ERROR_MSG( "BWEntity::onMoveLocally: Attempting to provide local "
				"movement for an entity under physics correction Entity %d\n",
			this->entityID() );
		return;
	}

	// TODO: Should this be a reset?
	this->onMoveInternal( time, position, vehicleID, spaceID_, direction,
		Vector3::zero(), /* isReset */ true );

	latestLocalMoveTime_ = time;
	latestLocalMoveIs2D_ = is2DPosition;
}


/**
 *	This method sends the latests locally-created move to the server.
 */
void BWEntity::sendLatestLocalMove() const
{
	MF_ASSERT( !this->isLocalEntity() );

	if (!this->isControlled())
	{
		ERROR_MSG( "BWEntity::sendLatestLocalMove: Attempting to send local "
			"movement for server-controlled Entity %d\n", this->entityID() );
		return;
	}

	if (isZero( latestLocalMoveTime_ ))
	{
		// We haven't generated a local move since we gained local control
		// or received a physics correction from the server.
		return;
	}

	// It should not be possible to have a non-zero latestLocalMoveTime_,
	// but be under a physics correction.
	MF_ASSERT( !this->isPhysicsCorrected() );

	Position3D localPosition;
	EntityID vehicleID;
	SpaceID spaceID;
	Vector3 dummyPositionError;
	Direction3D localDirection;

	this->getLatestMove( localPosition, vehicleID, spaceID, localDirection,
		dummyPositionError );

	// We cannot send space-changes.
	MF_ASSERT( spaceID == spaceID_ );

	pBWConnection_->addLocalMove( entityID_, spaceID, vehicleID, localPosition,
		localDirection, latestLocalMoveIs2D_, this->position() );
}



/**
 *
 */
void BWEntity::triggerOnBecomePlayer()
{
	MF_ASSERT( !isPlayer_ );
	isPlayer_ = true;

	this->onBecomePlayer();

	EntityExtensions::iterator iter = entityExtensions_.begin();

	while (iter != entityExtensions_.end())
	{
		(*iter)->triggerOnBecomePlayer();

		++iter;
	}
}


/**
 *
 */
void BWEntity::triggerOnEnterAoI( const EntityEntryBlocker & rBlocker )
{
	MF_ASSERT( pFilter_ == NULL );

	MF_ASSERT( pVehicle_ == NULL );

	if (vehicleID_ != NULL_ENTITY_ID)
	{
		const BWEntities & entities = pBWConnection_->entities();
		pVehicle_ = entities.find( vehicleID_ );
		MF_ASSERT( pVehicle_ != NULL );

		// Server entities may not ride local vehicles
		MF_ASSERT( this->isLocalEntity() || !pVehicle_->isLocalEntity() );
	}

	isInAoI_ = true;

	this->onEnterAoI( rBlocker );
	EntityExtensions::iterator iter = entityExtensions_.begin();

	while (iter != entityExtensions_.end())
	{
		(*iter)->triggerOnEnterAoI( rBlocker );

		++iter;
	}
}


/**
 *
 */
void BWEntity::triggerOnEnterWorld()
{
	if (pVehicle_ != NULL)
	{
		pVehicle_->onPassengerBoard( this );
	}

	isInWorld_ = true;

	this->onEnterWorld();

	EntityExtensions::iterator iter = entityExtensions_.begin();

	while (iter != entityExtensions_.end())
	{
		(*iter)->triggerOnEnterWorld();

		++iter;
	}
}


/**
 *
 */
void BWEntity::triggerOnChangeSpace()
{
	MF_ASSERT( this->isPlayer() );

	this->onChangeSpace();

	EntityExtensions::iterator iter = entityExtensions_.begin();

	while (iter != entityExtensions_.end())
	{
		(*iter)->triggerOnChangeSpace();

		++iter;
	}
}


/**
 *
 */
void BWEntity::triggerOnLeaveWorld()
{
	MF_ASSERT( passengers_.empty() );

	this->onLeaveWorld();

	EntityExtensions::iterator iter = entityExtensions_.begin();

	while (iter != entityExtensions_.end())
	{
		(*iter)->triggerOnLeaveWorld();

		++iter;
	}

	isInWorld_ = false;

	if (pVehicle_ != NULL)
	{
		pVehicle_->onPassengerAlight( this );
	}
}


/**
 *
 */
void BWEntity::triggerOnLeaveAoI()
{
	this->onLeaveAoI();

	EntityExtensions::iterator iter = entityExtensions_.begin();

	while (iter != entityExtensions_.end())
	{
		(*iter)->triggerOnLeaveAoI();

		++iter;
	}

	isInAoI_ = false;

	this->pFilter( NULL );

	pVehicle_ = NULL;
}


/**
 *
 */
void BWEntity::triggerOnBecomeNonPlayer()
{
	MF_ASSERT( isPlayer_ );
	isPlayer_ = false;
	this->onBecomeNonPlayer();

	EntityExtensions::iterator iter = entityExtensions_.begin();

	while (iter != entityExtensions_.end())
	{
		(*iter)->triggerOnBecomeNonPlayer();

		++iter;
	}
}


/**
 *
 */
void BWEntity::destroyNonPlayer()
{
	MF_ASSERT( !isDestroyed_ );

	// Players are immune from destruction while still the Player
	// This makes callers for this method much simpler
	if (isPlayer_)
		return;

	isDestroyed_ = true;
	this->onDestroyed();

	// This will trigger EntityExtension::onEntityDestroyed(), which is all
	// the warning they get.
	entityExtensions_.clear();
}


/**
 *
 */
void BWEntity::triggerOnChangeControl( bool isControlling,
	bool isInitialising )
{
	MF_ASSERT( isControlled_ != isControlling );

	bool didVolatileChange = (isReceivingVolatileUpdates_ != false);

	isControlled_ = isControlling;
	isReceivingVolatileUpdates_ = false;
	latestLocalMoveTime_ = 0.;

	this->onChangeControl( isControlling, isInitialising );

	EntityExtensions::iterator iter = entityExtensions_.begin();

	while (iter != entityExtensions_.end())
	{
		(*iter)->triggerOnChangeControl( isControlling, isInitialising );

		++iter;
	}

	if (didVolatileChange)
	{
		MF_ASSERT( !isInitialising );
		MF_ASSERT( isControlled_ );
		this->triggerOnChangeReceivingVolatileUpdates();
	}
}


/**
 *	This method is called when this entity receives a volatile update when
 *	it has been receiving non-volatile updates or vice-versa.
 *
 *	If the change is due to the entity becoming server-controlled, then
 *	onChangeControl will be called first. In both calls,
 *	BWEntity::isReceivingVolatileUpdates() will be correct.
 *
 *	If the change is due to a position update, the call will be made before
 *	the position update is given to the filter, to support replacing a
 *	non-historical filter with a historical filter without losing history.
 */
void BWEntity::triggerOnChangeReceivingVolatileUpdates()
{
	this->onChangeReceivingVolatileUpdates();

	EntityExtensions::iterator iter = entityExtensions_.begin();

	while (iter != entityExtensions_.end())
	{
		(*iter)->triggerOnChangeReceivingVolatileUpdates();

		++iter;
	}
}


/**
 *
 */
void BWEntity::onPassengerBoard( BWEntity * pEntity )
{
	passengers_.push_back( pEntity );
}


/**
 *
 */
void BWEntity::onPassengerAlight( BWEntity * pEntity )
{
	passengers_.remove( pEntity );
}


/**
 *	This method tells the entity about a position/direction update to either
 *	apply or pass to the active filter. It optionally resets any historical
 *	data.
 *
 *	@param time	The client time of the update
 *	@param position The position of the update
 *	@param vehicleID The vehicleID of the position
 *	@param spaceID The spaceID of the position
 *	@param direction The direction of the update
 *	@param positionError The position error of the update.
 *	@param isReset Whether this update should reset any historical data
 *
 *	@see BWEntity::onMoveFromServer
 *	@see BWEntity::onMoveLocally
 */
void BWEntity::onMoveInternal( double time, const Position3D & position,
	EntityID vehicleID,	SpaceID spaceID, const Direction3D & direction,
	const Vector3 & positionError, bool isReset )
{
	if (this->pFilter_ != NULL)
	{
		if (isReset)
		{
			pFilter_->reset( time );
		}

		pFilter_->input( time, spaceID, vehicleID, position, positionError,
			direction );

		if (isReset)
		{
			this->applyFilter( time );
		}
	}
	else
	{
		this->setSpaceVehiclePositionAndDirection( spaceID, vehicleID,
			position, direction );
	}
}


/**
 *	This is an internal method to fill in the defaults needed by
 *	onMoveFromServer
 */
void BWEntity::populateMoveDefaults( Position3D & position,
	EntityID & vehicleID, SpaceID & spaceID, Direction3D & direction,
	Vector3 & positionError )
{
	// TODO: Is this method correct? Do we want getLatestMove or do we always
	// want to default to where the model is historically-now?

	Position3D oldPosition;
	EntityID oldVehicleID;
	SpaceID oldSpaceID;
	Vector3 oldPositionError;
	Direction3D oldDirection;

	this->getLatestMove( oldPosition, oldVehicleID, oldSpaceID, oldDirection,
		oldPositionError );

	// See AVATAR_UPDATE_GET_POS_NoPos
	// Note that we _don't_ catch AVATAR_UPDATE_GET_POS_OnGround
	// as that needs to be passed through to the filter for handling
	if (position == Position3D( ServerConnection::NO_POSITION,
		ServerConnection::NO_POSITION, ServerConnection::NO_POSITION ))
	{
		position = oldPosition;
		positionError = oldPositionError;
	}

	if (vehicleID == EntityID(-1))
	{
		vehicleID = oldVehicleID;
	}

	if (spaceID == NULL_SPACE_ID)
	{
		spaceID = oldSpaceID;
	}

	// Doing direction components individually...
	// See AVATAR_UPDATE_GET_DIR_Yaw, AVATAR_UPDATE_GET_DIR_YawPitch
	// and AVATAR_UPDATE_GET_DIR_YawPitchRoll
	if (isEqual( direction.yaw, ServerConnection::NO_DIRECTION ))
	{
		direction.yaw = oldDirection.yaw;
	}

	if (isEqual( direction.pitch, ServerConnection::NO_DIRECTION ))
	{
		direction.pitch = oldDirection.pitch;
	}

	if (isEqual( direction.roll, ServerConnection::NO_DIRECTION ))
	{
		direction.roll = oldDirection.roll;
	}
}


/**
 *
 */
void BWEntity::onChangeVehicleID( EntityID newVehicleID )
{
	MF_ASSERT( newVehicleID != vehicleID_ );

	vehicleID_ = newVehicleID;

	if (!this->isInAoI())
	{
		MF_ASSERT( pVehicle_ == NULL );
		return;
	}

	if (pVehicle_ != NULL)
	{
		if (this->isInWorld())
		{
			pVehicle_->onPassengerAlight( this );
		}
		pVehicle_ = NULL;
	}

	if (vehicleID_ != NULL_ENTITY_ID)
	{
		const BWEntities & entities = pBWConnection_->entities();
		pVehicle_ = entities.find( vehicleID_ );
		// If pVehicle_ is NULL here, we've become inconsistent.
		if (pVehicle_ == NULL)
		{
			// We can't do anything about it though, as we could be being called
			// either from MovementFilter::output or BWEntity::onMoveExplicit,
			// and the former is likely to be iterating our container
			// TODO: Find a way to raise this as an event and deal with it when
			// safe to do so.
			return;
		}

		// Server entities may not ride local vehicles
		MF_ASSERT( this->isLocalEntity() || !pVehicle_->isLocalEntity() );

		// Only Entities in-world can be passengers.
		if (this->isInWorld())
		{
			pVehicle_->onPassengerBoard( this );
		}
	}
}


/**
 *	This method returns true if this entity is considered a witness by our
 *	connection.
 */
bool BWEntity::isWitness() const
{
	return pBWConnection_->isWitness( this->entityID() );
}


BW_END_NAMESPACE

// bw_entity.cpp

