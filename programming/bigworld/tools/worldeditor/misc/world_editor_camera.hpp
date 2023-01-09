#ifndef WORLD_EDITOR_CAMERA_HPP
#define WORLD_EDITOR_CAMERA_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "common/base_camera.hpp"
#include "cstdmf/singleton.hpp"

BW_BEGIN_NAMESPACE

class WorldEditorCamera :
			public Singleton< WorldEditorCamera >, public ReferenceCount
{
public:
	WorldEditorCamera();
	
	enum CameraType
	{
		CT_MouseLook	= 0,
		CT_Orthographic = 1
	};

	BaseCamera & currentCamera();
	BaseCamera const & currentCamera() const;
	BaseCamera & camera( CameraType type );

	void changeToCamera( CameraType type );

	void zoomExtents( ChunkItemPtr item, float sizeFactor = 1.2 );

	void respace( const Matrix& view );

private:
	Vector3 getCameraLocation() const;

private:
	typedef SmartPointer<BaseCamera> BaseCameraPtr;

	BW::vector<BaseCameraPtr>				cameras_;
	CameraType								currentCameraType_;
};

BW_END_NAMESPACE

#endif // WORLD_EDITOR_CAMERA_HPP
