
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif



/**
 * retrieve the action queue for this model
 *
 * @return the action queue for this model
 */
INLINE ActionQueue&
PyModel::actionQueue( void )
{
	return actionQueue_;
}




/**
 * retrieve the info for this model
 *
 * @return the modelInfo object for this model
 */
/*
INLINE const ModelInfo&
PyModel::modelInfo( void ) const
{
	return modelInfo_;
}
*/

/// retrieves the supermodel for this model
INLINE SuperModel * PyModel::pSuperModel() const
{
	return pSuperModel_;
}

/// sets the offset to apply to the root node of the scene unit when drawn
INLINE void PyModel::unitTransform( const Matrix& transform )
{
	unitTransform_ = transform;
}

/// sets the offset to apply to the root node of the scene unit when drawn
INLINE void PyModel::unitOffset( const Vector3& offset )
{
	unitOffset_ = offset;
	transformModsDirty_ = true;
}

/// sets the rotation to apply to the root node of the scene unit when drawn
INLINE void PyModel::unitRotation( float yaw )
{
	unitRotation_ = yaw;
	transformModsDirty_ = true;
}

/// gets the rotation to apply to the root node of the scene unit when drawn.
INLINE float PyModel::unitRotation( void )
{
	return unitRotation_;
}



/*model.ipp*/
