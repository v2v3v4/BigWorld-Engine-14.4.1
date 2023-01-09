#ifdef CODE_INLINE
#define INLINE    inline
#else
/// INLINE macro.
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: Accessors
// -----------------------------------------------------------------------------

/**
 *	This method returns the index of this entity description.
 */
INLINE EntityTypeID EntityDescription::index() const
{
   	return index_;
}


/**
 *	This method sets the index of this entity description.
 */
INLINE void EntityDescription::index( EntityTypeID index )
{
	index_ = index;
}


/**
 *	This method returns the client index of this entity description. If no
 *	client type is associated with this type, -1 is returned.
 */
INLINE EntityTypeID EntityDescription::clientIndex() const
{
   	return clientIndex_;
}


/**
 *	This method sets the client index of this entity description.
 */
INLINE void EntityDescription::clientIndex( EntityTypeID index )
{
	clientIndex_ = index;
}


/**
 *	This method returns the client name of this entity description.
 *	The client name is the entity type that should be sent to the
 *	client. For example, if NPC is derived from Avatar, and NPC
 *	contains additional properties that the client does not need
 *	to know about, NPC objects can be sent to the client as Avatars.
 *	This means that the client does not need a specific script to
 *	handle NPCs.
 */
INLINE const BW::string& EntityDescription::clientName() const
{
	return clientName_;
}


/**
 *	This method returns the volatile info of the entity class.
 */
INLINE const VolatileInfo & EntityDescription::volatileInfo() const
{
	return volatileInfo_;
}


/**
 *	This method returns the number of properties that may be event-stamped. For
 *	properties that are sent to other clients, the event number of their last
 *	change is kept by the entity on the cell. This is used to decide what is out
 *	of date for a cache.
 */
INLINE unsigned int EntityDescription::numEventStampedProperties() const
{
	return numEventStampedProperties_;
}


/**
 *	This method returns the number of properties and methods that have the
 *	SendLatestOnly setting set to true. Entities store a pointer to their last
 *	HistoryEvent for each of these types of DataDescription or
 *	MethodDescriptions.
 */
INLINE unsigned int EntityDescription::numLatestChangeOnlyMembers() const
{
	return numLatestChangeOnlyMembers_;
}

// entity_description.ipp
