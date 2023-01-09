// camera.ipp

#ifdef CODE_INLINE
    #define INLINE    inline
#else
    #define INLINE
#endif

namespace Moo
{


INLINE Camera::Camera( float nP, float fP, float f, float aR ) :
	nearPlane_( nP ),
	farPlane_( fP ),
	fov_( f ),
	aspectRatio_( aR ),
	viewHeight_( 100 ),
	ortho_( false )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( nP < fP )
	{
		MF_EXIT( "near plane must be closer than far plane" );
	}
	
	IF_NOT_MF_ASSERT_DEV( nP > 0 )
	{
		MF_EXIT( "near plane must be greater than 0" );
	}
	
	IF_NOT_MF_ASSERT_DEV( f > 0 && f < MATH_PI )
	{
		MF_EXIT( "fov must be between 0 and PI" );
	}
	
	IF_NOT_MF_ASSERT_DEV( aR != 0 )
	{
		MF_EXIT( "aspect ratio cannot be 0" );
	}
}

INLINE Camera::Camera( const Camera& camera)
{
	*this=camera;
}

INLINE Camera& Camera::operator =( const Camera& camera )
{
	nearPlane_=camera.nearPlane_;
	farPlane_=camera.farPlane_;
	fov_=camera.fov_;
	aspectRatio_=camera.aspectRatio_;
	viewHeight_=camera.viewHeight_;
	ortho_=camera.ortho_;
	return *this;
}

INLINE Camera::~Camera()
{
}


INLINE float Camera::nearPlane() const
{
	BW_GUARD;
	MF_ASSERT_DEV(nearPlane_<farPlane_);
	return nearPlane_;
}

INLINE void Camera::nearPlane( float f )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV(f>0)
	{
		MF_EXIT( "near plane must be greater than 0" );
	}
	nearPlane_=f;
}


INLINE float Camera::farPlane() const
{
	BW_GUARD;
	MF_ASSERT_DEV(nearPlane_<farPlane_);
	return farPlane_;
}

INLINE void Camera::farPlane( float f )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV(f>0)
	{
		MF_EXIT( "far plane must be greater than 0" );
	}
	
	farPlane_=f;
}


INLINE float Camera::fov() const
{
	return fov_;
}

INLINE void Camera::fov( float f )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV(f>0&&f<MATH_PI)
	{
		MF_EXIT( "fov must be between 0 and PI" );
	}
	
	fov_=f;
}


INLINE float Camera::aspectRatio() const
{
	return aspectRatio_;
}

INLINE void Camera::aspectRatio( float f )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV(f!=0)
	{
		MF_EXIT( "aspect ratio cannot be 0" );
	}
	
	aspectRatio_=f;
}


INLINE float Camera::viewHeight() const
{
	return viewHeight_;
}

INLINE void Camera::viewHeight( float height )
{
	viewHeight_ = height;
}


INLINE float Camera::viewWidth() const
{
	return aspectRatio_ * viewHeight_;
}


INLINE bool Camera::ortho() const
{
	return ortho_;
}

INLINE void Camera::ortho( bool b )
{
	ortho_ = b;
}


} // namespace Moo

// camera.ipp