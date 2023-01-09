
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


/**
 *	This method sets the selectable field of view. This is the angle, in
 *	radians, from the bottom of the selection volume to the top. Since the
 *	aspect ratio is 1, this is also the angle from the left to the right.
 *
 *	@param angleRadians	The field of view angle in radians.
 */
INLINE void EntityPicker::selectionFoV( float angleRadians )
{
	selectionFoV_ = angleRadians;
}


/**
 *	This method returns the selectable field of view.
 */
INLINE float EntityPicker::selectionFoV() const
{
	return selectionFoV_;
}


/**
 *	These methods set and return the selectable field of view when not
 *	overridden by an item.
 */
INLINE void EntityPicker::defaultSelectionFoV( float angleRadians )
{
	defaultSelectionFoV_ = angleRadians;
}
INLINE float EntityPicker::defaultSelectionFoV() const
{
	return defaultSelectionFoV_;
}


/**
 *	This method sets the maximum selection distance.
 *
 *	@param distance	The maximum selection distance in metres.
 */
INLINE void EntityPicker::selectionDistance( float distance )
{
	distance_ = distance;
}

INLINE void EntityPicker::selectionDistancePush( float distance )
{
	oldDistance_ = distance_;
	distance_ = distance;
}

INLINE void EntityPicker::selectionDistancePop( )
{
	distance_ = oldDistance_;
}



/**
 *	This method returns the maximum selection distance.
 */
INLINE float EntityPicker::selectionDistance() const
{
	return distance_;
}


/**
 *	These methods set and return the maximum selection distance when not
 *	overridden by an item.
 */
INLINE void EntityPicker::defaultSelectionDistance( float distance )
{
	defaultDistance_ = distance;
}
INLINE float EntityPicker::defaultSelectionDistance() const
{
	return defaultDistance_;
}


/**
 *	This method sets the deselection field of view. This is the angle, in
 *	radians, from the bottom of the deselection volume to the top. Since the
 *	aspect ratio is 1, this is also the angle from the left to the right. Once
 *	a selected object leaves this volume, it is deselected.
 *
 *	@param angleRadians	The field of view angle in radians.
 */
INLINE void EntityPicker::deselectionFoV( float angleRadians )
{
	deselectionFoV_ = angleRadians;
}


/**
 *	This method returns the deselectable field of view.
 */
INLINE float EntityPicker::deselectionFoV() const
{
	return deselectionFoV_;
}


/**
 *	These methods set and return the deselectable field of view when not
 *	overridden by an item.
 */
INLINE void EntityPicker::defaultDeselectionFoV( float angleRadians )
{
	defaultDeselectionFoV_ = angleRadians;
}
INLINE float EntityPicker::defaultDeselectionFoV() const
{
	return defaultDeselectionFoV_;
}


/**
 *	This method sets the auto aim angle. This is the angle, in
 *	radians, from the bottom of the auto aim volume to the top.
 *
 *	@param angleRadians	The field of view angle in radians.
 */
INLINE void EntityPicker::autoAimFrictionHorizontalAngle( float angleRadians )
{
	autoAimFrictionHorizontalAngle_ = angleRadians;
}
INLINE void EntityPicker::autoAimFrictionVerticalAngle( float angleRadians )
{
	autoAimFrictionVerticalAngle_ = angleRadians;
}


/**
 *	This method returns the auto aim angle.
 */
INLINE float EntityPicker::autoAimFrictionHorizontalAngle() const
{
	return autoAimFrictionHorizontalAngle_;
}
INLINE float EntityPicker::autoAimFrictionVerticalAngle() const
{
	return autoAimFrictionVerticalAngle_;
}


/**
 *	This method sets the auto aim distance. Entities need to be
 *	within this distance to be considered for auto aim.
 *
 *	@param distance	The distance in metres.
 */
INLINE void EntityPicker::autoAimFrictionDistance( float distance )
{
	autoAimFrictionDistance_ = distance;
}


/**
 *	This method returns the auto aim distance.
 */
INLINE float EntityPicker::autoAimFrictionDistance() const
{
	return autoAimFrictionDistance_;
}


/**
 *	This method sets the auto aim angle. This is the angle, in
 *	radians, from the bottom of the auto aim volume to the top.
 *
 *	@param angleRadians	The field of view angle in radians.
 */
INLINE void EntityPicker::autoAimFrictionHorizontalFalloffAngle( float angleRadians )
{
	autoAimFrictionHorizontalFalloffAngle_ = angleRadians;
}
INLINE void EntityPicker::autoAimFrictionVerticalFalloffAngle( float angleRadians )
{
	autoAimFrictionVerticalFalloffAngle_ = angleRadians;
}


/**
 *	This method returns the auto aim angle.
 */
INLINE float EntityPicker::autoAimFrictionHorizontalFalloffAngle() const
{
	return autoAimFrictionHorizontalFalloffAngle_;
}
INLINE float EntityPicker::autoAimFrictionVerticalFalloffAngle() const
{
	return autoAimFrictionVerticalFalloffAngle_;
}


/**
 *	This method sets the minimum distance for full auto aim friction.
 *	Short of this distance, the auto aim friction falls off to zero at zero.
 *
 *	@param distance	The distance in metres.
 */
INLINE void EntityPicker::autoAimFrictionMinimumDistance( float distance )
{
	autoAimFrictionMinimumDistance_ = distance;
}


/**
 *	This method returns the minimum distance for full auto aim friction.
 */
INLINE float EntityPicker::autoAimFrictionMinimumDistance() const
{
	return autoAimFrictionMinimumDistance_;
}


/**
 *	This method sets the auto aim distance. Entities need to be
 *	within this distance to be considered for auto aim.
 *
 *	@param distance	The distance in metres.
 */
INLINE void EntityPicker::autoAimFrictionFalloffDistance( float distance )
{
	autoAimFrictionFalloffDistance_ = distance;
}


/**
 *	This method returns the auto aim distance.
 */
INLINE float EntityPicker::autoAimFrictionFalloffDistance() const
{
	return autoAimFrictionFalloffDistance_;
}


/**
 *	This method sets the auto aim angle. This is the angle, in
 *	radians, from the bottom of the auto aim volume to the top.
 *
 *	@param angleRadians	The field of view angle in radians.
 */
INLINE void EntityPicker::autoAimAdhesionHorizontalAngle( float angleRadians )
{
	autoAimAdhesionHorizontalAngle_ = angleRadians;
}
INLINE void EntityPicker::autoAimAdhesionVerticalAngle( float angleRadians )
{
	autoAimAdhesionVerticalAngle_ = angleRadians;
}


/**
 *	This method returns the auto aim angle.
 */
INLINE float EntityPicker::autoAimAdhesionHorizontalAngle() const
{
	return autoAimAdhesionHorizontalAngle_;
}
INLINE float EntityPicker::autoAimAdhesionVerticalAngle() const
{
	return autoAimAdhesionVerticalAngle_;
}


/**
 *	This method sets the auto aim distance. Entities need to be
 *	within this distance to be considered for auto aim.
 *
 *	@param distance	The distance in metres.
 */
INLINE void EntityPicker::autoAimAdhesionDistance( float distance )
{
	autoAimAdhesionDistance_ = distance;
}


/**
 *	This method returns the auto aim distance.
 */
INLINE float EntityPicker::autoAimAdhesionDistance() const
{
	return autoAimAdhesionDistance_;
}


/**
 *	This method sets the auto aim angle. This is the angle, in
 *	radians, from the bottom of the auto aim volume to the top.
 *
 *	@param angleRadians	The field of view angle in radians.
 */
INLINE void EntityPicker::autoAimAdhesionHorizontalFalloffAngle( float angleRadians )
{
	autoAimAdhesionHorizontalFalloffAngle_ = angleRadians;
}
INLINE void EntityPicker::autoAimAdhesionVerticalFalloffAngle( float angleRadians )
{
	autoAimAdhesionVerticalFalloffAngle_ = angleRadians;
}


/**
 *	This method returns the auto aim angle.
 */
INLINE float EntityPicker::autoAimAdhesionHorizontalFalloffAngle() const
{
	return autoAimAdhesionHorizontalFalloffAngle_;
}
INLINE float EntityPicker::autoAimAdhesionVerticalFalloffAngle() const
{
	return autoAimAdhesionVerticalFalloffAngle_;
}


/**
 *	This method sets the auto aim distance. Entities need to be
 *	within this distance to be considered for auto aim.
 *
 *	@param distance	The distance in metres.
 */
INLINE void EntityPicker::autoAimAdhesionFalloffDistance( float distance )
{
	autoAimAdhesionFalloffDistance_ = distance;
}


/**
 *	This method returns the auto aim distance.
 */
INLINE float EntityPicker::autoAimAdhesionFalloffDistance() const
{
	return autoAimAdhesionFalloffDistance_;
}


/**
 *	This method returns the current target. If the current entity is not a full
 *	target, NULL is returned.
 */
INLINE Entity * EntityPicker::pTarget() const
{
	return (isFull_ && !isHidden_) ? pTarget_ : NULL;
}


/**
 *	This method return the current target. It may not be a full target but just
 *	the thing that a reticle is drawn around.
 */
INLINE Entity * EntityPicker::pGeneralTarget() const
{
	return pTarget_;
}


/**
 *	This method returns whether or not the picker should attempt to hold the
 *	current target. While this value is true, no new target is picked and the
 *	current target is kept while it is within the deselection frustrum and the
 *	target is not hidden for too long.
 */
INLINE
bool EntityPicker::isHeld() const
{
	return isHeld_;
}


/**
 *	This method sets whether or not the picker should attempt to hold the
 *	current target. While this value is true, no new target is picked and the
 *	current target is kept while it is within the deselection frustrum and the
 *	target is not hidden for too long.
 */
INLINE
void EntityPicker::isHeld( bool b )
{
	isHeld_ = b;
}


// entity_picker.ipp
