#include "pch.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/stl_to_py.hpp"
#include "pyscript/script.hpp"
#include "input.hpp"
#include "ime.hpp"

#include "py_input.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyKeyEvent
// -----------------------------------------------------------------------------

/*~ class BigWorld.PyKeyEvent
 *	@components{ client, tools }
 *
 *	This class represents an input event generated by a key (whether it be keyboard,
 *	mouse, or joystick).
 *
 *
 */
PY_TYPEOBJECT( PyKeyEvent )

PY_BEGIN_METHODS( PyKeyEvent )
	/*~ function PyKeyEvent.isKeyDown
	 *	@components{ client, tools }
	 *
	 *  Determines whether or not this is a key down event.
	 *
	 *  @return		A boolean indicating whether or not this is a key down event.
	 */
	PY_METHOD( isKeyDown )
	/*~ function PyKeyEvent.isKeyUp
	 *	@components{ client, tools }
	 *
	 *  Determines whether or not this is a key up event.
	 *
	 *  @return		A boolean indicating whether or not this is a key up event.
	 */
	PY_METHOD( isKeyUp )
	/*~ function PyKeyEvent.isRepeatedEvent
	 *	@components{ client, tools }
	 *
	 *  Returns true if this event is due to auto-repeat, false otherwise.
	 *
	 *  @return		A boolean indicating whether or not this is an auto-repeat event.
	 */
	PY_METHOD( isRepeatedEvent )
	/*~ function PyKeyEvent.isShiftDown
	 *	@components{ client, tools }
	 *
	 *  This method returns whether or not either shift key was down when this
	 *	event occurred.
	 *
	 *  @return		A boolean indicating whether or not shift was down.
	 */
	PY_METHOD( isShiftDown )
	/*~ function PyKeyEvent.isAltDown
	 *	@components{ client, tools }
	 *
	 *  This method returns whether or not either alt key was down when this
	 *	event occurred.
	 *
	 *  @return		A boolean indicating whether or not alt was down.
	 */
	PY_METHOD( isAltDown )
	/*~ function PyKeyEvent.isCtrlDown
	 *	@components{ client, tools }
	 *
	 *  This method returns whether or not either control key was down when this
	 *	event occurred.
	 *
	 *  @return		A boolean indicating whether or not control was down.
	 */
	PY_METHOD( isCtrlDown )
	/*~ function PyKeyEvent.isModifierDown
	 *	@components{ client, tools }
	 *
	 *  This method returns whether or not any of the modifier keys (i.e. shift,
	 *	alt, or control) were down when this event occured.
	 *
	 *  @return		A boolean indicating whether or not a modifier was down.
	 */
	PY_METHOD( isModifierDown )	
	/*~ function PyKeyEvent.isMouseButton
	 *	@components{ client, tools }
	 *
	 *  Determines whether or not this event refers to a mouse button.
	 *
	 *  @return		A boolean indicating whether or not this is a mouse button.
	 */
	PY_METHOD( isMouseButton )		
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyKeyEvent )
	/*~ attribute PyKeyEvent.key
	 *	@components{ client, tools }
	 *
	 *  The key-code of the key that generated this event.
	 */
	PY_ATTRIBUTE( key )
	/*~ attribute PyKeyEvent.modifiers
	 *	@components{ client, tools }
	 *
	 *  Modifiers indicates which modifier keys were pressed when the event 
	 *	occurred, and can include MODIFIER_SHIFT, MODIFIER_CTRL, MODIFIER_ALT.
	 */
	PY_ATTRIBUTE( modifiers )
	/*~ attribute PyKeyEvent.repeatCount
	 *	@components{ client, tools }
	 *
	 *  Represents the repeat count for this key. This will be zero 
	 *	for the very first key press. It will be -1 if it is a key-up event.
	 */
	PY_ATTRIBUTE( repeatCount )
	/*~ attribute PyKeyEvent.character
	 *	@components{ client, tools }
	 *
	 *  The Unicode character that was generated by this key event. If the key did 
	 *	not generate a character then this attribute is None.
	 */
	PY_ATTRIBUTE( character )
	/*~ attribute PyKeyEvent.cursorPosition
	 *	@components{ client, tools }
	 *
	 *  The clip-space position of where the mouse cursor was when the event occured.
	 */
	PY_ATTRIBUTE( cursorPosition )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS_DECLARE( PyKeyEvent );
PY_SCRIPT_CONVERTERS( PyKeyEvent );

/*~ function BigWorld.KeyEvent
 *	@components{ client, tools }
 *
 *	This function creates a new PyKeyEvent object. This can be used
 *	to create artificial events which did not originate from the 
 *	engine's input system itself.
 *
 *	@param	key			The key code.
 *	@param	state		The state of the key, where -1 indicates up and
 *						>= 0 indicates the repeat count (so 0 means the key
 *						has just been pressed).
 *	@param	modifiers	The state of the modifier keys (ALT, CONTROL, SHIFT).
 *	@param	character	A unicode string or None.
 *	@param	cursorPos	The clip-space position of the mouse cursor.
 *
 *	@return			the newly created key event.
 */
PY_FACTORY_NAMED( PyKeyEvent, "KeyEvent", BigWorld )


/**
 *	Specialised get method for the character attribute.
 */
PyObject* PyKeyEvent::pyGet_character()
{
	if (event_.utf16Char()[0] == NULL)
	{
		Py_RETURN_NONE;
	}
	else
	{
		return Script::getData( BW::wstring( event_.utf16Char() ) );
	}
}


/**
 *	Static python factory method.
 */
PyObject *PyKeyEvent::pyNew( PyObject *args )
{
	BW_GUARD;
	int key, state, modifiers;
	PyObject* character = NULL;
	PyObject* cursorPosition = NULL;

	if (!PyArg_ParseTuple( args, "iiiOO:BigWorld.KeyEvent", 
					&key, &state, &modifiers, &character, 
					&cursorPosition ))
	{
		return NULL;
	}

	Vector2 vCursorPosition;
	if (Script::setData( cursorPosition, vCursorPosition ) != 0)
	{
		PyErr_SetString( PyExc_TypeError, 
			"BigWorld.KeyEvent: Expected a Vector2 for parameter 5." );
		return NULL;
	}

	KeyEvent keyEvent;
	if (character == Py_None )
	{
		keyEvent.fill( KeyCode::Key(key), state, modifiers, vCursorPosition );
	}
	else
	{
		BW::wstring characterString;
		if (Script::setData( character, characterString ) != 0)
		{
			PyErr_SetString( PyExc_TypeError, 
				"BigWorld.KeyEvent: Expected a unicode string or None for parameter 4." );
			return NULL;
		}

		keyEvent.fill( KeyCode::Key(key), state, 
						characterString.c_str(),
						modifiers, vCursorPosition );
	}

	return new PyKeyEvent( keyEvent );
}


// -----------------------------------------------------------------------------
// Section: PyMouseEvent
// -----------------------------------------------------------------------------

/*~ class BigWorld.PyMouseEvent
 *	@components{ client, tools }
 *
 *	This class represents an input event generated by moving the mouse.
 *
 */
PY_TYPEOBJECT( PyMouseEvent )

PY_BEGIN_METHODS( PyMouseEvent )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyMouseEvent )
	/*~ attribute PyMouseEvent.dx
	 *	@components{ client, tools }
	 *
	 *  The movement delta in the X direction.
	 */
	PY_ATTRIBUTE( dx )
	/*~ attribute PyMouseEvent.dy
	 *	@components{ client, tools }
	 *
	 *  The movement delta in the Y direction.
	 */
	PY_ATTRIBUTE( dy )
	/*~ attribute PyMouseEvent.dz
	 *	@components{ client, tools }
	 *
	 *  The movement delta in the Y direction (i.e. mouse wheel).
	 */
	PY_ATTRIBUTE( dz )
	/*~ attribute PyMouseEvent.cursorPosition
	 *	@components{ client, tools }
	 *
	 *  The clip-space position of where the mouse cursor was when the event occured.
	 */
	PY_ATTRIBUTE( cursorPosition )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS_DECLARE( PyMouseEvent );
PY_SCRIPT_CONVERTERS( PyMouseEvent );

/*~ function BigWorld.MouseEvent
 *	@components{ client, tools }
 *
 *	This function creates a new PyMouseEvent object. This can be used
 *	to create artificial events which did not originate from the 
 *	engine's input system itself.
 *
 *	@param	dx			A signed integer representing the delta movement on the X-axis.
 *	@param	dy			A signed integer representing the delta movement on the Y-axis.
 *	@param	dz			A signed integer representing the mouse wheel.
 *	@param	cursorPos	The clip-space position of the mouse cursor.
 *
 *	@return			the newly created mouse event.
 */
PY_FACTORY_NAMED( PyMouseEvent, "MouseEvent", BigWorld )

/**
 *	Static python factory method.
 */
PyObject *PyMouseEvent::pyNew( PyObject *args )
{
	BW_GUARD;
	int dx, dy, dz;
	PyObject* cursorPosition = NULL;

	if (!PyArg_ParseTuple( args, "iiiO:BigWorld.MouseEvent", 
					&dx, &dy, &dz, &cursorPosition ))
	{
		return NULL;
	}

	Vector2 vCursorPosition;
	if (Script::setData( cursorPosition, vCursorPosition ) != 0)
	{
		PyErr_SetString( PyExc_TypeError, 
			"BigWorld.KeyEvent: Expected a Vector2 for parameter 4." );
		return NULL;
	}

	MouseEvent mouseEvent;
	mouseEvent.fill( dx, dy, dz, vCursorPosition );

	return new PyMouseEvent( mouseEvent );
}

// -----------------------------------------------------------------------------
// Section: PyAxisEvent
// -----------------------------------------------------------------------------

/*~ class BigWorld.PyAxisEvent
 *	@components{ client, tools }
 *
 *	This class represents an input event generated by moving a joystick axis.
 *
 */
PY_TYPEOBJECT( PyAxisEvent )

PY_BEGIN_METHODS( PyAxisEvent )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyAxisEvent )
	/*~ attribute PyAxisEvent.axis
	 *	@components{ client, tools }
	 *
	 *  The axis that was moved by the user. This can be one of the 
	 *	following values, defined in Keys.py:
	 *
	 *	@{
	 *	Keys.AXIS_LX	= 0
	 *	Keys.AXIS_LY	= 1
	 *	Keys.AXIS_RX	= 2
	 *	Keys.AXIS_RY	= 3
	 *	}@
	 *
	 */
	PY_ATTRIBUTE( axis )
	/*~ attribute PyAxisEvent.value
	 *	@components{ client, tools }
	 *
	 *  The axis movement value, ranged between -1.0 and +1.0.
	 */
	PY_ATTRIBUTE( value )
	/*~ attribute PyAxisEvent.dTime
	 *	@components{ client, tools }
	 *
	 *  The time since that axis was last processed. 
	 */
	PY_ATTRIBUTE( dTime )	
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS_DECLARE( PyAxisEvent );
PY_SCRIPT_CONVERTERS( PyAxisEvent );

/*~ function BigWorld.AxisEvent
 *	@components{ client, tools }
 *
 *	This function creates a new PyAxisEvent object. This can be used
 *	to create artificial events which did not originate from the 
 *	engine's input system itself.
 *
 *	@param	axis		The axis index, as defined in Keys.py
 *	@param	value		A float value between -1.0 and +1.0.
 *	@param	dTime		A value representing the time since the last 
 *						time the axis was processed.
 *
 *	@return			the newly created axis event.
 */
PY_FACTORY_NAMED( PyAxisEvent, "AxisEvent", BigWorld )


/**
 *	Static python factory method.
 */
PyObject *PyAxisEvent::pyNew( PyObject *args )
{
	BW_GUARD;
	int axis;
	float value, dTime;

	if (!PyArg_ParseTuple( args, "iff:BigWorld.AxisEvent", 
					&axis, &value, &dTime ))
	{
		return NULL;
	}

	axis = uint(axis) % AxisEvent::NUM_AXES;
	if (value < -1.f) value = -1.f;
	if (value >  1.f) value =  1.f;
	if (dTime < 0.f) dTime = 0.f;

	AxisEvent axisEvent;
	axisEvent.fill( AxisEvent::Axis(axis), value, dTime );

	return new PyAxisEvent( axisEvent );
}

// -----------------------------------------------------------------------------
// Section: PyIMEEvent
// -----------------------------------------------------------------------------

/*~ class BigWorld.PyIMEEvent
 *
 *	This class is used to notify the scripts when an IME state change has
 *	occured. It has a set of flags to determine which properties on the global
 *	PyIME object have changed.
 *
 */
PY_TYPEOBJECT( PyIMEEvent )

PY_BEGIN_METHODS( PyIMEEvent )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyIMEEvent )
	/*~ attribute PyIMEEvent.stateChanged
	 *
	 *  If this is True, then the PyIME.state attribute has changed.
	 */
	PY_ATTRIBUTE( stateChanged )
	/*~ attribute PyIMEEvent.readingVisibilityChanged
	 *
	 *  If this is True, then the PyIME.readingVisible attribute has changed.
	 */
	PY_ATTRIBUTE( readingVisibilityChanged )
	/*~ attribute PyIMEEvent.readingChanged
	 *
	 *  If this is True, then the PyIME.reading attribute has changed.
	 */
	PY_ATTRIBUTE( readingChanged )
	/*~ attribute PyIMEEvent.candidatesVisibilityChanged
	 *
	 *  If this is True, then the PyIME.candidatesVisible attribute has changed.
	 */
	PY_ATTRIBUTE( candidatesVisibilityChanged )
	/*~ attribute PyIMEEvent.candidatesChanged
	 *
	 *  If this is True, then the PyIME.candidates attribute has changed.
	 */
	PY_ATTRIBUTE( candidatesChanged )
	/*~ attribute PyIMEEvent.candidateSelectionChanged
	 *
	 *  If this is True, then the PyIME.selectedCandidate attribute has changed.
	 */
	PY_ATTRIBUTE( selectedCandidateChanged )
	/*~ attribute PyIMEEvent.compositionChanged
	 *
	 *  If this is True, then the PyIME.composition attribute has changed.
	 */
	PY_ATTRIBUTE( compositionChanged )
	/*~ attribute PyIMEEvent.compositionCursorPositionChanged
	 *
	 *  If this is True, then the PyIME.compositionCursorPosition attribute has changed.
	 */
	PY_ATTRIBUTE( compositionCursorPositionChanged )	
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS_DECLARE( PyIMEEvent );
PY_SCRIPT_CONVERTERS( PyIMEEvent );

// -----------------------------------------------------------------------------
// Section: Event Converters
// -----------------------------------------------------------------------------

namespace Script
{
	/// PyScript converter for KeyEvents
	int setData( PyObject * pObject, KeyEvent & rEvent,
		const char * varName )
	{
		BW_GUARD;

		if (!PyKeyEvent::Check( pObject ) )
		{
			PyErr_Format( PyExc_TypeError,
				"%s must be set to a PyKeyEvent object.", varName );
			return -1;
		}

		rEvent.fill( ((PyKeyEvent*)pObject)->event() );
		return 0;
	}

	/// PyScript converter for KeyEvents
	PyObject * getData( const KeyEvent & rEvent )
	{
		BW_GUARD;
		return new PyKeyEvent( rEvent );
	}

	/// PyScript converter for IMEEvents
	int setData( PyObject * pObject, IMEEvent & rEvent,
		const char * varName )
	{
		BW_GUARD;

		if (!PyIMEEvent::Check( pObject ) )
		{
			PyErr_Format( PyExc_TypeError,
				"%s must be set to a PyIMEEvent object.", varName );
			return -1;
		}

		rEvent = ((PyIMEEvent*)pObject)->event();
		return 0;
	}

	/// PyScript converter for IMEEvents
	PyObject * getData( const IMEEvent & rEvent )
	{
		BW_GUARD;
		return new PyIMEEvent( rEvent );
	}

	/// PyScript converter for MouseEvents
	int setData( PyObject * pObject, MouseEvent & rEvent,
		const char * varName )
	{
		BW_GUARD;

		if (!PyMouseEvent::Check( pObject ) )
		{
			PyErr_Format( PyExc_TypeError,
				"%s must be set to a PyMouseEvent object.", varName );
			return -1;
		}

		rEvent.fill( ((PyMouseEvent*)pObject)->event() );
		return 0;
	}

	/// PyScript converter for MouseEvents
	PyObject * getData( const MouseEvent & rEvent )
	{
		BW_GUARD;
		return new PyMouseEvent( rEvent );
	}

	/// PyScript converter for AxisEvents
	int setData( PyObject * pObject, AxisEvent & rEvent,
		const char * varName )
	{
		BW_GUARD;

		if (!PyAxisEvent::Check( pObject ) )
		{
			PyErr_Format( PyExc_TypeError,
				"%s must be set to a PyAxisEvent object.", varName );
			return -1;
		}

		rEvent.fill( ((PyAxisEvent*)pObject)->event() );
		return 0;
	}

	/// PyScript converter for AxisEvents
	PyObject * getData( const AxisEvent & rEvent )
	{
		BW_GUARD;
		return new PyAxisEvent( rEvent );
	}

};	// namespace Script


// -----------------------------------------------------------------------------
// Section: PyIME
// -----------------------------------------------------------------------------

class PyIME : public PyObjectPlus
{
	Py_Header( PyIME, PyObjectPlus )

public:
	PyIME( PyTypeObject* pType = &PyIME::s_type_ );
	~PyIME();

	void enabled( bool enable );
	bool enabled();
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, enabled, enabled );

	PY_RO_ATTRIBUTE_DECLARE( IME::instance().language(), language );
	PY_RO_ATTRIBUTE_DECLARE( IME::instance().state(), state );
	PY_RO_ATTRIBUTE_DECLARE( IME::instance().composition(), composition );
	PY_RO_ATTRIBUTE_DECLARE( compositionAttrHolder_, compositionAttr );		
	PY_RO_ATTRIBUTE_DECLARE( IME::instance().compositionCursorPosition(), compositionCursorPosition );
	PY_RO_ATTRIBUTE_DECLARE( IME::instance().readingVisible(), readingVisible );
	PY_RO_ATTRIBUTE_DECLARE( IME::instance().readingVertical(), readingVertical );
	PY_RO_ATTRIBUTE_DECLARE( IME::instance().reading(), reading );
	PY_RO_ATTRIBUTE_DECLARE( candidatesHolder_, candidates );
	PY_RO_ATTRIBUTE_DECLARE( IME::instance().candidatesVisible(), candidatesVisible );
	PY_RO_ATTRIBUTE_DECLARE( IME::instance().candidatesVertical(), candidatesVertical );
	PY_RO_ATTRIBUTE_DECLARE( IME::instance().selectedCandidate(), selectedCandidate );
	PY_RO_ATTRIBUTE_DECLARE( IME::instance().currentlyVisible(), currentlyVisible );
private:
	PySTLSequenceHolder<IME::WStringArray> candidatesHolder_;
	PySTLSequenceHolder<IME::AttrArray> compositionAttrHolder_;
};

PyIME::PyIME( PyTypeObject* pType ) 
	:	PyObjectPlus( pType ),
		candidatesHolder_( IME::instance().candidatesNonConst(), this, false ),
		compositionAttrHolder_( IME::instance().compositionAttrNonConst(), this, false )
{
}

PyIME::~PyIME()
{
}

void PyIME::enabled( bool enable )
{
	IME::instance().enabled( enable );
}

bool PyIME::enabled()
{
	return IME::instance().enabled();
}


/*~ class BigWorld.PyIME
 *
 *	Provides access to the current IME state for the client process. This cannot 
 *	be directly instanciated and is accessed via the BigWorld.ime singleton object.
 *
 */
PY_TYPEOBJECT( PyIME )

PY_BEGIN_METHODS( PyIME )	
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyIME )
	/*~ attribute PyIME.enabled
	 *
	 *  Controls whether or not IME is enabled for the client. For example, you would
	 *	set this to True when an IME enabled edit control comes into focus and then
	 *	set it to False when it loses focus.
	 *
	 *	@type Read-Write boolean
	 *
	 */
	PY_ATTRIBUTE( enabled )
	/*~ attribute PyIME.language
	 *
	 *  Reflects the current language selected by the user. Supported languages 
	 *	are are:
	 *
	 *	@{
	 *	"NON_IME"	- A language that does not require an IME.
	 *	"CHS"		- Chinese (Simplified)
	 *	"CHT"		- Chinese (Traditional)
	 *	"JAPANESE"	- Japanese
	 *	"KOREAN"	- Korean
	 *	@}
	 *
	 *	@type Read-Only string
	 */
	PY_ATTRIBUTE( language )

	/*~ attribute PyIME.state
	 *
	 *  Reflects the current state of the IME. Possible values are:
	 *
	 *	@{
	 *	"OFF"		- The IME is turned off.
	 *	"ON"		- The IME is active.
	 *	"ENGLISH"	- The IME is active, but is in alpha numeric mode.
	 *	@}
	 *
	 */
	PY_ATTRIBUTE( state )
	/*~ attribute PyIME.composition
	 *
	 *  Contains the unicode string representing the current composition
	 *	entered by the user.
	 *
	 *	@type Read-Only Unicode string
	 *
	 */
	PY_ATTRIBUTE( composition )
	/*~ attribute PyIME.compositionAttr
	 *
	 *  Of the same length as the composition string, this specifies the
	 *	current conversion attributes for each character in the string.
	 *
	 *	Possible values are:
	 *
	 *	@{
	 *	ATTR_INPUT					= 0
	 *	ATTR_TARGET_CONVERTED		= 1
	 *	ATTR_CONVERTED				= 2
	 *	ATTR_TARGET_NOTCONVERTED	= 3
	 *	ATTR_INPUT_ERROR			= 4
	 *	ATTR_FIXEDCONVERTED			= 5
	 *	@}		
	 *
	 *	@type Read-Only Array of integers
	 *
	 */
	PY_ATTRIBUTE( compositionAttr )
	/*~ attribute PyIME.compositionCursorPosition
	 *
	 *  Index into the composition string representing where the input cursor 
	 *	is currently located.
	 *
	 *	@type Read-Only int
	 *
	 */
	PY_ATTRIBUTE( compositionCursorPosition )
	/*~ attribute PyIME.readingVisible
	 *
	 *  Determines whether or not the reading string should be displayed to the user.
	 *
	 *	@type Read-Only boolean
	 *
	 */
	PY_ATTRIBUTE( readingVisible )
	/*~ attribute PyIME.reading
	 *
	 *  Contains the unicode string representing the current reading string
	 *	entered by the user.
	 *
	 *	@type Read-Only Unicode string
	 *
	 */
	PY_ATTRIBUTE( reading )
	/*~ attribute PyIME.readingVertical
	 *
	 *  Determines whether ot not the reading string is horizontal or vertical for
	 *	the current language.
	 *
	 *	@type Read-Only boolean
	 *
	 */
	PY_ATTRIBUTE( readingVertical )	
	/*~ attribute PyIME.candidates
	 *
	 *  Contains a list of possible candidates based on the keystrokes
	 *	entered by the user.
	 *
	 *	@type Read-Only list of Unicode strings
	 *
	 */
	PY_ATTRIBUTE( candidates )
	/*~ attribute PyIME.selectedCandidate
	 *
	 *  Index into the candidate selection list of the currently highlighted candidate.
	 *
	 *	@type Read-Only int
	 *
	 */
	PY_ATTRIBUTE( selectedCandidate )
	/*~ attribute PyIME.candidatesVisible
	 *
	 *  Determines whether or not the candidate list should be displayed to the user.
	 *
	 *	@type Read-Only boolean
	 *
	 */
	PY_ATTRIBUTE( candidatesVisible )
	/*~ attribute PyIME.candidatesVertical
	 *
	 *  Determines whether ot not the candidate list is horizontal or vertical for
	 *	the current language.
	 *
	 *	@type Read-Only boolean
	 *
	 */
	PY_ATTRIBUTE( candidatesVertical )
	/*~ attribute PyIME.currentlyVisible
	 *
	 *  If this is true, then there are IME components with which the user are currently
	 *	interacting (e.g. composition string and/or candidate list).
	 *
	 *	@type Read-Only boolean
	 *
	 */
	PY_ATTRIBUTE( currentlyVisible )
PY_END_ATTRIBUTES()

PY_MODULE_ATTRIBUTE( BigWorld, ime, new PyIME );

// -----------------------------------------------------------------------------
// Section: Input BigWorld module functions
// -----------------------------------------------------------------------------

/*~ function BigWorld.localeInfo
 *	@components{ client, tools }
 *
 *	Returns a tuple containing information about the currently activated input. 
 *	locale. The locale changes when the user selects a new language from the language
 *	bar in the operating system. Use the  BWPersonality.handleInputLangChange
 *	event handler to determine when this happens.
 *
 *	The tuple returned is of the form: 
 *
 *	@{
 *	(country, abbreviatedCountry, language, abbreviatedLanguage)
 *	@}
 *
 *	@return A 4-tuple of unicode strings that contain containing locale information..
 *
 *	@see BWPersonality.handleInputLangChange
 *
 */
/**
 *	Returns information about the currently activated input locale
 */
static PyObject* localeInfo()
{
	BW_GUARD;
	const int BUF_LEN = 64;	
	wchar_t country[BUF_LEN]= {0};
	wchar_t abbrevCountry[BUF_LEN] = {0};
	wchar_t language[BUF_LEN] = {0};
	wchar_t abbrevLanguage[BUF_LEN] = {0};

	LCID lcid = MAKELCID( LOWORD(GetKeyboardLayout(0)), SORT_DEFAULT );

	GetLocaleInfo( lcid, LOCALE_SCOUNTRY, country, BUF_LEN );
	GetLocaleInfo( lcid, LOCALE_SABBREVCTRYNAME, abbrevCountry, BUF_LEN );
	GetLocaleInfo( lcid, LOCALE_SLANGUAGE, language, BUF_LEN );
	GetLocaleInfo( lcid, LOCALE_SABBREVLANGNAME, abbrevLanguage, BUF_LEN );

	PyObject * pTuple = PyTuple_New( 4 );
	PyTuple_SetItem( pTuple, 0, Script::getData( BW::wstring(country) ) );
	PyTuple_SetItem( pTuple, 1, Script::getData( BW::wstring(abbrevCountry) ) );
	PyTuple_SetItem( pTuple, 2, Script::getData( BW::wstring(language) ) );
	PyTuple_SetItem( pTuple, 3, Script::getData( BW::wstring(abbrevLanguage) ) );
	return pTuple;
}

PY_AUTO_MODULE_FUNCTION( RETOWN, localeInfo, END, BigWorld )


/*~ function BigWorld.isKeyDown
 *	@components{ client, tools }
 *
 *	The 'isKeyDown' method allows the script to check if a particular key has
 *	been pressed and is currently still down. The term key is used here to refer
 *	to any control with an up/down status; it can refer to the keys of a
 *	keyboard, the buttons of a mouse or even that of a joystick. The complete
 *	list of keys recognised by the client can be found in the Keys module,
 *	defined in Keys.py.
 *
 *	The return value is zero if the key is not being held down, and a non-zero
 *	value is it is being held down.
 *
 *	@param key	An integer value indexing the key of interest.
 *
 *	@return True (1) if the key is down, false (0) otherwise.
 *
 *	Code Example:
 *	@{
 *	if BigWorld.isKeyDown( Keys.KEY_ESCAPE ):
 *	@}
 */
/**
 *	Returns whether or not the given key is down.
 */
static PyObject * py_isKeyDown( PyObject * args )
{
	BW_GUARD;
	int	key;
	if (!PyArg_ParseTuple( args, "i", &key ))
	{
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.isKeyDown: Argument parsing error." );
		return NULL;
	}

	if ( InputDevices::isKeyDown( (KeyCode::Key)key ) )
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}
PY_MODULE_FUNCTION( isKeyDown, BigWorld )



/*~ function BigWorld.stringToKey
 *	@components{ client, tools }
 *
 *	The 'stringToKey' method converts from the name of a key to its
 *	corresponding key index as used by the 'isKeyDown' method. The string names
 *	for a key can be found in the keys.py file. If the name supplied is not on
 *	the list defined, the value returned is zero, indicating an error. This
 *	method has a inverse method, 'keyToString' which does the exact opposite.
 *
 *	@param string	A string argument containing the name of the key.
 *
 *	@return An integer value for the key with the supplied name.
 *
 *	Code Example:
 *	@{
 *	if BigWorld.isKeyDown( BigWorld.stringToKey( "KEY_ESCAPE" ) ):
 *	@}
 */
/**
 *	Returns the key given its name
 */
static PyObject * py_stringToKey( PyObject * args )
{
	BW_GUARD;
	char * str;
	if (!PyArg_ParseTuple( args, "s", &str ))
	{
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.stringToKey: Argument parsing error." );
		return NULL;
	}

	return PyInt_FromLong( KeyCode::stringToKey( str ) );
}
PY_MODULE_FUNCTION( stringToKey, BigWorld )


/*~ function BigWorld.keyToString
 *	@components{ client, tools }
 *
 *	The 'keyToString' method converts from a key index to its corresponding
 *	string name. The string names returned by the integer index can be found in
 *	the keys.py file. If the index supplied is out of bounds, an empty string
 *	will be returned.
 *
 *	@param key	An integer representing a key index value.
 *
 *	@return A string containing the name of the key supplied.
 *
 *	Code Example:
 *	@{
 *	print BigWorld.keyToString( key ), "pressed."
 *	@}
 */
/**
 *	Returns the name of the given key
 */
static PyObject * py_keyToString( PyObject * args )
{
	BW_GUARD;
	int	key;
	if (!PyArg_ParseTuple( args, "i", &key ))
	{
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.keyToString: Argument parsing error." );
		return NULL;
	}

	return PyString_FromString( KeyCode::keyToString(
		(KeyCode::Key) key ) );
}
PY_MODULE_FUNCTION( keyToString, BigWorld )


/**
 *	Returns the value of the given joypad axis
 */
static PyObject * py_axisDirection( PyObject * args )
{
	BW_GUARD;
	int	stick;
	if (!PyArg_ParseTuple( args, "i", &stick ))
	{
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.axisDirection: Argument parsing error." );
		return NULL;
	}

	return PyInt_FromLong( InputDevices::joystick().stickDirection(stick) );
}


/*~ function BigWorld.axisDirection
 *	@components{ client, tools }
 *
 *	This function returns the direction the specified joystick is pointing in.
 *
 *	The return value indicates which direction the joystick is facing, as
 *	follows:
 *
 *	@{
 *	- 0 down and left
 *	- 1 down
 *	- 2 down and right
 *	- 3 left
 *	- 4 centred
 *	- 5 right
 *	- 6 up and left
 *	- 7 up
 *	- 8 up and right
 *	@}
 *
 *	@param	axis	This is one of AXIS_LX, AXIS_LY, AXIS_RX, AXIS_RY, with the
 *					first letter being L or R meaning left thumbstick or right
 *					thumbstick, the second, X or Y being the direction.
 *
 *	@return			An integer representing the direction of the specified
 *					thumbstick, as listed above.
 */
PY_MODULE_FUNCTION( axisDirection, BigWorld )

BW_END_NAMESPACE

// py_input.cpp
