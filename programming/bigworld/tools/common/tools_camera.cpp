#include "pch.hpp"
#include "tools_camera.hpp"

#include "appmgr/options.hpp"
#include "math/boundbox.hpp"
#include "math/matrix.hpp"
#include "moo/render_context.hpp"
#include "mouse_look_camera.hpp"
#include "orbit_camera.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include <iostream>
#include <limits>

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

ToolsCamera * ToolsCamera::s_instance = NULL;

/**
 *	Constructor.
 */
ToolsCamera::ToolsCamera()
	: PyObjectPlus( &ToolsCamera::s_type_ )
	, cameraMode_(CameraMode_Free)
	, modeChange_(false)
	, cameraPositionX_(5.f, 1.f, 0.f)
	, cameraPositionY_(0.f, 5.f, 0.f)
	, cameraPositionZ_(0.f, 1.f, 5.f)
	, zoomAnim_(true)
	, zoomTime_(0.f)
	, minZoomIn_(0.1f)
    , maxZoomOut_(std::numeric_limits<float>::max())
{
	BW_GUARD;

	s_instance = this;

	lastView_.setIdentity();

	mouseLookCamera_ = MouseLookCameraPtr(new MouseLookCamera(), true);
	orbitCamera_ = OrbitCameraPtr(new OrbitCamera(), true);
	
	// set initial free camera matrix
	Vector3 initialPosition(-1.8f, .72f, 1.8f);
	Vector3 initialDirection(initialPosition);
	initialDirection.y -= .8f;
	initialDirection *= -1.f;
	initialDirection.normalise();
	cameraMatrixFree_.lookAt(initialPosition, initialDirection, Vector3(0.f, 1.f, 0.f));

	zoomDuration_ = Options::getOptionFloat( "settings/zoomDuration", 0.25f );
}


/**
 *	Destructor
 */
ToolsCamera::~ToolsCamera()
{
}

void ToolsCamera::update( float dTime /* = 0.f */, bool activeInputHandler /* = true */)
{
	BW_GUARD;

	BaseCameraPtr bc = (cameraMode_ != CameraMode_Orbit) 
		? (BaseCameraPtr)mouseLookCamera_
		: (BaseCameraPtr)orbitCamera_;
	
	if (zoomTime_ > 0.f)
	{
		zoomTime_ -= dTime;
		if (zoomTime_ < 0.f)
			zoomTime_ = 0.f;

		Vector3 pos =	(zoomDuration_ - zoomTime_) * centreFinish_ / zoomDuration_ +
						(zoomTime_) * centreStart_ / zoomDuration_;

		orbitCamera_->origin( pos );

		Matrix view = bc->view();
		view.invert();
		pos =	(zoomDuration_ - zoomTime_) * zoomFinish_ / zoomDuration_ +
				(zoomTime_) * zoomStart_ / zoomDuration_;
		view[3] = pos;
		view.invert();
		bc->view( view );
		return;
	}

	bc->update( dTime, activeInputHandler );

	// do some post processing, depending on the mode
	switch (cameraMode_)
	{
	case CameraMode_Free:
	default:
		{
			// set initial view direction
			if (modeChange_)
			{
				bc->view( cameraMatrixFree_ );
				modeChange_ = false;
			}

			cameraMatrixFree_ = bc->view();

            break;
		}
	case CameraMode_X:
		{
			Matrix view = bc->view();
			view.invert();
			Vector3 camPos( view.applyToOrigin() );

			if (modeChange_)
			{
				// set initial position
				camPos = cameraPositionX_;
				modeChange_ = false;
			}

			Matrix xform = bc->view();
			xform.setRotateY( -0.5f * MATH_PI );
			xform.postTranslateBy( camPos );
			xform.invert();
			bc->view(xform);
			
			cameraPositionX_ = camPos;

			break;
		}
	case CameraMode_Y:
		{
			Matrix view = bc->view();
			view.invert();
			Vector3 camPos( view.applyToOrigin() );

			if ( modeChange_ )
			{
				// set initial position
				camPos = cameraPositionY_;
				modeChange_ = false;
			}

			Matrix viewMat;
			viewMat.setRotateZ( MATH_PI );
			Matrix rotateMat;
			rotateMat.setRotateX( 0.5f * MATH_PI );
			Matrix xform = bc->view();
			xform.multiply(viewMat, rotateMat);
			xform.postTranslateBy( camPos );
			xform.invert();
			bc->view(xform);

			cameraPositionY_ = camPos;

			break;
		}
	case CameraMode_Z:
		{
			Matrix view = bc->view();
			view.invert();
			Vector3 camPos( view.applyToOrigin() );

			if ( modeChange_ )
			{
				// set initial position
				camPos = cameraPositionZ_;
				modeChange_ = false;
			}

			Matrix viewMat;
			viewMat.setRotateZ( MATH_PI );
			Matrix rotateMat;
			rotateMat.setRotateX( MATH_PI );
			Matrix xform = bc->view();
			xform.multiply(viewMat, rotateMat);
			xform.postTranslateBy( camPos );
			xform.invert();
			bc->view(xform);

			cameraPositionZ_ = camPos;

			break;
		}
	case CameraMode_Orbit:
		{
			// set initial view direction
			if (modeChange_)
			{
				orbitCamera_->view(lastView_);
				modeChange_ = false;
			}
		}
	}

	//Save the view for future reference
	lastView_ = bc->view();
}

bool ToolsCamera::handleKeyEvent( const KeyEvent & ke )
{
	BW_GUARD;

	bool handled = false;
	
	if (cameraMode_ != CameraMode_Orbit)
		handled = mouseLookCamera_->handleKeyEvent(ke);
	else
		handled = orbitCamera_->handleKeyEvent(ke);

	//Why not have this behaviour too...
	if ( InputDevices::isKeyDown( KeyCode::KEY_MIDDLEMOUSE ) )
		zoomToExtents( true );

	return handled;
}

bool ToolsCamera::handleMouseEvent( const MouseEvent & me )
{
	BW_GUARD;

	bool handled = false;
	
	BaseCameraPtr bc;
	
	if (cameraMode_ != CameraMode_Orbit)
	{
		handled = mouseLookCamera_->handleMouseEvent(me);
		if (handled)
		{
			//Break out of lock mode gracefully
			if (cameraMode_ != CameraMode_Free)
			{
				cameraMatrixFree_ = lastView_;
				mode( CameraMode_Free ); 
			}
		}
		bc = mouseLookCamera_;
	}
	else
	{
		handled = orbitCamera_->handleMouseEvent(me);
		bc = orbitCamera_;
	}

	if (me.dz())
	{
		static float minRot = 1024.f;
		if (minRot > (float)fabs((float)me.dz()))
			minRot = (float)fabs((float)me.dz());

		float move = powf(1.15f, me.dz()/minRot );

		Matrix view = bc->view();
		view.invert();

        Vector3 origDistOrigin = view.applyToOrigin();
		
		Vector3 forward = view.applyToUnitAxisVector( 2 );

		float distance = getLodDist();

		if (distance < 0.5f && me.dz() < 0)
		{
			distance = 0.5f;
		}

		view._41 += forward.x * (move - 1.f) * distance;
		view._42 += forward.y * (move - 1.f) * distance;
		view._43 += forward.z * (move - 1.f) * distance;
		view.invert();

        // Only move if we aren't too far from the origin (or too close)
        Vector3 distOrigin = view.applyToOrigin();
        if ((distOrigin.length() > minZoomIn_) &&
			(distOrigin.length() < maxZoomOut_))
        {
		    bc->view( view );
        }

		handled = true;

	}

	return handled;
}

void ToolsCamera::save()
{
	BW_GUARD;

	Matrix viewInverse = view();
	viewInverse.invert();

	//Save the settings...
	Options::setOptionVector3( "startup/lastOrigin", origin() );
	Options::setOptionMatrix34( "startup/lastView", viewInverse );
	Options::setOptionInt( "camera/mode", cameraMode_ );
	Options::setOptionInt( "camera/rotDir", orbitCamera_->rotDir() );
}

void ToolsCamera::boundingBox( const BoundingBox& bb, bool updateCentre /* = true */ )
{
	BW_GUARD;

	bb_ = bb;
	if (updateCentre)
	{
		Vector3 centre(
			(bb_.maxBounds().x + bb_.minBounds().x) / 2.f,
			(bb_.maxBounds().y + bb_.minBounds().y) / 2.f,
			(bb_.maxBounds().z + bb_.minBounds().z) / 2.f );
		orbitCamera_->origin( centre );
	}
}

/* 
 * The following method finds the lod distance in the same way as supermodel.
 * It uses the planar distance so it may appear a bit strange.
 */
float ToolsCamera::getLodDist()
{
	BW_GUARD;

	const Matrix & mooWorld = Moo::rc().world();
	const Matrix & mooView = Moo::rc().view();
	float realDist = mooWorld.applyToOrigin().dotProduct(
	Vector3( mooView._13, mooView._23, mooView._33 ) ) + mooView._43;
	float yscale = mooWorld.applyToUnitAxisVector(1).length();
	return (realDist / std::max( 1.f, yscale )) * Moo::rc().zoomFactor();
}

void ToolsCamera::zoomToExtents( bool animate, const BoundingBox & box /* = BoundingBox::s_insideOut_ */ )
{
	BW_GUARD;

	if (box != BoundingBox::s_insideOut_) bb_ = box;

	if (bb_ == BoundingBox::s_insideOut_) return;
		
	Vector3 bounds = bb_.maxBounds() - bb_.minBounds();

	float radius = bounds.length() / 2.f;
	
	if (radius < 0.01f) return;

	float dist = radius / tanf( Moo::rc().camera().fov() / 2.f );

	//Special case to avoid near plane clipping of small objects
	if (Moo::rc().camera().nearPlane()  > dist - radius)
		dist = Moo::rc().camera().nearPlane() + radius;
	
	BaseCameraPtr bc = (cameraMode_ != CameraMode_Orbit) 
		? (BaseCameraPtr)mouseLookCamera_
		: (BaseCameraPtr)orbitCamera_;

	Matrix view = bc->view();

	view.invert();

	Vector3 forward = view.applyToUnitAxisVector( 2 );

	zoomStart_ = view.applyToUnitAxisVector( 3 );

	view.invert();

	centreStart_ = orbitCamera_->origin();

	centreFinish_ = Vector3(
		(bb_.maxBounds().x + bb_.minBounds().x) / 2.f,
		(bb_.maxBounds().y + bb_.minBounds().y) / 2.f,
		(bb_.maxBounds().z + bb_.minBounds().z) / 2.f );

	zoomFinish_ = centreFinish_ - 1.05f * dist * forward;

	float zoomDist = (zoomFinish_-zoomStart_).length();
		
	if ((zoomAnim_) && (animate) && ( zoomDist > 0.1f ))
	{
		zoomTime_ = zoomDuration_;
	}
	else
	{
		orbitCamera_->origin( centreFinish_ );
		
		view.lookAt( zoomFinish_, forward, Vector3( 0.f, 1.f, 0.f ) );

		bc->view( view );
	
		//Ensure this new camera position is used when next rendering
		render();
	}
}

void ToolsCamera::render( float dt /* = 0.f */ )
{
	BW_GUARD;

	if (cameraMode_ != CameraMode_Orbit)
		mouseLookCamera_->render(dt);
	else
		orbitCamera_->render(dt);
}

void ToolsCamera::windowHandle( HWND hwnd )
{
	BW_GUARD;

	mouseLookCamera_->windowHandle(hwnd);
	orbitCamera_->windowHandle(hwnd);
}

void ToolsCamera::origin( const Vector3& val )
{
	BW_GUARD;

	orbitCamera_->origin(val);
}

void ToolsCamera::view( const Matrix& val )
{
	BW_GUARD;

	mouseLookCamera_->view(val);
	orbitCamera_->view(val);
}

float ToolsCamera::maxZoomOut() const
{
    return maxZoomOut_;
}

void ToolsCamera::maxZoomOut(float maxv)
{
	BW_GUARD;

    maxZoomOut_ = maxv;
    BoundingBox b(Vector3(-maxv, -maxv, -maxv), Vector3(maxv, maxv, maxv));
    mouseLookCamera_->limit(b);
}

float ToolsCamera::minZoomIn() const
{
    return minZoomIn_;
}

void ToolsCamera::minZoomIn(float minv)
{
    minZoomIn_ = minv;
}

const Vector3& ToolsCamera::origin() const
{
	BW_GUARD;

	return orbitCamera_->origin();
}

const Matrix& ToolsCamera::view() const
{
	BW_GUARD;

	if (cameraMode_ != CameraMode_Orbit)
		return mouseLookCamera_->view();
	else
		return orbitCamera_->view();
}

void ToolsCamera::invert( bool val )
{
	BW_GUARD;

	mouseLookCamera_->invert(val);
	orbitCamera_->invert(val);
}
void ToolsCamera::speed( float val )
{
	BW_GUARD;

	mouseLookCamera_->speed(val);
	orbitCamera_->speed(val);
}
void ToolsCamera::turboSpeed( float val )
{
	BW_GUARD;

	mouseLookCamera_->turboSpeed(val);
	orbitCamera_->turboSpeed(val);
}
void ToolsCamera::rotDir( int dir )
{
	BW_GUARD;

	orbitCamera_->rotDir( dir );
}
void ToolsCamera::orbitSpeed( float speed )
{
	BW_GUARD;

	orbitCamera_->orbitSpeed( speed );
}

bool ToolsCamera::invert()
{
	BW_GUARD;

	return mouseLookCamera_->invert();
}
float ToolsCamera::speed()
{
	BW_GUARD;

	return mouseLookCamera_->speed();
}
float ToolsCamera::turboSpeed()
{
	BW_GUARD;

	return mouseLookCamera_->turboSpeed();
}
int ToolsCamera::rotDir()
{
	BW_GUARD;

	return orbitCamera_->rotDir();
}
float ToolsCamera::orbitSpeed()
{
	BW_GUARD;

	return orbitCamera_->orbitSpeed();
}


//Lets set up all the python stuff now...


PY_TYPEOBJECT( ToolsCamera )

PY_BEGIN_METHODS( ToolsCamera )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ToolsCamera )
	PY_ATTRIBUTE( invert )
	PY_ATTRIBUTE( speed )
	PY_ATTRIBUTE( turboSpeed )
	PY_ATTRIBUTE( mode )
	PY_ATTRIBUTE( rotDir )
PY_END_ATTRIBUTES()


/*~	function ModelEditor.camera
 *	@components{ modeleditor }
 *
 *	This function gets ModelEditor's camera
 *
 *	@return ModelEditor's camera instance.
 */
static PyObject * py_camera( PyObject * args )
{
	BW_GUARD;

	Py_INCREF( &ToolsCamera::instance() );
	return &ToolsCamera::instance();
}
PY_MODULE_FUNCTION( camera, ModelEditor )
BW_END_NAMESPACE

