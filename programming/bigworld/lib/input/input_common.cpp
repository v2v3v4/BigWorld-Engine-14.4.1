#include "pch.hpp"

#include "input.hpp"

#include "math/mathdef.hpp"


BW_BEGIN_NAMESPACE

/**
 *	@file	This file contains input manager code common to all
 *	supported platforms.
 */


// -----------------------------------------------------------------------------
// Section: Joystick
// -----------------------------------------------------------------------------


/**
 *	The constructor for Joystick.
 */
Joystick::Joystick() :
	hWnd_( NULL ),
	pDIJoystick_( NULL ),
	axis_( AxisEvent::NUM_AXES ),
	lastProcessedTime_( 0 )
{
	// start quantised direction at the centre
	quantJoyDir_[0] = 4;
	quantJoyDir_[1] = 4;
}



/**
 *	This method updates the state of the Joystick.
 */
bool Joystick::update()
{
	BW_GUARD;
	bool updated = false;

	if (hasJoystick())
	{
		updated = this->updateFromJoystickDevice();
	}

	return updated;
}


/**
 *	This method generates a key event on behalf of the joystick
 */
void Joystick::generateKeyEvent( bool isDown, int key, InputHandler & handler,
	KeyStates& keyStates, const Vector2& cursorPosition )
{
	BW_GUARD;
	KeyEvent event;
	event.fill( KeyCode::Key( key ), isDown, InputDevices::modifiers(), cursorPosition );
	keyStates.setKeyStateNoRepeat( event.key(), event.isKeyDown() );
	handler.handleKeyEvent( event );
}



/**
 *	Helper function to get a direction from a joystick position
 */
static int joystickDirection( float joy_x, float joy_y, float box )
{
	BW_GUARD;
	if (joy_x*joy_x + joy_y*joy_y < box*box) return 4;
	float a = atan2f( joy_y, joy_x );

	const int dirMap[] = { 5, 8, 7, 6, 3, 0, 1, 2 };
	return dirMap[ uint(a * 4.f / MATH_PI + 8.5f) & 7 ];
	//int xd = (joy.x <= -box) ? -1 : (joy.x < box) ? 0 : 1;
	//int yd = (joy.y <= -box) ? -1 : (joy.y < box) ? 0 : 1;
	//return (yd+1)*3 + (xd+1);
}

static const float JOY_SEL_AMT = 0.5f;


/**
 *	This function gets the joystick to generate its events from its
 *	internal data.
 */
void Joystick::generateCommonEvents( InputHandler & handler, KeyStates& keyStates,
									 const Vector2& cursorPosition )
{
	BW_GUARD;
	// Figure out how times have changed
	uint64 curProcessedTime = timestamp();
	float dTime = 0.f;
	if (lastProcessedTime_)
	{
		dTime = float( int64(curProcessedTime - lastProcessedTime_) /
			stampsPerSecondD() );
		if (dTime > 1.f) dTime = 1.f;
	}
	lastProcessedTime_ = curProcessedTime;

	// Now send out all the axis events (our update method was just called)

	// first update our quantised directions
	int oldJoyDir[2];
	for (int i = 0; i < 2; i++)
	{
		oldJoyDir[i] = quantJoyDir_[i];

		AxisEvent::Axis horA = i ? AxisEvent::AXIS_RX : AxisEvent::AXIS_LX;
		AxisEvent::Axis verA = i ? AxisEvent::AXIS_RY : AxisEvent::AXIS_LY;

		quantJoyDir_[i] = joystickDirection(
			axis_[horA].value(), axis_[verA].value(),
			JOY_SEL_AMT );
	}

	// now send the axis events
	for (int a = AxisEvent::AXIS_LX; a < AxisEvent::NUM_AXES; a++)
	{
		if (axis_[a].value() != 0.f || !axis_[a].sentZero())
		{
			AxisEvent event;
			event.fill( (AxisEvent::Axis)a, axis_[a].value(), dTime );

			handler.handleAxisEvent( event );
			axis_[a].sentZero( axis_[a].value() == 0.f );
		}
	}

	// and then send the direction key events
	for (int i = 0; i < 2; i++)
	{
		int nDir, oDir;

		int keyEvBase = i ? KeyCode::KEY_JOYARUP : KeyCode::KEY_JOYALUP;

		// do they differ in x?
		if ((nDir = quantJoyDir_[i] % 3) != (oDir = oldJoyDir[i] % 3))
		{
			if (oDir != 1) this->generateKeyEvent(
				false, keyEvBase + (oDir ? 3 : 2), handler, keyStates, cursorPosition );
			if (nDir != 1) this->generateKeyEvent(
				true , keyEvBase + (nDir ? 3 : 2), handler, keyStates, cursorPosition );
		}

		// do they differ in y?
		if ((nDir = quantJoyDir_[i] / 3) != (oDir = oldJoyDir[i] / 3))
		{
			if (oDir != 1) this->generateKeyEvent(
				false, keyEvBase + (oDir ? 1 : 0), handler, keyStates, cursorPosition );
			if (nDir != 1) this->generateKeyEvent(
				true , keyEvBase + (nDir ? 1 : 0), handler, keyStates, cursorPosition );
		}
	}
}

BW::vector<KeyboardDevice*> gVirtualKeyboards;

BW_END_NAMESPACE

// input_common.cpp
