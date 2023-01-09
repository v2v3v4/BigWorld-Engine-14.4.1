#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "math/vector3.hpp"
#include "math/mathdef.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This class contains camera settings used to construct the projection matrix
 */
class Camera
{
public:
    Camera( float nearPlane, float farPlane, float fov, float aspectRatio );
    Camera( const Camera& camera );
    ~Camera();

    Camera& operator =( const Camera& camera );

	float	nearPlane() const;
	void	nearPlane( float f );

	float	farPlane() const ;
	void	farPlane( float f );

	float	fov() const;
	void	fov( float f );

	float	aspectRatio() const;
	void	aspectRatio( float f );

	float	viewHeight() const;
	void	viewHeight( float height );

	float	viewWidth() const;

	bool	ortho() const;
	void	ortho( bool b );

	Vector3	nearPlanePoint( float xClip, float yClip ) const;
	Vector3 farPlanePoint( float xClip, float yClip ) const;

private:
	float	nearPlane_;
	float	farPlane_;
	float	fov_;
	float	aspectRatio_;
	float	viewHeight_;
	bool	ortho_;
};


} // namespace Moo

#ifdef CODE_INLINE
    #include "camera.ipp"
#endif

BW_END_NAMESPACE

#endif // CAMERA_HPP
