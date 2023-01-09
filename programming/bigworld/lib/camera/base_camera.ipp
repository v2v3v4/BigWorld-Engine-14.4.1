// base_camera.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


/**
 *	This method returns the view matrix
 */
INLINE const Matrix & BaseCamera::view() const
{
	return view_;
}

/**
 *	This method returns the inverse view matrix
 */
INLINE const Matrix & BaseCamera::invView() const
{
	return invView_;
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
 *	This method is a base class stub for handling axis events
 */
INLINE bool BaseCamera::handleAxisEvent( const AxisEvent & )
{
	return false;
}


// base_camera.ipp
