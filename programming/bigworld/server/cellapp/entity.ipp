#ifdef CODE_INLINE
#define INLINE	inline
#else
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: Entity
// -----------------------------------------------------------------------------

/**
 *	This method increments the reference count of this entity. It should be
 *	called each time a reference to this entity is kept.
 *
 *	@see decRef
 */
INLINE
void Entity::incRef() const
{
	Py_INCREF(const_cast<Entity *>(this));
}


/**
 *	This method decrements the reference count of this entity. It should be
 *	called each time a reference to this entity is lost.
 *
 *	@see incRef
 */
INLINE
void Entity::decRef() const
{
	Py_DECREF(const_cast<Entity *>(this));
}


/**
 *	This method returns the ID associated with this entity. Each entity in the
 *	world has a unique ID.
 */
INLINE
EntityID Entity::id() const
{
	return id_;
}


/**
 *	This method sets whether this entity should return its ID to the ID pool.
 * 	Cell-only entities should do this. Cell entities with a base should let
 * 	the base return the ID.
 */
INLINE
void Entity::setShouldReturnID( bool shouldReturnID )
{
	shouldReturnID_ = shouldReturnID;
}


/**
 *	This method returns the position of this entity.
 */
INLINE
const Position3D & Entity::position() const
{
	return globalPosition_;
}


/**
 *	This method returns the direction this entity is currently facing.
 */
INLINE
const Direction3D & Entity::direction() const
{
	return globalDirection_;
}


/**
 *	This method returns the range list node of this entity.
 */
INLINE
EntityRangeListNode * Entity::pRangeListNode() const
{
	return pRangeListNode_;
}


/**
 *	This method return the volatile info of this entity. The volatile info
 *	indicates what type of data should be sent regularly to those interested in
 *	this entity. For example, if this entity moves regularly and changes
 *	direction regularly, its volatile info would be VOLATILE_AVATAR.
 */
INLINE
const VolatileInfo & Entity::volatileInfo() const
{
	return volatileInfo_;
}


/**
 *	This method returns whether or not this entity is real. An entity may be
 *	either <i>real</i> or <i>ghost</i>. For an entity, there is only one <i>real
 *	</i> and zero to many <i>ghosts</i>. A <i>real</i> entity is the
 *	authoritative instance and <i>ghosts</i> are a copy of this.
 */
INLINE
bool Entity::isReal() const
{
	// You shouldn't be able to have a real and a real channel at the same time.
	MF_ASSERT( !(pReal_ && pRealChannel_) );

	return pReal_ != NULL;
}


/**
 *	This method returns the real entity (if any) associated with this entity.
 *	For <i>ghost</i> entities, this will be NULL.
 */
INLINE
RealEntity * Entity::pReal() const
{
	return pReal_;
}


/**
 *	This method returns the space that this entity resides in.
 */
INLINE
Space & Entity::space()
{
	MF_ASSERT( pSpace_ != NULL );
	return *pSpace_;
}


/**
 *	This method returns the space that this entity resides in.
 */
INLINE
const Space & Entity::space() const
{
	MF_ASSERT( pSpace_ != NULL );
	return *pSpace_;
}


/**
 *	This method returns the event history associated with this entity. The event
 *	history stores the events that have recently occurred to this entity that
 *	other onlookers may be interested in.
 */
INLINE
EventHistory & Entity::eventHistory()
{
	return eventHistory_;
}


/**
 *	This method returns the event history associated with this entity. The event
 *	history stores the events that have recently occurred to this entity that
 *	other onlookers may be interested in.
 */
INLINE
const EventHistory & Entity::eventHistory() const
{
	return eventHistory_;
}


/**
 *	This method returns whether or not this entity has been destroyed. If an
 *	entity is destroyed, it is no longer a real or ghost entity.
 *
 *	The reason that we cannot delete this entity straight away is that there
 *	could be reference to it from things such as Area of Interest lists of other
 *	entities. To solve this, entities are reference counted.
 */
INLINE
bool Entity::isDestroyed() const
{
	return isDestroyed_;
}


/**
 *	This method returns the ID of entity type associated with this entity.
 */
INLINE
EntityTypeID Entity::entityTypeID() const
{
	return pEntityType_->typeID();
}


/**
 *	This method returns the client type ID associated with this entity. Often,
 *	this is the same as the entity type ID but may not be so. For example, an
 *	entity of type NPC on the server may be of type Avatar on the client.
 *
 *	@see entityTypeID
 */
INLINE
EntityTypeID Entity::clientTypeID() const
{
	return pType()->clientTypeID();
}


/**
 *	This method returns the object that represents the type of this entity.
 */
INLINE
EntityTypePtr Entity::pType() const
{
	return pEntityType_;
}


/**
 *	This method is used in RealEntityWithWitnesses constructor. It is used to
 *	quick check those entities that are currently in an entity's Area of
 *	Interest so that it can construct the unseen list easily.
 */
INLINE
bool Entity::isInAoIOffload() const
{
	return isInAoIOffload_;
}


/**
 *	This method is used in RealEntityWithWitnesses constructor. It is used to
 *	quick mark those entities that are currently in an entity's Area of Interest
 *	so that it can construct the unseen list easily.
 */
INLINE
void Entity::isInAoIOffload( bool isInAoIOffload )
{
	isInAoIOffload_ = isInAoIOffload;
}


/**
 *	This method sets the local position and direction of this entity _without_
 *	updating the global position and direction.
 */
INLINE
void Entity::setLocalPositionAndDirection( const Position3D & localPosition,
			const Direction3D & localDirection )
{
	localPosition_ = localPosition;
	localDirection_ = localDirection;
}


/**
 *	This method sets the global position and direction of this entity _without_
 *	updating the local position and direction.
 */
INLINE
void Entity::setGlobalPositionAndDirection( const Position3D & globalPosition,
			const Direction3D & globalDirection )
{
	MF_ASSERT( removalHandle_ == NO_SPACE_REMOVAL_HANDLE );

	globalPosition_ = globalPosition;
	globalDirection_ = globalDirection;
}


/**
 *	This method returns the vehicle that this entity is currently travelling on.
 *	If this entity is not currently riding a vehicle, NULL is returned.
 */
INLINE
Entity * Entity::pVehicle() const
{
	return pVehicle_;
}

/**
 *	This method returns the vehicle change number of this entity.
 *	It is incremented whenever the entity changes horses. It wraps.
 */
INLINE uint8 Entity::vehicleChangeNum() const
{
	return vehicleChangeNum_;
}


/**
 *	This method returns whether or not this entity is on the ground.
 */
INLINE bool Entity::isOnGround() const
{
	return isOnGround_;
}


/**
 *	This method returns the property event-stamps associated with some of the
 *	properties associated with this entity. This stamp is the event number when
 *	a property last changed.
 */
INLINE
const PropertyEventStamps & Entity::propertyEventStamps() const
{
	return propertyEventStamps_;
}


/**
 *	This method returns the last event number that was used by this entity.
 */
INLINE
EventNumber Entity::lastEventNumber() const
{
	return lastEventNumber_;
}


/**
 *	This method returns and consumes the next event number that this entity will
 *	use. The lastEventNumber counter is incremented.
 */
INLINE
EventNumber Entity::getNextEventNumber()
{
	++lastEventNumber_;

	return lastEventNumber_;
}


/**
 *	This method sets the entity up with the given fake ID.
 *	This method is used for searches for entities when sorted by id,
 *	when the searcher does not have the entity pointer.
 */
INLINE void Entity::fakeID( EntityID id )
{
	id_ = id;
}


INLINE void Entity::pRealChannel( CellAppChannel * channel )
{
	pRealChannel_ = channel;
}


// entity.ipp
