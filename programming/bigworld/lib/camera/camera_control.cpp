#include "pch.hpp"
#include "camera_control.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Camera", 0 )


BW_BEGIN_NAMESPACE

bool CameraControl::isControlledByMouse_ = false;

float	CameraControl::strafeRate_ = 1;

Matrix CameraControl::deltaMatrix_;

float CameraControl::yawVelocity_ = 0.0f;
float CameraControl::pitchVelocity_ = 0.0f;
float CameraControl::orientationDamper_ = 1.0f;

Vector3 CameraControl::velocity_( 0.0f, 0.0f, 0.0f );
Vector3 CameraControl::nudge_( 0.0f, 0.0f, 0.0f );

float CameraControl::cameraMass_ = 5.0f;

float CameraControl::strafeRateVel_ = 0.f;
float CameraControl::xNudge_ = 0.f;
float CameraControl::yNudge_ = 0.f;
float CameraControl::zNudge_ = 0.f;

float CameraControl::xVel_ = 0.f;
float CameraControl::yVel_ = 0.f;
float CameraControl::zVel_ = 0.f;


/**
 *	Initialisation function which adds debug info
 */
void CameraControl::initDebugInfo()
{
	BW_GUARD;
	MF_WATCH( "Client Settings/Strafe Rate",	strafeRate_, Watcher::WT_READ_WRITE, "Base speed for cameras that use keyboard movement (e.g. the free camera)" );
	MF_WATCH( "Client Settings/Mouse Inverted",
		DirectionCursor::instance(),
		MF_ACCESSORS( bool, DirectionCursor, invertVerticalMovement ),
		"Inverts the y-axis of the mouse." );
	MF_WATCH( "Client Settings/Camera Mass",	cameraMass_, Watcher::WT_READ_WRITE, "Mass of the camera.  Affects momentum of camera movement." );
	MF_WATCH( "Client Settings/Orientation Damper",	orientationDamper_, Watcher::WT_READ_WRITE, "Akin to inertial tensor of the camera.  Affects camera's angular momentum." );

}


/**
 *	Key input handler
 */
bool CameraControl::handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;
	bool isDown = event.isKeyDown();

	bool handled = true;

	switch (event.key())
	{
	case KeyCode::KEY_VOLUMEUP:
		strafeRateVel_ = isDown ? 0.1f : 0;
		break;
	case KeyCode::KEY_VOLUMEDOWN:
		strafeRateVel_ = isDown ? -0.1f : 0;
		break;

	case KeyCode::KEY_UPARROW:
		zVel_ = isDown ? -1.f : 0.f;
		break;
	case KeyCode::KEY_DOWNARROW:
		zVel_ = isDown ? 1.f : 0.f;
		break;

	case KeyCode::KEY_PGUP:
		yVel_ = isDown ? -1.f : 0.f;
		break;
	case KeyCode::KEY_PGDN:
		yVel_ = isDown ? 1.f : 0.f;
		break;

	case KeyCode::KEY_RIGHTARROW:
		xVel_ = isDown ? -1.f : 0.f;
		break;
	case KeyCode::KEY_LEFTARROW:
		xVel_ = isDown ? 1.f : 0.f;
		break;

	case KeyCode::KEY_JOYALUP		:
	case KeyCode::KEY_JOYALDOWN	:
	case KeyCode::KEY_JOYALLEFT	:
	case KeyCode::KEY_JOYALRIGHT	:
	case KeyCode::KEY_JOYARUP		:	
	case KeyCode::KEY_JOYARDOWN	:
	case KeyCode::KEY_JOYARLEFT	:
	case KeyCode::KEY_JOYARRIGHT	:
		handled = true;
		break;

	default:
		handled = false;
	}

	return handled;
}


/**
 *	This method clears any velocity the camera may have acquired
 */
void CameraControl::clearVel()
{
	xVel_ = yVel_ = zVel_ = 0.f;
}


/**
 *	This function builds up a matrix describing the movement of the
 *	camera over dTime seconds.
 */
Matrix& CameraControl::calculateDeltaMatrix( float dTime )
{
	BW_GUARD;
	Matrix	m;

	deltaMatrix_.setIdentity();

	strafeRate_ += strafeRateVel_;

	if (0)	// turning left
	{
		m.setRotateY( DEG_TO_RAD(60.f*dTime));
		deltaMatrix_.postMultiply(m);
	}

	if (0)	// turning right
	{
		m.setRotateY( -DEG_TO_RAD(60.f*dTime));
		deltaMatrix_.postMultiply(m);
	}

	if (0)	// rolling left
	{
		m.setRotateZ( -DEG_TO_RAD(60.f*dTime));
		deltaMatrix_.postMultiply(m);
	}

	if (0)	// rolling right
	{
		m.setRotateZ( DEG_TO_RAD(60.f*dTime));
		deltaMatrix_.postMultiply(m);
	}

	Vector3 newNudge( xNudge_ + xVel_, yNudge_ + yVel_, zNudge_ + zVel_ );
	newNudge *= strafeRate_;
	CameraControl::nudge( newNudge );


	// Update nudge and velocity.
	float halfLife = cameraMass() * 0.1f;
	if ( halfLife > 0.0 )
	{
		float x = nudge().x == 0.0f ?
			Math::decay( velocity().x, nudge().x, halfLife * 0.3f, dTime ) :
			Math::decay( velocity().x, nudge().x, halfLife, dTime );
		float y = nudge().y == 0.0f ?
			Math::decay( velocity().y, nudge().y, halfLife * 0.3f, dTime ) :
			Math::decay( velocity().y, nudge().y, halfLife, dTime );
		float z = nudge().z == 0.0f ?
			Math::decay( velocity().z, nudge().z, halfLife * 0.3f, dTime ) :
			Math::decay( velocity().z, nudge().z, halfLife, dTime );
		Vector3 newVelocity( x, y, z );
		CameraControl::velocity( newVelocity );
	}
	else
	{
		CameraControl::velocity( nudge() );
	}

	// Update position.
	m.setTranslate( velocity() * dTime );
	deltaMatrix_.postMultiply( m );

	return deltaMatrix_;
}

BW_END_NAMESPACE

// camera_control.cpp
