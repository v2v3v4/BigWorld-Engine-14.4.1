#ifndef CAMERA_CONTROL_HPP
#define CAMERA_CONTROL_HPP

#include "moo/moo_math.hpp"

#include "direction_cursor.hpp"


BW_BEGIN_NAMESPACE

class KeyEvent;


// -----------------------------------------------------------------------------
// Section: CameraControl
// -----------------------------------------------------------------------------

/**
 *	This class is a helper class to abstract what the controls for the camera
 *	are.
 */
class CameraControl
{
public:
	static Matrix & calculateDeltaMatrix( float dTime );

	static bool handleKeyEvent( const KeyEvent & event );

	/// @todo Comment
	static bool isControlledByMouse()
		{ return isControlledByMouse_; }
	/// @todo Comment
	static void isControlledByMouse( bool value )
		{ isControlledByMouse_ = value; }

	/// @todo Comment
	static bool isMouseInverted()
	{
		return DirectionCursor::instance().invertVerticalMovement();
	}
	/// @todo Comment
	static void isMouseInverted( bool value )
	{
		DirectionCursor::instance().invertVerticalMovement( value );
	}

	/// @todo Comment
	static float strafeRate()					{ return strafeRate_; }
	/// @todo Comment
	static void strafeRate( float value )		{ strafeRate_ = value; }

	static void initDebugInfo();

	///	@name Free-Camera Behaviour.
	//@{
	static float yawVelocity()					{ return yawVelocity_; }
	static void yawVelocity( float value )		{ yawVelocity_ = value; }

	static float pitchVelocity()				{ return pitchVelocity_; }
	static void pitchVelocity( float value )	{ pitchVelocity_ = value; }

	static const Vector3 &velocity()		{ return velocity_; }
	static void velocity( const Vector3 &value )
		{ velocity_ = value; }

	static float orientationDamper()			{ return orientationDamper_; }

	static const Vector3 &nudge()			{ return nudge_; }
	static void nudge( const Vector3 &value )
		{ nudge_ = value; }

	static Vector3 axisNudge()
		{ return Vector3( xNudge_, yNudge_, zNudge_ ); }
	static void axisNudge( const Vector3 &value )
		{ xNudge_ = value.x; yNudge_ = value.y; zNudge_ = value.z; }

	static float cameraMass()					{ return cameraMass_; }
	static void cameraMass( float value )		{ cameraMass_ = value; }

	static void clearVel();
	//@}


private:	
	static bool isControlledByMouse_;

	static float strafeRate_;

	static Matrix	deltaMatrix_;

	static float yawVelocity_;
	static float pitchVelocity_;
	static float orientationDamper_;

	static Vector3 velocity_;
	static Vector3 nudge_;

	static float strafeRateVel_;
	static float xNudge_;
	static float yNudge_;
	static float zNudge_;

	static float xVel_;
	static float yVel_;
	static float zVel_;


	static float cameraMass_;
};

BW_END_NAMESPACE


#endif // CAMERA_CONTROL_HPP
