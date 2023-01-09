// flexicam.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Methods associated with camera position relative to the target.
// -----------------------------------------------------------------------------

/**
 *	Get the preferred offset position from the target for the camera.
 *
 *	@return The preferred offset position as a 3D vector.
 */
INLINE const Vector3 &FlexiCam::preferredPos(void) const
{
	return preferredPos_;
}

/**
 *	Set the preferred offset position for the camera from the target.
 *
 *	@param	newPos	The vector specifying the new preferred position offset.
 *
 *	@return	None.
 */
INLINE void FlexiCam::preferredPos(const Vector3 &newPos)
{
	preferredPos_ = newPos;
}

/**
 *	Get the actual position of the camera relative to the target.
 *
 *	@return	The actual position as a 3D vector.
 */
INLINE const Vector3 &FlexiCam::actualPos(void) const
{
	return actualPos_;
}

/**
 *	Set the actual position of the camera relative to the target.
 *
 *	@param	newPos	The vector specifying the new actual position.
 *
 *	@return None.
 */
INLINE void FlexiCam::actualPos(const Vector3 &newPos)
{
	actualPos_ = newPos;
}

/**
 *	Get the preferred yaw angle relative to the target. 
 *	The yaw value is zero along the negative z-axis, it is increasing in the
 *	direction from x to z axes. It is the yaw-value of the view direction
 *	relative to the target, ie. if the camera is directly behind the target,
 *	the angle position is -90 degrees, but the yaw value of the target's
 *	view is actually 0 degrees. Likewise, if the target is looking 45 degrees
 *	to the right, the angle position is -135 degrees, but the yaw value is
 *	-45 degrees.
 *
 *	@return The yaw angle of the preferred camera position relative to the
 *			target, in degrees.
 */
INLINE const float FlexiCam::preferredYaw(void) const
{
	float yawInRadians = atan2f(preferredPos_.z,
							   preferredPos_.x) + DEG_TO_RAD(90);
	if (yawInRadians > MATH_PI)
	{
		yawInRadians -= float(2 * MATH_PI);
	}
	return RAD_TO_DEG(yawInRadians);
}


/**
 *	Change the preferred yaw of the camera position by deltaDegrees.
 *
 *	@param	deltaDegree	The change to the yaw value of the camera position.
 *
 *	@return None.
 */
INLINE void FlexiCam::changePreferredYawBy(float deltaDegree)
{
	Matrix rotation;
	rotation.setRotateY(DEG_TO_RAD(deltaDegree));
	preferredPos_ = rotation.applyPoint(preferredPos_);
}

/**
 *	Get the preferred pitch of the camera position in degrees.
 *	The pitch value is zero along the positive z-axis, it increases in the
 *	direction from z-axis to y-axis. It is the pitch of the view direction
 *	relative to the target (what the target is looking at.) If the camera
 *	is directly behind the target, it is at angle 180 degrees. However the
 *	pitch of the view is 0 degrees. If the target looks up by 45 degrees,
 *	the camera drops down 45 degrees from there - pitch is 45 degrees, but
 *	angle is 225 degrees or -135 degrees (when restricted to 180 and -180.)
 *
 *	@return The pitch angle of the preferred camera position relative to the
 *			target, in degrees.
 */
INLINE const float FlexiCam::preferredPitch(void) const
{
	float pitchInRadians = atan2f(preferredPos_.y,
								 preferredPos_.z) - DEG_TO_RAD(180);
	if (pitchInRadians < -MATH_PI)
	{
		pitchInRadians += float(2 * MATH_PI);
	}
	return RAD_TO_DEG(pitchInRadians);
}

/**
 *	Change the preferred pitch of the camera position by deltaDegrees.
 *
 *	@param	deltaDegree	The change to the yaw value of the camera position.
 *
 *	@return None.
 */
INLINE void FlexiCam::changePreferredPitchBy(float deltaDegree)
{
	Matrix rotation;
	rotation.setRotateX(DEG_TO_RAD(deltaDegree));
	preferredPos_ = rotation.applyPoint(preferredPos_);
}

/**
 *	Get camera position acceleration. Acceleration is set as a half-life
 *	value in seconds to reach half the remaining distance.
 *
 *	@return The camera acceleration.
 */
INLINE float FlexiCam::positionAcceleration(void) const
{
	return positionAcceleration_;
}

/**
 *	Set camera position acceleration. Acceleration is set as a half-life
 *	value in seconds to reach half the remaining distance.
 *
 *	@param	newAcceleration	The new value for the camera acceleration.
 *
 *	@return None.
 */
INLINE void FlexiCam::positionAcceleration(float newAcceleration)
{
	positionAcceleration_ = newAcceleration;
}


// -----------------------------------------------------------------------------
// Methods associated with camera view relative to the target.
// -----------------------------------------------------------------------------

/**
 *	Get the current target for the camera.
 *
 *	@return The matrix representing the target.
 */
INLINE MatrixProviderPtr FlexiCam::pTarget() const
{
	return pTarget_;
}

/**
 *	Set the current target for the camera.
 *
 *	@param	pNewTarget	The new view target for the camera.
 *
 *	@return	None.
 */
INLINE void FlexiCam::pTarget( MatrixProviderPtr pNewTarget )
{
	pTarget_ = pNewTarget;
}


/**
 *	Get the upright direction for the camera.
 *
 *	@return Upright direction as a 3D vector.
 */
INLINE const Vector3 &FlexiCam::uprightDir(void) const
{
	return uprightDir_;
}

/**
 *	Set the upright direction for the camera.
 *
 *	@param	newDir	The new upright direction for the camera.
 *
 *	@return None.
 */
INLINE void FlexiCam::uprightDir(const Vector3 &newDir)
{
	uprightDir_ = newDir;
}


/**
 *	Get the current camera direction.
 *
 *	@return A 3D vector representing the current camera direction relative to
 *			to the camera.
 */
INLINE const Vector3 &FlexiCam::actualDir(void) const
{
	return actualDir_;
}

/**
 *	Set the current camera direction.
 *
 *	@param	newDir	The vector in the direction of camera view.
 *
 *	@return None.
 */
INLINE void FlexiCam::actualDir(const Vector3 &newDir)
{
	actualDir_ = newDir;
}

/**
 *	Get the view offset from the target.
 *
 *	@return The vector representing the view target offset from the origin of
 *			the target.
 */
INLINE const Vector3 &FlexiCam::viewOffset(void) const
{
	return viewOffset_;
}

/**
 *	Set the view offset from the target.
 *
 *	@param	newOffset	The new viewing offset from the target.
 *
 *	@return None.
 */
INLINE void FlexiCam::viewOffset(const Vector3 &newOffset)
{
	viewOffset_ = newOffset;
}

/**
 *	Get camera target tracking acceleration. Acceleration is set as a half-life
 *	value in seconds to reach half the remaining distance.
 *
 *	@return The camera angle acceleration value from 0.0 to 1.0.
 */
INLINE float FlexiCam::trackingAcceleration(void) const
{
	return trackingAcceleration_;
}

/**
 *	Set camera target tracking acceleration. Acceleration is set as a half-life
 *	value in seconds to reach half the remaining distance.
 *
 *	@param	newAcceleration	The new accleration for the camera angle.
 */
INLINE void FlexiCam::trackingAcceleration(float newAcceleration)
{
	trackingAcceleration_ = newAcceleration;
}


// -----------------------------------------------------------------------------
// Methods associated with transforms and the 3D world.
// -----------------------------------------------------------------------------

/**
 *	Returns the actual position of the camera in world space.
 *
 *	@return	The position vector.
 */
INLINE const Vector3 &FlexiCam::posInWS( void ) const
{
	return actualPos_;
}


/**
 *	Returns the actual direction of the camera in world space.
 *
 *	@return	The direction vector.
 */
INLINE const Vector3 &FlexiCam::dirInWS( void ) const
{
	return viewDir_;
}


// -----------------------------------------------------------------------------
// Destructor for FlexiCam.
// -----------------------------------------------------------------------------

/**
 *	The destructor for the FlexiCam class.
 */
INLINE FlexiCam::~FlexiCam()
{
}

/* flexicam.ipp*/