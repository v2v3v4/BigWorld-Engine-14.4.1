// simple_gui.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


INLINE void SimpleGUI::countDrawCall()
{
	drawCallCount_ += 1;
}

/**
 * Gets the current resolution override.
 */
INLINE const Vector2& SimpleGUI::resolutionOverride() const
{
	return resolutionOverride_;
}

/**
 * Determines whether or not the resolution has been overridden using
 * the resolutionOverride.
 */
INLINE bool SimpleGUI::usingResolutionOverride() const
{
	return !almostZero( resolutionOverride_.lengthSquared(), 0.0002f );
}

/**
 * Gets the current resolution used by the GUI (either real res, or current override)
 */
INLINE Vector2 SimpleGUI::screenResolution() const
{
	BW_GUARD;
	if ( usingResolutionOverride() )
	{
		return resolutionOverride_;
	}
	else
	{
		return Vector2( Moo::rc().screenWidth(), Moo::rc().screenHeight() );
	}
}

/**
 * Gets the current screen width used by the GUI system.
 */
INLINE float SimpleGUI::screenWidth() const
{
	return resolutionOverride_.x > 0 ? resolutionOverride_.x : Moo::rc().screenWidth();
}

/**
 * Gets the current screen height used by the GUI system.
 */
INLINE float SimpleGUI::screenHeight() const
{
	return resolutionOverride_.y > 0 ? resolutionOverride_.y : Moo::rc().screenHeight();
}

/**
 * Gets half the current screen width used by the GUI system.
 */
INLINE float SimpleGUI::halfScreenWidth() const
{
	return resolutionOverride_.x > 0 ? resolutionOverride_.x/2.0f : Moo::rc().halfScreenWidth();
}

/**
 * Gets half the current screen height used by the GUI system.
 */
INLINE float SimpleGUI::halfScreenHeight() const
{
	return resolutionOverride_.y > 0 ? resolutionOverride_.y/2.0f : Moo::rc().halfScreenHeight();
}

/**
 *	This method returns a counter for the current screen resolution used
 *	by the GUI. This value is only increased if the actual resolution is
 *	changed by the device.
 *
 *	@return Current resolution counter
 */
INLINE uint32 SimpleGUI::realScreenResolutionCounter() const
{
	return realResolutionCounter_;
}

/**
 *	Sets the minimum distance the mouse pointer has to travel after the left
 *	mouse button has been pressed before the movement is considered a drag.
 *
 *	@param	distance	the minimum drag distance (float, in clip space).
 *						The default value is 0.002.
 */
INLINE void SimpleGUI::dragDistance( float distance )
{
	dragDistanceSqr_ = distance * distance;	
}

// simple_gui.ipp