#include "pch.hpp"
#include "free_camera.hpp"

#include "camera_control.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: FreeCamera
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
FreeCamera::FreeCamera( PyTypeObject * pType ) :
	BaseCamera( pType ),
	invViewProvider_( NULL ),
	fixed_( false )
{
}


/**
 *	Destructor.
 */
FreeCamera::~FreeCamera()
{
}


/**
 *	Handles this key event
 */
bool FreeCamera::handleKeyEvent( const KeyEvent & ev )
{
	BW_GUARD;
	return CameraControl::handleKeyEvent( ev );
}


static const float MOUSE_H_SPEED = 100.0f;
static const float MOUSE_V_SPEED = 300.0f;
static const float MOUSE_W_SPEED = 150.0f;

/**
 *	Handles this mouse event
 */
bool FreeCamera::handleMouseEvent( const MouseEvent & event )
{
	BW_GUARD;
	// Calculate the angular change in the camera.
	float deltaYaw = event.dx() / MOUSE_H_SPEED;
	float deltaPitch = event.dy() / MOUSE_V_SPEED;

	// Invert mouse Y if needed.
	deltaPitch *= CameraControl::isMouseInverted() ? -1.0f : 1.0f;

	// Update camera angle velocities.
	CameraControl::yawVelocity( deltaYaw + CameraControl::yawVelocity() );
	CameraControl::pitchVelocity( deltaPitch +
		CameraControl::pitchVelocity() );

	return true;
}

/**
 *	Handles this axis event
 */
bool FreeCamera::handleAxisEvent( const AxisEvent & event )
{
	BW_GUARD;
	bool handled = false;

	if (event.axis() == AxisEvent::AXIS_RX)
	{
		float deltaYaw = event.value()/6;

		CameraControl::yawVelocity( deltaYaw +
			CameraControl::yawVelocity() );

		handled = true;
	}
	else if (event.axis() == AxisEvent::AXIS_RY)
	{
		float deltaPitch = -event.value()/10;

		// Invert mouse Y if needed.
		deltaPitch *= CameraControl::isMouseInverted() ? -1.0f : 1.0f;

		CameraControl::pitchVelocity( deltaPitch +
			CameraControl::pitchVelocity() );

		handled = true;
	}
	else if (event.axis() == AxisEvent::AXIS_LX)
	{
		Vector3 nudge = CameraControl::axisNudge();
		if (InputDevices::isKeyDown(KeyCode::KEY_JOYALPUSH))
			nudge.x -= event.value()*10*10;
		else
			nudge.x -= event.value()*10;
		CameraControl::axisNudge( nudge );
		handled = true;
	}
	else if (event.axis() == AxisEvent::AXIS_LY)
	{
		Vector3 nudge = CameraControl::axisNudge();
		if (InputDevices::isKeyDown(KeyCode::KEY_JOYALPUSH))
			nudge.z -= event.value()*10*10;
		else
			nudge.z -= event.value()*10;
		CameraControl::axisNudge( nudge );
		handled = true;
	}

	return true;
}


/**
 *	Updates the camera's position
 */
void FreeCamera::update( float dTime )
{
	BW_GUARD;
	if ( invViewProvider_ )
	{
		invViewProvider_->matrix( invView_ );
		view_.invert( invView_ );
	}
	else if ( !fixed_ )
	{
		view_.postMultiply(
			CameraControl::calculateDeltaMatrix( dTime ) );

		// Update the camera matrix.
		invView_.invert( view_ );
		Vector3 t = invView_.applyToOrigin();
		invView_.postRotateY( CameraControl::yawVelocity() * dTime * CameraControl::orientationDamper() );
		invView_.preRotateX( CameraControl::pitchVelocity() * dTime * CameraControl::orientationDamper() );
		invView_.translation( t );
		view_.invert( invView_ );

		// Update free-camera velocities.
		float halfLife = CameraControl::cameraMass() * 0.05f;
		if ( halfLife > 0.0 )
		{
			CameraControl::yawVelocity( Math::decay(
				CameraControl::yawVelocity(), 0.f, halfLife, dTime ) );
			CameraControl::pitchVelocity( Math::decay(
				CameraControl::pitchVelocity(), 0.f, halfLife, dTime ) );
		}
		else
		{
			CameraControl::yawVelocity( 0.0f );
			CameraControl::pitchVelocity( 0.0f );
		}

		// Clear stuff
		CameraControl::axisNudge( Vector3::zero() );
	}
}


/**
 *	Directly sets the camera to the given matrix
 */
void FreeCamera::set( const Matrix & viewMatrix )
{
	view_ = viewMatrix;
	invView_.invert( view_ );
	CameraControl::clearVel();
	CameraControl::yawVelocity( 0.0f );
	CameraControl::pitchVelocity( 0.0f );
}


// -----------------------------------------------------------------------------
// Section: Python stuff
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( FreeCamera )

PY_BEGIN_METHODS( FreeCamera )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( FreeCamera )
	/*~ attribute FreeCamera.fixed
	 *
	 *	If this attribute is set to non-zero then the camera remains fixed in
	 *	position, and does not move in response to the mouse and arrow keys.
	 *	This can be useful if there is more than one FreeCamera in use, and you
	 *	only want to move one at once.  It defaults to 0, which means it can be
	 *	moved around.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( fixed )
	/*~ attribute FreeCamera.invViewProvider
	 *
	 *	This attribute is the view matrix for the camera.  It is a 
	 *	MatrixProvider.
	 *	
	 *	@type	Read-Only MatrixProvider
	 */
	PY_ATTRIBUTE( invViewProvider )
PY_END_ATTRIBUTES()

/*~ function BigWorld.FreeCamera
 *
 *	This function creates a new FreeCamera.  This is a camera that is 
 *	controlled by the mouse and arrow keys, and moves around
 *	independently of entities and geometry, making it useful for debugging
 *	and exploring the world.
 *	
 *	@return		a new FreeCamera object.
 */
PY_FACTORY( FreeCamera, BigWorld )


/**
 *	Python factory method
 */
PyObject * FreeCamera::pyNew( PyObject * args )
{
	BW_GUARD;
	return new FreeCamera();
}

BW_END_NAMESPACE

// free_camera.cpp
