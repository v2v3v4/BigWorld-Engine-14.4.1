// direction_cursor.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Methods associated with SimpleSpeedProvider.
// -----------------------------------------------------------------------------

INLINE SimpleSpeedProvider::SimpleSpeedProvider( float horizontalSpeed,  float verticalSpeed )
{
	horizontalSpeed_ = horizontalSpeed;
	verticalSpeed_ = verticalSpeed;
}


// -----------------------------------------------------------------------------
// Methods associated with DirectionCursor Representation.
// -----------------------------------------------------------------------------

/**
 *	Returns the direction of the cursor represented by the DirectionCursor.
 *	If the old direction is out of date, the direction is recalculated,
 *	normalised, and cached before returning it.
 *	The convention for yaw is: z-axis is the zero angle, with z-axis to x-axis
 *	as positive increase.
 *
 *	@return	A normalised 3D vector representing the direction of the
 *			DirectionCursor.
 */
INLINE const Vector3& DirectionCursor::direction( void ) const
{
	if ( refreshDirection_ )
	{
		direction_.setPitchYaw( -this->pitch(), this->yaw() );
		refreshDirection_ = false;
	}
	return direction_;
}

/**
 *	Sets the cursor direction by the yaw and pitch angles. This method is
 *	useful for setting both yaw and pitch values simultaneously. If only one
 *	or the other needs to be set, (but not both,) the separate yaw and pitch
 *	methods may be used instead.
 *	The convention for yaw is: z-axis is the zero angle, with z-axis to x-axis
 *	as positive increase.
 *
 *	@param	pitchInRadians	The new pitch angle in radians.
 *	@param	yawInRadians	The new yaw angle in radians.
 */
INLINE void DirectionCursor::direction( const Angle &pitchInRadians,
		const Angle &yawInRadians )
{
	this->pitch( pitchInRadians );
	this->yaw( yawInRadians );

	// Set dirty flag for direction vector.
	refreshDirection_ = true;
}


/**
 *	Returns the pitch angle of the cursor direction.
 *
 *	@return	The pitch angle of the cursor in radians.
 */
INLINE const Angle &DirectionCursor::pitch( void ) const
{
	return pitch_;
}

/**
 *	Sets the pitch angle of the cursor direction. If both pitch and yaw need to
 *	be set simultaneously, the direction method may be used instead.
 *
 *	@param	pitchInRadians	The new pitch angle in radians.
 */
INLINE void DirectionCursor::pitch( const Angle &pitchInRadians )
{
	if ( pitch_ != pitchInRadians )
	{
		pitch_ = pitchInRadians;

		// Set dirty flag for direction vector.
		refreshDirection_ = true;

		// Reset elapsed time for kicking in the look-spring behaviour.
		elapsedTimeForLookSpring_ = 0.0;
	}
}

/**
 *	Returns the yaw angle of the cursor direction.
 *	The convention for yaw is: x-axis is the zero angle, with z-axis to x-axis
 *	as positive increase.
 *
 *	@return	The yaw angle of the cursor in radians.
 */
INLINE const Angle &DirectionCursor::yaw( void ) const
{
	return yaw_;
}

/**
 *	Sets the yaw angle of the cursor direction. If both pitch and yaw need to
 *	be set simultaneously, the direction method may be used instead.
 *	The convention for yaw is: z-axis is the zero angle, with z-axis to x-axis
 *	as positive increase.
 *
 *	@param	yawInRadians	The new yaw angle in radians.
 */
INLINE void DirectionCursor::yaw( const Angle &yawInRadians )
{
	if ( yaw_ != yawInRadians )
	{
		yaw_ = yawInRadians;

		// Set dirty flag for direction vector.
		refreshDirection_ = true;

		// Reset elapsed time for kicking in the look-spring behaviour.
		elapsedTimeForLookSpring_ = 0.0;
	}
}


/**
 *	Returns the roll angle of the cursor direction.
 *
 *	@return	The roll angle of the cursor in radians.
 */
INLINE const Angle &DirectionCursor::roll( void ) const
{
	return roll_;
}

/**
 *	Sets the roll angle of the cursor direction.
 *
 *	@param	rollInRadians	The new roll angle in radians.
 */
INLINE void DirectionCursor::roll( const Angle &rollInRadians )
{
	roll_ = rollInRadians;
}


// -----------------------------------------------------------------------------
// Methods associated with DirectionCursor Behaviour.
// -----------------------------------------------------------------------------

/**
 *	Returns true if vertical movement is inverted. When vertical movement is
 *	inverted, a downward movement will look up and an upward movement will
 *	look down.
 *
 *	@return	True if vertical movement is inverted. False, otherwise.
 */
INLINE bool DirectionCursor::invertVerticalMovement( void ) const
{
	return invertVerticalMovement_;
}

/**
 *	Sets vertical movement to be inverted or not inverted.
 *
 *	@param flag		A boolean flag indicating whether vertical movement is to
 *					be inverted or not.
 */
INLINE void DirectionCursor::invertVerticalMovement( bool flag )
{
	invertVerticalMovement_ = flag;
}

/**
 *	Returns the multiplier to the mouse readings. This is a crude mouse
 *	movement to angle converter.
 *
 *	@return	Mouse sensitivity multiplier.
 */
INLINE float DirectionCursor::mouseSensitivity( void ) const
{
	return mouseSensitivity_;
}

/**
 *	Sets the multiplier to the mouse readings. This is a crude mouse movement
 *	to angle converter. 
 *
 *	@param	newMultiplier		The mouse sensitivity multiplier.
 */
INLINE void DirectionCursor::mouseSensitivity( float newMultiplier )
{
	mouseSensitivity_ = newMultiplier;
}

/**
 *	Returns the value of the mouse horizontal/vertical bias. The value
 *	ranges between 0.0 and 1.0, with 0.0 meaning biased fully towards vertical
 *	movement, 1.0 meaning fully biased towards horizontal movement, and 0.5
 *	meaning both readings are equal.
 *	
 *	@return	Mouse horizontal sensitivity bias.
 */
INLINE float DirectionCursor::mouseHVBias( void ) const
{
	return mouseHVBias_;
}

/**
 *	Sets the mouse horizontal sensitivity bias. The value ranges between 0.0
 *	and 1.0, with 0.0 meaning biased fully towards vertical movement, 1.0
 *	meaning fully biased towards horizontal movement, and 0.5 meaning both
 *	readings are equal.
 *
 *	@param	newHBias		Mouse horizontal bias between 0.0 and 1.0.
 */
INLINE void DirectionCursor::mouseHVBias( float newHBias)
{
	mouseHVBias_ = Math::clamp( 0.0f, newHBias, 1.0f );
}

/**
 *	Returns get maximum pitch allowed for the cursor direction.
 *
 *	@return	Maximum pitch in radians.
 */
INLINE const Angle &DirectionCursor::maxPitch( void ) const
{
	return maxPitch_;
}

/**
 *	Sets the maximum pitch allowed for the cursor direction.
 *
 *	@param	newAngleInRadians	The maximum pitch in radians.
 */
INLINE 	void DirectionCursor::maxPitch ( const Angle &newAngleInRadians )
{
	maxPitch_ = newAngleInRadians;
}

/**
 *	Returns get minimum pitch allowed for the cursor direction.
 *
 *	@return	Minimum pitch in radians.
 */
INLINE const Angle &DirectionCursor::minPitch( void ) const
{
	return minPitch_;
}

/**
 *	Sets the minimum pitch allowed for the cursor direction.
 *
 *	@param	newAngleInRadians	The minimum pitch in radians.
 */
INLINE void DirectionCursor::minPitch ( const Angle &newAngleInRadians )
{
	minPitch_ = newAngleInRadians;
}

/**
 *	Returns the flag stating if the direction cursor will decay towards a
 *	neutral pitch after a set amount of time.
 *
 *	@return	A true or false value.
 */
INLINE bool DirectionCursor::lookSpring( void ) const
{
	return lookSpring_;
}

/**
 *	Sets the flag stating if the direction cursor will decay towards a
 *	neutral pitch after a set amount of time.
 *
 *	@param	flag	The new setting for the look-spring behaviour.
 */
INLINE void DirectionCursor::lookSpring( bool flag )
{
	lookSpring_ = flag;
}

/**
 *	Returns the decay value of the look spring movement. It is a value between
 *	0.0f and 1.0f. A value of 1.0f means no decay towards neutral, while a
 *	value of 0.0f means instantaneous movement towards neutral.
 *
 *	@return	The value between 0.0f and 1.0f.
 */
INLINE float DirectionCursor::lookSpringRate( void ) const
{
	return lookSpringRate_;
}

/**
 *	Sets the decay value of the look spring movement. It is a value between
 *	0.0f and 1.0f. A value of 1.0f means no decay towards neutral, while a
 *	value of 0.0f means instantaneous movement towards neutral.
 *
 *	@param	newRate		The value between 0.0f and 1.0f.
 */
INLINE void DirectionCursor::lookSpringRate( float newRate )
{
	lookSpringRate_ = Math::clamp( 0.0f, newRate, 1.0f );
}

/**
 *	Sets the idle time before look spring kicks in if lookSpringOnIdle is set.
 *
 *	@return	A value in seconds.
 */
INLINE double DirectionCursor::lookSpringIdleTime( void ) const
{
	return lookSpringIdleTime_;
}

/**
 *	Returns the idle time before look spring kicks in if lookSpringOnIdle is
 *	set.
 *
 *	@param	newTimeInSeconds	The time in seconds.
 */
INLINE void DirectionCursor::lookSpringIdleTime( double newTimeInSeconds )
{
	lookSpringIdleTime_ = newTimeInSeconds;
}

/**
 *	Returns the flag stating if the direction cursor would like to return
 *	neutral pitch upon movement of the character.
 *
 *	@return	A true or false value.
 */
INLINE bool DirectionCursor::lookSpringOnMove( void ) const
{
	return lookSpringOnMove_;
}

/**
 *	Sets the flag stating if the direction cursor would like to return
 *	neutral pitch upon movement of the character.
 *
 *	@param	flag	The new setting for the look-spring behaviour.
 */
INLINE void DirectionCursor::lookSpringOnMove( bool flag )
{
	lookSpringOnMove_ = flag;
}

/*~ function DirectionCursor.forceLookSpring
 *
 *	This function forces a return to a neutral pitch
 *
 *	@return			None
 */
/**
 *	Forces a return to neutral pitch.
 */
INLINE void DirectionCursor::forceLookSpring( void )
{
	forcedLookSpring_ = true;
}


// direction_cursor.ipp