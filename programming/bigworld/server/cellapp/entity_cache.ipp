// entity_cache.ipp

#ifdef CODE_INLINE
#define INLINE	inline
#else
#define INLINE
#endif

const double BW_MAX_PRIORITY = DBL_MAX;

// -----------------------------------------------------------------------------
// Section: EntityCache methods
// -----------------------------------------------------------------------------

/**
 *	This contructor is only used in EntityCacheMap::find and EntityCacheMap::add
 */
INLINE EntityCache::EntityCache( const Entity * pEntity ) :
	pEntity_( pEntity ),
	flags_( 0 ),
	updateSchemeID_( pEntity ? pEntity->aoiUpdateSchemeID() : 0 ),
	lastEventNumber_( 0 ),
	lastVolatileUpdateNumber_( 0 ),
	detailLevel_( 0 ),
	idAlias_( NO_ID_ALIAS )
{
	if (pEntity)
	{
		this->resetClientState();
	}
}


/**
 *	Reset client related state.
 *
 *	This could be called either when this EntityCache is constructed or when it
 *	is removed from the client.
 */
INLINE void EntityCache::resetClientState()
{
	MF_ASSERT( this->pEntity() );

	lastEventNumber_ = this->pEntity()->lastEventNumber();
	lastVolatileUpdateNumber_ = this->pEntity()->volatileUpdateNumber() - 1;
	detailLevel_ = this->numLoDLevels();

	idAlias_ = NO_ID_ALIAS;
	// Using DBL_MAX to indicate that the last delta was "infinitely" large
	// and thus the new delta should not be capped.
	lastPriorityDelta_ = DBL_MAX;

	MF_ASSERT( detailLevel_ <= MAX_LOD_LEVELS );

	for (DetailLevel i = 0; i < detailLevel_; i++)
		lodEventNumbers_[i] = 0;
}


/**
 *	Called when the entity is removed from client. Reset client related state but
 *	retain server related ones.
 */
INLINE void EntityCache::onEntityRemovedFromClient()
{
	// flags_ contain both client and server related one. We need retain reset
	// client related ones but retain server related ones. 
	flags_ &= ~CLIENT_STATE;

	this->resetClientState();
}


/**
 *	This method calculates the distance to origin.
 */
INLINE float EntityCache::getDistanceSquared( const Vector3 & origin ) const
{
	float diffX = this->pEntity()->position().x - origin.x;
	float diffZ = this->pEntity()->position().z - origin.z;
	float ret = diffX * diffX + diffZ * diffZ;
	// FLT_MAX squared results in +inf; fix that
	return ret > FLT_MAX ? FLT_MAX : ret;
}
	

/**
 *	This method calculates the LoD priority.
 */
INLINE float EntityCache::getLoDPriority( const Vector3 & origin ) const
{
	if (AoIUpdateSchemes::shouldTreatAsCoincident( updateSchemeID_ ))
	{
		return 0.0f;
	}
	else
	{
		return this->getDistanceSquared( origin );
	}
}


/**
 *	This method updates the priority associated with this cache.
 */
INLINE void EntityCache::updatePriority( const Vector3 & origin )
{
	// TODO: The use of a double precision floating for the priority
	//		values gives 52 bits for significant figures.
	//
	//		If we increment the priority values by 10000 a second, we will
	//		run into trouble in about 140 years (provided the avatar doesn't
	//		change cells in that time), so we probably won't need to reset
	//		the priority values.
	//
	//		At present, we are incrementing priority values at around
	//		1000 per second - which assumes that everyone is roughly 500
	//		metres from the avatar.
	//
	// PM:	One solution to this would be to use an integer value for the
	//		priority and use a comparison that wraps. e.g. (x - y) > 0. This
	//		would work if the range in the priorities never exceeds half the
	//		range of the integer type.

	float distSQ = this->getDistanceSquared( origin );

	Priority delta = AoIUpdateSchemes::apply( updateSchemeID_, distSQ );

	// Limit the delta increase to a fraction of the previous delta to avoid
	// client's avatar filter starvation caused by rapid priority changes due
	// to AoI update scheme change.
	const double deltaGrowthThrottle =
		CellAppConfig::witnessUpdateDeltaGrowthThrottle();
	delta = std::min( delta, deltaGrowthThrottle * lastPriorityDelta_ );
	lastPriorityDelta_ = delta;

	// MF_ASSERT( priority_ < BW_MAX_PRIORITY );
	priority_ += delta;
	// MF_ASSERT( priority_ < BW_MAX_PRIORITY );
}


/**
 *	This method returns the number of LoD levels associated with this cache.
 */
INLINE int EntityCache::numLoDLevels() const
{
	MF_ASSERT( this->pEntity() );
	return EntityCache::numLoDLevels( *this->pEntity() );
}

/**
 *	Static method to return number of lod levels for an entity
 */
INLINE int EntityCache::numLoDLevels( const Entity & e )
{
	return e.pType()->description().lodLevels().size();
}



/**
 *	This method returns the priority associated with the entity.
 */
INLINE EntityCache::Priority EntityCache::priority() const
{
	return priority_;
}


/**
 *	This method sets the priority associated with the entity.
 */
INLINE
void EntityCache::priority( Priority newPriority )
{
	MF_ASSERT( fabs(newPriority) < BW_MAX_PRIORITY );
	priority_ = newPriority;
}


/**
 *	This method sets the number of the last event that was considered when the
 *	associated entity was updated.
 */
INLINE
void EntityCache::lastEventNumber( EventNumber number )
{
	lastEventNumber_ = number;
}


/**
 *	This method returns the number of the last event that was considered when
 *	the associated entity was updated.
 */
INLINE
EventNumber EntityCache::lastEventNumber() const
{
	return lastEventNumber_;
}


/**
 *	This method sets the volatile number associated with the last update.
 */
INLINE
void EntityCache::lastVolatileUpdateNumber( VolatileNumber number )
{
	lastVolatileUpdateNumber_ = number;
}


/**
 *	This method sets the volatile number associated with the last update.
 */
INLINE
VolatileNumber EntityCache::lastVolatileUpdateNumber() const
{
	return lastVolatileUpdateNumber_;
}


/**
 *	This method sets the current detail level this cache is at.
 */
INLINE void EntityCache::detailLevel( DetailLevel detailLevel )
{
	detailLevel_ = detailLevel;
}


/**
 *	This method sets the current detail level this cache is at.
 */
INLINE DetailLevel EntityCache::detailLevel() const
{
	return detailLevel_;
}


/**
 *	This method returns the id alias that is used for this entity.
 */
INLINE IDAlias EntityCache::idAlias() const
{
	return idAlias_;
}


/**
 *	This method sets the id alias that is used for this entity.
 */
INLINE void EntityCache::idAlias( IDAlias idAlias )
{
	idAlias_ = idAlias;
}


/**
 *	This method returns whether or not volatile position updates are detailed
 *	at all distances.
 */
INLINE bool EntityCache::isAlwaysDetailed() const
{
	return (flags_ & IS_ALWAYS_DETAILED) != 0;
}


/**
 *	This method sets whether or not volatile position updates are detailed at
 *	all distances.
 */
INLINE void EntityCache::isAlwaysDetailed( bool value )
{
	if (value)
	{
		flags_ |= IS_ALWAYS_DETAILED;
	}
	else
	{
		flags_ &= ~IS_ALWAYS_DETAILED;
	}
}


/**
 *	This method is used to stamp a cache level with the last event number that
 *	sent from it.
 *
 *	@see lodEventNumber
 */
INLINE void EntityCache::lodEventNumber( int level, EventNumber eventNumber )
{
	MF_ASSERT( 0 <= level && level < this->numLoDLevels() );
	lodEventNumbers_[ level ] = eventNumber;
}


/**
 *	This method is used get the event number a cache level was last stamped
 *	with.
 *
 *	@see lodEventNumber
 */
INLINE EventNumber EntityCache::lodEventNumber( int level ) const
{
	MF_ASSERT( 0 <= level && level < this->numLoDLevels() );
	return lodEventNumbers_[ level ];
}

// entity_cache.ipp
