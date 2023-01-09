#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// -----------------------------------------------------------------------------
// BaseNodeInfo:: Accessor Methods for Limits.
// -----------------------------------------------------------------------------

/**
 *	Get accessor for the minimum pitch angle for the direction vector.
 *
 *	@return	Minimum pitch in radians.
 */
INLINE float BaseNodeInfo::minPitch() const
{
	return minPitch_;
}

/**
 *	Set accessor for the minimum pitch angle for the direction vector.
 *
 *	@param	newMinimum	Minimum pitch in radians.
 */
INLINE void BaseNodeInfo::minPitch( float newMinimum )
{
	minPitch_ = newMinimum;
}

/**
 *	Get accessor for the maximum pitch angle for the direction vector.
 *
 *	@return	Maximum pitch in radians.
 */
INLINE float BaseNodeInfo::maxPitch() const
{
	return maxPitch_;
}

/**
 *	Set accessor for the maximum pitch angle for the direction vector.
 *
 *	@param	newMaximum	Maximum pitch in radians.
 */
INLINE void BaseNodeInfo::maxPitch( float newMaximum )
{
	maxPitch_ = newMaximum;
}

/**
 *	Get accessor for the minimum yaw angle for the direction vector.
 *
 *	@return	Minimum yaw in radians.
 */
INLINE float BaseNodeInfo::minYaw() const
{
	return minYaw_;
}

/**
 *	Set accessor for the minimum yaw angle for the direction vector.
 *
 *	@param	newMinimum	Minimum yaw in radians.
 */
INLINE void BaseNodeInfo::minYaw( float newMinimum )
{
	minYaw_ = newMinimum;
}

/**
 *	Get accessor for the maximum yaw angle for the direction vector.
 *
 *	@return	Maximum yaw in radians.
 */
INLINE float BaseNodeInfo::maxYaw() const
{
	return maxYaw_;
}

/**
 *	Set accessor for the maximum yaw angle for the direction vector.
 *
 *	@param	newMaximum	Maximum yaw in radians.
 */
INLINE void BaseNodeInfo::maxYaw( float newMaximum )
{
	maxYaw_ = newMaximum;
}

/**
 *	Get accessor for the angular velocity in radians per second.
 *
 *	@return	Angular velocity in radians per second.
 */
INLINE float BaseNodeInfo::angularVelocity() const
{
	return angularVelocity_;
}

/**
 *	Get accessor for the angular threshold in radians.
 *
 *	@return	Angular threshold in radians.
 */
INLINE float BaseNodeInfo::angularThreshold() const
{
	return angularThreshold_;
}

/**
 *	Get accessor for the angular halflife in seconds.
 *
 *	@return	Angular Halflife in seconds.
 */
INLINE float BaseNodeInfo::angularHalflife() const
{
	return angularHalflife_;
}

/* tracker.ipp */