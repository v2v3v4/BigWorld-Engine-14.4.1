#include "pch.hpp"
#include "worldeditor/misc/world_editor_camera.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "appmgr/options.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "common/mouse_look_camera.hpp"
#include "common/orthographic_camera.hpp"
#include "resmgr/datasection.hpp"
#include "gizmo/undoredo.hpp"

BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( WorldEditorCamera )


WorldEditorCamera::WorldEditorCamera()
{
	BW_GUARD;

	{
		MouseLookCamera * mlc = new MouseLookCamera;
		mlc->windowHandle( WorldManager::instance().hwndGraphics() );
		
		// Inform camera of speed we want it to go
		BW::string speedName = Options::getOptionString( "camera/speed" );
		mlc->speed( Options::getOptionFloat( "camera/speed/" + speedName, 60.0f ) );
		mlc->turboSpeed( Options::getOptionFloat( "camera/speed/" + speedName + "/turbo", 120.0f ) );

		// Calculate the max bounds for the camera
		BoundingBox bounds = ChunkManager::instance().cameraSpace()->gridBounds();

		// bring the bounds in by 0.5m, otherwise being at the edge but looking
		// at the wrong angle will cause everything to disappear
		Vector3 minBound = bounds.minBounds() + Vector3( .5f, 0.f, .5f );
		Vector3 maxBound = bounds.maxBounds() - Vector3( .5f, 0.f, .5f );

		mlc->limit( BoundingBox( minBound, maxBound ) );

		cameras_.push_back(mlc);
		Py_DecRef(mlc);
	}


	{
		OrthographicCamera * mlc = new OrthographicCamera;
		mlc->windowHandle( WorldManager::instance().hwndGraphics() );
		
		// Inform camera of speed we want it to go
		BW::string speedName = Options::getOptionString( "camera/speed" );
		mlc->speed( Options::getOptionFloat( "camera/speed/" + speedName, 60.0f ) );
		mlc->turboSpeed( Options::getOptionFloat( "camera/speed/" + speedName + "/turbo", 120.0f ) );

		// Calculate the max bounds for the camera
		BoundingBox bounds = ChunkManager::instance().cameraSpace()->gridBounds();

		// bring the bounds in by 0.5m, otherwise being at the edge but looking
		// at the wrong angle will cause everything to disappear
		Vector3 minBound = bounds.minBounds() + Vector3( .5f, 0.f, .5f );
		Vector3 maxBound = bounds.maxBounds() - Vector3( .5f, 0.f, .5f );

		mlc->limit( BoundingBox( minBound, maxBound ) );

		cameras_.push_back(mlc);
		Py_DecRef(mlc);
	}

	currentCameraType_ = CT_MouseLook;
}


BaseCamera & WorldEditorCamera::currentCamera() 
{
	BW_GUARD;

	return *cameras_[currentCameraType_]; 
}


BaseCamera const & WorldEditorCamera::currentCamera() const 
{ 
	BW_GUARD;

	return *cameras_[currentCameraType_]; 
}


BaseCamera & WorldEditorCamera::camera( CameraType type ) 
{ 
	BW_GUARD;

	return *cameras_[type]; 
}


void WorldEditorCamera::changeToCamera( CameraType type )
{
	BW_GUARD;

	if (currentCameraType_ == type)
		return;

	Matrix m = cameras_[currentCameraType_]->view();
	m.invert();
	Vector3 currentPosition(m.applyToOrigin());
	
	currentCameraType_ = type;

	if (type == CT_Orthographic)
	{
		WorldManager::instance().setPlayerPreviewMode( false );
		Vector3 currentGroundPosition(Snap::toGround(currentPosition));
		currentPosition.y = currentGroundPosition.y + 50.f;
		Matrix newM;
		newM.lookAt( currentPosition,
					Vector3(0.f, -1.f, 0.f),
					Vector3(0.f, 0.f, 1.f));
		//newM.invert();
		cameras_[CT_Orthographic]->view(newM);
	}
	else if (type == CT_MouseLook)
	{
		Matrix newM = cameras_[CT_MouseLook]->view();
		newM.invert();
		newM[3].x = currentPosition.x;
		newM[3].z = currentPosition.z;
		newM.invert();
		cameras_[CT_MouseLook]->view(newM);
	}
}


void WorldEditorCamera::zoomExtents( ChunkItemPtr item, float sizeFactor )
{
	BW_GUARD;

	// calculate the world-space center of the item based on
	// the center of the local min/max bounding box metrics
	BoundingBox bb;
	item->edBounds( bb );

	// TODO: Shouldn't transform the BB twice, need to improve this code.
	// apply rotation and scale to the bounding box.
	Matrix m = item->edTransform();
	m.row( 3, Vector4( 0.0f, 0.0f, 0.0f, 1.0f ) );
	bb.transformBy( m );

	const Vector3 max = bb.maxBounds( );
	const Vector3 min = bb.minBounds( );
	const Vector3 diff = max - min;
	const Vector3 maxMinusMinOnTwo = diff / 2.f;

	if ( almostZero( maxMinusMinOnTwo.length(), 0.0001f ) )
	{
		//abort before we do something silly like ...
		//setting the camera matrix to zero!
		return;
	}

	ChunkPtr chunk = item->chunk();
	MF_ASSERT( chunk );
	const Vector3 worldPosition = chunk->transform().applyPoint(
										item->edTransform().applyToOrigin() );
	const Vector3 bbCenter = min + maxMinusMinOnTwo;
	const Vector3 itemCenter = worldPosition + bbCenter;

	// use the bounding box plan area as the camera's relative position
	Vector3 relativePosition = diff;
	relativePosition.y = 0.f;
	if ( almostZero( relativePosition.length(), 0.0001f ))
		relativePosition.z = 1.f;
	relativePosition.normalise( );

	//Calculate the radius of the bounding box, as seen by the camera
	bb.transformBy( Moo::rc().invView() );
	float radX = ( bb.maxBounds( )[0] - bb.minBounds( )[0] );
	float radY = ( bb.maxBounds( )[1] - bb.minBounds( )[1] );
	float radius = ( radX > radY) ? ( radX / 2.f ) : ( radY / 2.f );

	// calculate the distance the camera must be from the object
	// and set the new camera position
	const float FOV = Moo::rc().camera().fov();
	const float dist = float(radius / tan( FOV/2.f ));
	// should be doing something intelligent here
	float actualSizeFactor = std::max( sizeFactor - radius / 8.f, 1.f);
	Vector3 newPosition = relativePosition * dist * actualSizeFactor;
	newPosition += itemCenter;
	Matrix xform = currentCamera().view();
	xform.setTranslate( -newPosition.x, -newPosition.y, -newPosition.z );
	currentCamera().view(xform);

	// lookAt the object
	Vector3 lookDirection( -relativePosition );
	xform.lookAt( getCameraLocation(), lookDirection, Vector3( 0, 1, 0 ) );
	currentCamera().view(xform);
}


Vector3 WorldEditorCamera::getCameraLocation() const
{
	BW_GUARD;

    Matrix invView( currentCamera().view() );
	invView.invert();
    return invView.applyToOrigin();
}


void WorldEditorCamera::respace( const Matrix& view )
{
	BW_GUARD;

	// Calculate the max bounds for the camera
	BoundingBox bounds = ChunkManager::instance().cameraSpace()->gridBounds();

	// bring the bounds in by 0.5m, otherwise being at the edge but looking	
	// at the wrong angle will cause everything to disappear
	Vector3 minBound = bounds.minBounds() + Vector3( .5f, 0.f, .5f );
	Vector3 maxBound = bounds.maxBounds() - Vector3( .5f, 0.f, .5f );

	( (MouseLookCamera*)cameras_[ CT_MouseLook ].getObject() )
		->limit( BoundingBox( minBound, maxBound ) );
	( (OrthographicCamera*)cameras_[ CT_Orthographic ].getObject() )
		->limit( BoundingBox( minBound, maxBound ) );
	( (MouseLookCamera*)cameras_[ CT_MouseLook ].getObject() )->view( view );
	( (OrthographicCamera*)cameras_[ CT_Orthographic ].getObject() )->view( view );
}
BW_END_NAMESPACE

