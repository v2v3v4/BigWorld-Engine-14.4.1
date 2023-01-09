#ifndef CAMERA_BLENDER_HPP
#define CAMERA_BLENDER_HPP

#include "base_camera.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This is a record of a camera and how much it affects the eventual
 *	world transform.
 */
struct CameraBlend
{
	explicit CameraBlend( BaseCameraPtr pCam ) :
		pCam_( pCam ), prop_( 0.f ) { }

	BaseCameraPtr	pCam_;
	float			prop_;
};

typedef BW::vector<CameraBlend> CameraBlends;


/**
 *	This is a vector of selected cameras, with routines to move a
 *	camera to prime position and decay the influence of others.
 */
class CameraBlender : public BaseCamera
{
public:
	CameraBlender( float maxAge = 0.5f );

	void add( BaseCameraPtr pCam );
	void select( int index );

	virtual void update( float dTime );

	CameraBlends & elts() { return elts_; }

private:
	CameraBlends	elts_;
	float			maxAge_;
};



BW_END_NAMESPACE

#endif // CAMERA_BLENDER_HPP
