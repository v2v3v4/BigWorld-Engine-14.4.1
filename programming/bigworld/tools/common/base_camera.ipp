
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE

// INLINE void BaseCamera::inlineFunction()
// {
// }

/**
 *	This method returns the view matrix
 */
INLINE const Matrix & BaseCamera::view() const
{
	return view_;
}


/**
 *	This method sets the view matrix
 */
INLINE void BaseCamera::view( const Matrix & m )
{
	view_ = m;
}


/**
 *	This method returns the position of the camera
 */
INLINE Vector3 BaseCamera::position() const
{
	BW_GUARD;

	Matrix invView( view_ );
	invView.invert();
	return Vector3( invView[3][0], invView[3][1], invView[3][2] );
}


/**
 *	This method sets the position of the camera
 */
INLINE void BaseCamera::position( const Vector3 & pos )
{
	BW_GUARD;

	view_.invert();
	view_.translation( pos );
	view_.invert();
}


/**
 *	This method is a base class stub for handling key events
 */
INLINE bool BaseCamera::handleKeyEvent( const KeyEvent & )
{
 	return false;
}


/**
 *	This method is a base class stub for handling mouse events
 */
INLINE bool BaseCamera::handleMouseEvent( const MouseEvent & )
{
	return false;
}


/**
 *	This method returns the camera's speed
 */
INLINE float BaseCamera::speed() const
{
	return speed_[0];
}


/**
 *	This method sets the camera's speed
 */
INLINE void	BaseCamera::speed( float s )
{
	speed_[0] = s;
}


/**
 *	This method returns the camera's turbo speed
 */
INLINE float BaseCamera::turboSpeed() const
{
	return speed_[1];
}


/**
 *	This method sets the camera's speed
 */
INLINE void	BaseCamera::turboSpeed( float s )
{
	speed_[1] = s;
}


/**
 *	This method returns the camera's inversion
 */
INLINE bool BaseCamera::invert() const
{
	return invert_;
}


/**
 *	This method sets the camera's inversion
 */
INLINE void	BaseCamera::invert( bool s )
{
	invert_ = s;
}


/**
 *	This method returns the camera's extents it may move to
 */
INLINE const BoundingBox & BaseCamera::limit() const
{
	return limit_;
}


/**
 *	This method sets the camera's extents it may move to
 */
INLINE void	BaseCamera::limit( const BoundingBox & bb)
{
	limit_ = bb;
}


BW_END_NAMESPACE
/*base_camera.ipp*/
