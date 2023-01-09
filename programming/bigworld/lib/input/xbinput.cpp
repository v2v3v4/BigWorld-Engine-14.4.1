#include "pch.hpp"

#include <stdio.h>

#include <xtl.h>

#include "input.hpp"
#include "math/vector2.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/dogwatch.hpp"
#include "math/mathdef.hpp"
#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"


DECLARE_DEBUG_COMPONENT2( "UI", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "input.ipp"
#endif


// -----------------------------------------------------------------------------
// Section: XBInput
// -----------------------------------------------------------------------------


/**
 *	This class does xBox specific input stuff
 */
class XBInput
{
public:
	~XBInput();

	void update();

	const Vector2& leftStick() const { return leftStick_; };
	const Vector2& rightStick() const { return rightStick_; };

	const XINPUT_STATE& state() const { return state_; };

	void rumble(float leftMotor, float rightMotor);

	void rumbleEnabled( bool state )
	{
		if (!state)
			rumble( 0, 0 );
		rumbleEnabled_ = state;
	}

	bool rumbleEnabled() const
	{
		return rumbleEnabled_;
	}

	static XBInput& instance();
private:
	void openPad();
	void closePad();
	void probeDevices();

	XBInput();
	Vector2 leftStick_;
	Vector2 rightStick_;
	HANDLE	padHandle_;
	int		port_;
	XINPUT_STATE state_;
	XINPUT_FEEDBACK feedbackState_;

	bool	rumbleEnabled_;
	bool	rumbleWasBusy_;

	XBInput( const XBInput& );
	XBInput& operator=( const XBInput& );
};



const float deadZone = 0.24f;

static inline float stickValue( SHORT val )
{
	float ret = val;
	ret /= 32768.f;

	if (ret > 0)
	{
		ret = max( 0.f, ret - deadZone );
		ret *= 1.f / (1.f - deadZone);
	}
	else
	{
		ret = min( 0.f, ret + deadZone );
		ret *= 1.f / (1.f - deadZone);
	}
	return ret;
}

/**
 *	Constructor.
 */
XBInput::XBInput()
: leftStick_( 0, 0 ),
  rightStick_( 0, 0 ),
  padHandle_( NULL ),
  port_( -1 ),
  rumbleEnabled_( true ),
  rumbleWasBusy_( false )
{
	MF_WATCH( "Client Settings/rumble", *this,
		MF_ACCESSORS( bool, XBInput, rumbleEnabled) );
	ZeroMemory(&feedbackState_, sizeof(feedbackState_));
	probeDevices();
	if (port_ != -1)
		openPad();
}


/**
 *	Destructor.
 */
XBInput::~XBInput()
{
}

void XBInput::probeDevices()
{
	DWORD deviceMap = XGetDevices( XDEVICE_TYPE_GAMEPAD );

	for (int i = 0; i < 4; i++)
	{
		if (deviceMap & (1<<i))
		{
			port_ = i;
			break;
		}
	}
}

void XBInput::openPad()
{
	XINPUT_POLLING_PARAMETERS pp = { TRUE, FALSE, 0, 8, 8, 0 };
	padHandle_ = XInputOpen( XDEVICE_TYPE_GAMEPAD, port_, XDEVICE_NO_SLOT, &pp );
	if (!padHandle_)
		port_ = -1;

}

void XBInput::closePad()
{
	if (padHandle_)
	{
		XInputClose( padHandle_ );
		padHandle_ = NULL;
	}
	port_ = -1;

}

void XBInput::update()
{
	DWORD insert, remove;
	if (TRUE == XGetDeviceChanges( XDEVICE_TYPE_GAMEPAD, &insert, &remove ))
	{
		if (port_ != -1 && (remove & (1 << port_)))
		{
			closePad();
		}
	}

	if ( port_ != -1 && ERROR_SUCCESS == XInputGetState( padHandle_, &state_ ))
	{
		leftStick_.set( stickValue(state_.Gamepad.sThumbLX), stickValue(state_.Gamepad.sThumbLY) );
		rightStick_.set( stickValue(state_.Gamepad.sThumbRX), stickValue(state_.Gamepad.sThumbRY) );
	}
	else
	{
		ZeroMemory( &state_, sizeof(state_) );
		probeDevices();
		if (port_ != -1)
			openPad();

		leftStick_.set( 0, 0 );
		rightStick_.set( 0, 0 );
	}

	// if the device was busy when we wanted to turn off rumbling,
	// then keep trying to turn it off until we succeed
	if (rumbleWasBusy_)
	{
		//NOTICE_MSG( "XBInput::update: "
		//	"Rumble device was busy when last wanted to turn off, retrying\n" );
		this->rumble( 0.f, 0.f );
	}
}


XBInput& XBInput::instance()
{
	static XBInput input;
	return input;
}



void XBInput::rumble( float leftMotor, float rightMotor )
{
	if (!rumbleEnabled_ || !padHandle_) return;

	WORD leftWord = WORD(65535 * Math::clamp(0.0f, leftMotor, 1.0f));
	WORD rightWord = WORD(65535 * Math::clamp(0.0f, rightMotor, 1.0f));

	// honour this rumble request if we're not still busy with the last one
	if (feedbackState_.Header.dwStatus != ERROR_IO_PENDING)
	{
		rumbleWasBusy_ = false;
		feedbackState_.Rumble.wLeftMotorSpeed = leftWord;
		feedbackState_.Rumble.wRightMotorSpeed = rightWord;
		XInputSetState( padHandle_, &feedbackState_ );
	}
	// otherwise remember to retry if it was turning it off
	else if (leftWord == 0 && rightWord == 0)
	{
		//DEBUG_MSG( "XBInput::rumble: Device busy, will retry stop later\n" );
		rumbleWasBusy_ = true;
	}
	// presumably it does not matter if we miss a turn on request,
	// but if the rumble was left on for a long time that could be bad
	else
	{
		//DEBUG_MSG( "XBInput::rumble: Device busy, ignoring turn on request\n" );
	}
}



// -----------------------------------------------------------------------------
// Section: Data declaration
// -----------------------------------------------------------------------------

const int DIRECT_INPUT_AXIS_MAX = 32768;
const int DIRECT_INPUT_AXIS_DEAD_ZONE = 32768*24/100;

InputDevices InputDevices::instance_;
bool InputDevices::focus_;

const int InputDevices::EXCLUSIVE_MODE = 0x01;


// -----------------------------------------------------------------------------
// Section: InputDevices
// -----------------------------------------------------------------------------

static const int KEYBOARD_BUFFER_SIZE = 32;
static const int MOUSE_BUFFER_SIZE = 64;
static const int JOYSTICK_BUFFER_SIZE = 32;


//
// InputDevices::InputDevices:
//
InputDevices::InputDevices() : 
	hWnd_( NULL ),
	pDirectInput_( NULL ),
	languageChanged_( false ),
	processingEvents_( false ),
	consumeUpToSeqId_( 0 ),
	lastRawKeyMakeCode_(),
	lostData_( NO_DATA_LOST ),
	ignoringKey_( KeyCode::KEY_NONE )
{
	memset( isKeyDown_, 0, sizeof(isKeyDown_) );
	// TODO:PM We could initialise this with the correct state of the keyboard
	// and other buttons.
}


//
// InputDevices::privateInit:
//
bool InputDevices::privateInit( void * _hInst, void * _hWnd, int flags )
{
	this->joystick_.init( NULL, _hWnd );
	return true;
}


/**
 *	Destructor.
 */
InputDevices::~InputDevices()
{
}


/**
 *	This method processes all device events since it was last called. It asks
 *	the input handler to handle each of these events.
 */
bool InputDevices::privateProcessEvents( InputHandler & handler )
{
	int lostData = NO_DATA_LOST;

	if (!focus_)
		return true;

	// TODO:PM For now, update the Joystick state when this is called.
	this->joystick_.update();

	bool jbLostData = false;
	this->joystick().processEvents( handler, isKeyDown_, &jbLostData );
	if ( jbLostData )
		lostData |= JOY_DATA_LOST;

	for (uint i = 0; i < gVirtualKeyboards.size(); i++)
	{
		KeyboardDevice * pKB = gVirtualKeyboards[i];
		pKB->update();

		KeyEvent event;
		while (pKB->next( event ))
		{
			isKeyDown_[ event.key() ] = event.isKeyDown();
			handler.handleKeyEvent( event );
		}
	}

	return true;
}


/**
 *	This method is called if DirectInput encountered buffer
 *	overflow or lost data, and button events were lost.
 *
 *	We get the current state of all buttons, and compare them to
 *	our presumed state.  If there is any difference, then create
 *	imaginary events.
 *
 *	Note that while these events will be delivered out of order,
 *	vital key up events that were missed will be delivered, saving
 *	the game from untenable positions
 */
void InputDevices::handleLostData( InputHandler & handler, int mask )
{
}


// -----------------------------------------------------------------------------
// Section: Joystick
// -----------------------------------------------------------------------------


/**
 *	This method initialises the Joystick.
 */
bool Joystick::init( IDirectInput7 * pDirectInput, void * hWnd )
{
	isUsingKeyboard_ = false;

	return true;
}


/*
 *	Simple helper method to convert from the joystick axis coordinates that
 *	DirectInput returns to a float in the range [-1, 1].
 */
static inline float scaleFromDIToUnit( int value )
{
	// We want to do the following, where the range mappings are linear.
	//
	// [-DIRECT_INPUT_AXIS_MAX,			DIRECT_INPUT_AXIS_DEAD_ZONE	] -> [-1, 0]
	// [-DIRECT_INPUT_AXIS_DEAD_ZONE,	DIRECT_INPUT_AXIS_DEAD_ZONE	] -> [ 0, 0]
	// [DIRECT_INPUT_AXIS_DEAD_ZONE,	DIRECT_INPUT_AXIS_MAX		] -> [ 0, 1]

	bool isNegative = false;

	if (value < 0)
	{
		value = -value;
		isNegative = true;
	}

	value -= DIRECT_INPUT_AXIS_DEAD_ZONE;


	if (value < 0)
	{
		value = 0;
	}

	float floatValue = (float)value/
		(float)(DIRECT_INPUT_AXIS_MAX - DIRECT_INPUT_AXIS_DEAD_ZONE);

	return isNegative ? -floatValue : floatValue;
}


/**
 *	This method updates this object from a keyboard device.
 */
bool Joystick::updateFromKeyboardDevice()
{
	return true;
}


/**
 *	This method updates this object from a joystick device.
 */
bool Joystick::updateFromJoystickDevice()
{
	const XINPUT_STATE & padState = XBInput::instance().state();

	this->getAxis( AxisEvent::AXIS_LX ).value(
		scaleFromDIToUnit( padState.Gamepad.sThumbLX ) );
	this->getAxis( AxisEvent::AXIS_LY ).value(
		scaleFromDIToUnit( padState.Gamepad.sThumbLY ) );

	this->getAxis( AxisEvent::AXIS_RX ).value(
		scaleFromDIToUnit( padState.Gamepad.sThumbRX ) );
	this->getAxis( AxisEvent::AXIS_RY ).value(
		scaleFromDIToUnit( padState.Gamepad.sThumbRY ) );

	return true;
}





/**
 *	This methods processes the pending joystick events.
 */
bool Joystick::processEvents( InputHandler & handler,
							 bool * pIsKeyDown,
							 bool * pLostDataFlag )
{
	MF_ASSERT( pIsKeyDown != NULL )

	XBInput::instance().update();

	const XINPUT_STATE & padState = XBInput::instance().state();

	bool	boolbut[24];
	const uint8 halfDownLevels[] = { 80, 80, 80, 80, 80, 80, 128, 128 };
	for (int i = 0; i < 8; i++)
	{
		boolbut[i] = !!(padState.Gamepad.wButtons & (1<<i));
		boolbut[i+8] = padState.Gamepad.bAnalogButtons[i] > halfDownLevels[i];
		boolbut[i+16] = padState.Gamepad.bAnalogButtons[i] > 240;
	}

	for (int i = 0; i < 24; i++)
	{
		int key = KeyEvent::KEY_JOY0 + i;
		if (boolbut[i] != pIsKeyDown[ key ])
			this->generateKeyEvent( boolbut[ i ], key, handler, pIsKeyDown );
	}

	this->generateCommonEvents( handler, pIsKeyDown );

	return true;
}



static PyObject* py_rumble( PyObject * args )
{
	float leftMotor, rightMotor;

	if (!PyArg_ParseTuple( args, "ff", &rightMotor, &leftMotor ))
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.rumble: "
				"expected args(leftMotor, rightMotor)" );
		return NULL;
	}

#ifdef _XBOX
	XBInput::instance().rumble(leftMotor, rightMotor);
#endif

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( rumble, BigWorld )

BW_END_NAMESPACE

// xbinput.cpp
