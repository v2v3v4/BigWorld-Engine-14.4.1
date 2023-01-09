#include "pch.hpp"
#include "camera_planes_setter.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This constructor saves the current values for the near and far planes
 *  of Moo's current camera.
 */
CameraPlanesSetter::CameraPlanesSetter()
{
	BW_GUARD;
	const Moo::Camera & camera = Moo::rc().camera();
	savedNearPlane_ = camera.nearPlane();
	savedFarPlane_ = camera.farPlane();
}


/**
 *	This constructor saves the current values for the near and far planes
 *  of Moo's current camera and sets the new values
 *
 *	@param	nearPlane		new near plane
 *	@param	farPlane		new far plane
 */
CameraPlanesSetter::CameraPlanesSetter( float nearPlane, float farPlane )
{
	BW_GUARD;
	Moo::Camera camera = Moo::rc().camera();
	savedNearPlane_ = camera.nearPlane();
	savedFarPlane_ = camera.farPlane();
	camera.nearPlane( nearPlane );
	camera.farPlane( farPlane );
	Moo::rc().camera( camera );
	Moo::rc().updateProjectionMatrix();
	Moo::rc().updateViewTransforms();
}


/**
 *	This destructor automatically resets the saved near and far plane values.
 */
CameraPlanesSetter::~CameraPlanesSetter()
{
	BW_GUARD;
	Moo::rc().camera().nearPlane( savedNearPlane_ );
	Moo::rc().camera().farPlane( savedFarPlane_ );
	Moo::rc().updateProjectionMatrix();
	Moo::rc().updateViewTransforms();
}


}	//namespace Moo

BW_END_NAMESPACE

// camera_planes_setter.cpp
