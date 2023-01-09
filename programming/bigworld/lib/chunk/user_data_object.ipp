#ifdef CODE_INLINE
#define INLINE	inline
#else
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: UserDataObject
// -----------------------------------------------------------------------------

/**
 *	This method returns the GUID associated with this user data object
 *  Each user data object in the world has a unique ID.
 */
INLINE
const UniqueID& UserDataObject::guid() const
{
	return guid_;
}

/**
 *	This method returns the position of this user data object
 */
INLINE
const Position3D & UserDataObject::position() const
{
	return globalPosition_;
}


/**
 *	This method returns the direction this user data object
 *  is currently facing.
 */
INLINE
const Direction3D & UserDataObject::direction() const
{
	return globalDirection_;
}

// UserDataObject.ipp
