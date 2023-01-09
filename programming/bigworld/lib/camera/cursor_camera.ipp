// cursor_camera.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif



// -----------------------------------------------------------------------------
// Methods associated with Camera Behaviour.
// -----------------------------------------------------------------------------

/**
 *	Returns the position of the pivot as offset from the local origin.
 *
 *	@return	Pivot position.
 */
INLINE const Vector3 &CursorCamera::pivotPosition( void ) const
{
	return pivotPosition_;
}

/**
 *	Sets the position of the pivot as an offset relative to the local origin.
 *
 *	@param	newPosition			A 3D vector describing the point in space.
 */
INLINE void CursorCamera::pivotPosition( const Vector3 &newPosition )
{
	targetPivotPosition_ = newPosition;
}

/**
 *	Sets the position entered to the position of the pivot.
 *
 *	@param	pos			A 3D vector describing the point in space.
 */
INLINE void CursorCamera::setPivotPosition( const Vector3 &pos )
{
	targetPivotPosition_ = pos;
}

/**
 *	Scales the position of the pivot by the vector entered. This multiplies
 *	each dimension by the factor in each dimension of the factor.
 *	(x, y, z) * (fx, fy, fz) = (x*fx, y*fy, z*fz)
 *
 *	@param	factor		A vector representing a multiplying factor for each
 *						3D basis vector.
 */
INLINE void CursorCamera::scalePivotPosition( const Vector3 &factor )
{
	targetPivotPosition_.set( factor.x * targetPivotPosition_.x,
		factor.y * targetPivotPosition_.y,
		factor.z * targetPivotPosition_.z );
}


/**
 *	Returns the maximum distance of the camera from the pivot offset along the
 *	camera direction vector.
 *
 *	@return	Maximum camera distance from pivot.
 */
INLINE float CursorCamera::maxDistanceFromPivot( void ) const
{
	return maxDistanceFromPivot_;
}

/**
 *	Returns the maximum distance of the camera from the pivot offset along the
 *	camera direction vector.
 *
 *	@return	Maximum camera distance from pivot.
 */
INLINE float CursorCamera::targetMaxDistanceFromPivot( void ) const
{
	return targetMaxDistanceFromPivot_;
}


/**
 *	Returns the minimum distance of the camera from the pivot offset along the
 *	camera direction vector.
 *
 *	@return	Minimum camera distance from pivot.
 */
INLINE float CursorCamera::minDistanceFromPivot( void ) const
{
	return minDistanceFromPivot_;
}

/**
 *	Sets the minimum camera distance from the pivot offset.
 *
 *	@param	newDistance		The minimum camera offset from the pivot offset.
 */
INLINE void CursorCamera::minDistanceFromPivot( float newDistance )
{
	if ( newDistance > 0.0f )
	{
		minDistanceFromPivot_ = newDistance;
	}
	else
	{
		minDistanceFromPivot_ = 0.0f;
	}
}

/**
 *	Returns the minimum distance within which the camera can be to the terrain.
 *
 *	@return	Distance in metres.
 */
INLINE float CursorCamera::minDistanceFromTerrain( void ) const
{
	return minDistanceFromTerrain_;
}

/**
 *	Sets the minimum distance within which the camera can be to the terrain.
 *
 *	@param	newDistance		The minimum distance in metres.
 */
INLINE void CursorCamera::minDistanceFromTerrain( float newDistance )
{
	minDistanceFromTerrain_ = newDistance;
}

/**
 *	Returns the half-life for camera movement. This value is the time for the
 *	camera to move to half the distance between its current position and the
 *	desired camera position.
 *
 *	@return	Half-life value in seconds.
 */
INLINE float CursorCamera::movementHalfLife( void ) const
{
	return movementHalfLife_;
}

/**
 *	Sets the half-life for camera movement. This value is the time for the
 *	camera to move to half the distance between its current position and the
 *	desired camera position.
 *
 *	@param	halfLifeInSeconds	The new Half-life value in seconds.
 */
INLINE void CursorCamera::movementHalfLife( float halfLifeInSeconds )
{
	movementHalfLife_ = halfLifeInSeconds;
}

/**
 *	Returns the half-life for camera direction changing. This value is the time
 *	for the camera to turn to half the distance between its current view target
 *	and the desired camera direction.
 *
 *	@return	Half-life value in seconds.
 */
INLINE float CursorCamera::turningHalfLife( void ) const
{
	return turningHalfLife_;
}

/**
 *	Sets the half-life for camera direction changing. This value is the time
 *	for the camera to turn to half the distance between its current view target
 *	and the desired camera direction.
 *
 *	@param	halfLifeInSeconds	The new Half-life value in seconds.
 */
INLINE void CursorCamera::turningHalfLife( float halfLifeInSeconds )
{
	turningHalfLife_ = halfLifeInSeconds;
}

/**
 *	Returns the 3D-vector pointing in the upright direction for the camera.
 *
 *	@return	Upright vector.
 */
INLINE const Vector3 &CursorCamera::uprightDirection( void ) const
{
	return uprightDirection_;
}

/**
 *	Sets the vector pointing in the upright direction for the camera.
 *
 *	@param	newDirection		The new upright Vector.
 */
INLINE void CursorCamera::uprightDirection( const Vector3 &newDirection )
{
	uprightDirection_ = newDirection;
}


/**
 *	Sets the current target for the camera via a transform matrix produced.
 *	The camera centres around the point that results when the origin is
 *	applied to this matrix.
 *
 *	@param	pMProv		The transform matrix for a point in space that
 *							the camera should hover around.
 */
INLINE void CursorCamera::target( MatrixProviderPtr pMProv )
{
	pTarget_ = pMProv;
}


/**
 *	Sets the input to the CursorCamera. This matrix is the source of the
 *	orientation of the camera. It is usually a MatrixProvider which
 *	queries a DirectionCursor.
 */
INLINE void CursorCamera::source( MatrixProviderPtr pMProv )
{
	pSource_ = pMProv;
}


/**
 *	Returns true if the camera is in first-person mode.
 *
 *	@return	True, if in first-person perspective. False, otherwise.
 */
INLINE const bool CursorCamera::firstPersonPerspective( void ) const
{
	return firstPersonPerspective_;
}

/**
 *	Sets the mode for first-person perspective.
 *
 *	@param	flag	A boolean value. If true, the camera moves into
 *					first-person perspective. If false, the camera moves into
 *					third-person perspective.
 */
INLINE void CursorCamera::firstPersonPerspective( bool flag )
{
	firstPersonPerspective_ = flag;
}

/**
 *	Returns true if the camera view is to be reflected. This makes the
 *	camera look at the model from the front instead of from behind.
 *
 *	@return True, if the camera pitch is to be reflected.
 */
INLINE bool CursorCamera::reverseView( void ) const
{
	return reverseView_;
}

/**
 *	Sets the flag for determining if the camera view is to be reflected.
 *
 *	@param flag		The new true or false value of the flag.
 */
INLINE void CursorCamera::reverseView( bool flag )
{
	reverseView_ = flag;
}


// -----------------------------------------------------------------------------
// Methods associated with Camera Position.
// -----------------------------------------------------------------------------


/**
 *	Returns the actual direction of the camera in world space.
 *
 *	@return	The direction vector.
 */
INLINE const Vector3 &CursorCamera::dirInWS( void ) const
{
	return cameraDirInWS_;
}

/**
 *	Returns the actual position of the camera in world space.
 *
 *	@return	The position vector.
 */
INLINE const Vector3 &CursorCamera::posInWS( void ) const
{
	return cameraPosInWS_;
}


// -----------------------------------------------------------------------------
// Protected Methods associated with Camera Direction.
// -----------------------------------------------------------------------------

/**
 *	Returns the yaw of the camera direction in world space.
 *
 *	@return	Yaw of the camera direction in radians.
 */
INLINE const Angle &CursorCamera::yaw( void ) const
{
	return yaw_;
}

/**
 *	Sets the yaw of the camera direction in world space.
 *
 *	@param	yawInRadians		The new yaw of the camera direction.
 */
INLINE void CursorCamera::yaw( const Angle &yawInRadians )
{
	yaw_ = yawInRadians;
}

/**
 *	Returns the pitch of the camera direction in world space.
 *
 *	@return	Pitch of the camera direction in radians.
 */
INLINE const Angle &CursorCamera::pitch( void ) const
{
	return pitch_;
}

/**
 *	Sets the pitch of the camera direction in world space.
 *
 *	@param	pitchInRadians		The new pitch of the camera direction.
 */
INLINE void CursorCamera::pitch( const Angle &pitchInRadians )
{
	pitch_ = pitchInRadians;
}

/**
 *	Returns the roll of the camera direction in world space.
 *
 *	@return	roll of the camera direction in radians.
 */
INLINE const Angle &CursorCamera::roll( void ) const
{
	return roll_;
}

/**
 *	Sets the roll of the camera direction in world space.
 *
 *	@param	rollInRadians		The new roll of the camera direction.
 */
INLINE void CursorCamera::roll( const Angle &rollInRadians )
{
	roll_ = rollInRadians;
}


// cursor_camera.ipp
