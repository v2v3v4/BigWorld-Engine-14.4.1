// entity_type.ipp

#ifdef CODE_INLINE
#define INLINE	inline
#else
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: EntityType
// -----------------------------------------------------------------------------

/**
 *	This method returns the volatile info associated with this entity type. The
 *	volatile info indicates what data associated with entities of this type
 *	changes regularly and so should be sent regularly (such as position and
 *	direction).
 */
INLINE
const VolatileInfo & EntityType::volatileInfo() const
{
	return entityDescription_.volatileInfo();
}


/**
 *	This method returns an object that describes this entity type. The
 *	description includes things such as the properties and method supported by
 *	entities of this type.
 */
INLINE
const EntityDescription & EntityType::description() const
{
	return entityDescription_;
}


/**
 *	This method returns the name associated with the entity type.
 */
INLINE
const char * EntityType::name() const
{
	return entityDescription_.name().c_str();
}


/**
 *	This method returns the ID of the client entity type associated with this
 *	server entity type. Often, this is the same as this object's ID but may not
 *	be so. For example, an entity of type NPC on the server may be of type
 *	Avatar on the client.
 */
INLINE
EntityTypeID EntityType::clientTypeID() const
{
	return entityDescription_.clientIndex();
}

// entity_type.ipp
