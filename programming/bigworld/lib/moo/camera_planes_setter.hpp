#ifndef CAMERA_PLANES_SETTER_HPP
#define CAMERA_PLANES_SETTER_HPP

BW_BEGIN_NAMESPACE

namespace Moo
{
/**
 *	This class is a scoped camera near/far planes setter, allowing you to
 *	easily set these values on the device.
 *
 *	The previous settings are restored automatically when the
 *	class goes out of scope.
 */
	class CameraPlanesSetter
	{
	public:		
		CameraPlanesSetter();
		CameraPlanesSetter( float nearPlane, float farPlane );
		~CameraPlanesSetter();
	private:		
		float savedNearPlane_;
		float savedFarPlane_;
	};
};	//namespace Moo

BW_END_NAMESPACE

#endif	//CAMERA_PLANES_SETTER_HPP
