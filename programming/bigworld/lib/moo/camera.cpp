#include "pch.hpp"

#include "camera.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
    #include "camera.ipp"
#endif

namespace Moo
{

/**
 *	Finds the point on the near plane in camera space,
 *	given x and y in clip space.
 */
Vector3 Camera::nearPlanePoint( float xClip, float yClip ) const
{
	const float yLength = nearPlane_ * tanf( fov_ * 0.5f );
	const float xLength = yLength * aspectRatio_;

	return Vector3( xLength * xClip, yLength * yClip, nearPlane_ );
}

Vector3 Camera::farPlanePoint( float xClip, float yClip ) const
{
	const float yLength = farPlane_ * tanf( fov_ * 0.5f );
	const float xLength = yLength * aspectRatio_;

	return Vector3( xLength * xClip, yLength * yClip, farPlane_ );
}

} // namespace Moo

BW_END_NAMESPACE

// camera.cpp
