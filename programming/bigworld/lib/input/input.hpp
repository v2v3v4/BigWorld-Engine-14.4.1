#ifndef INPUT_HPP
#define INPUT_HPP

#include "cstdmf/singleton.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/stringmap.hpp"
#include "cstdmf/guard.hpp"

#include "math/vector2.hpp"

#include "key_code.hpp"

#include <sstream>
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include <queue>
#include "cstdmf/bw_map.hpp"
#include "input/ime.hpp"


struct IDirectInput8;
struct IDirectInputDevice8;


BW_BEGIN_NAMESPACE

class IMEEvent;


/**
 *	An instance of this class represents a key/button press. It may be from the
 *	keyboard, mouse, or a joystick.
 */
class KeyEvent
{
public:	
	static const uint32 CHAR_MAX_SIZE = 4;

	static const int32 STATE_NOT_PRESSED = -1;
	static const int32 STATE_JUST_PRESSED = 0;

	/// Creates a new KeyEvent based on the given parameters.
	static KeyEvent make( 
		KeyCode::Key key, 
		int32 state, 
		uint32 mods, 
		const Vector2& cursorPos );

	static KeyEvent make( 
		KeyCode::Key key, 
		int32 state, 
		const wchar_t *ch, 
		uint32 mods, 
		const Vector2& cursorPos );

	static KeyEvent make( 
		KeyCode::Key key, 
		bool keyDown, 
		uint32 mods, 
		const Vector2& cursorPos );

	/// Fills this struct with the given parameters.
	void fill( const KeyEvent& o );
	void fill( KeyCode::Key key, 
		int32 state, 
		uint32 mods, 
		const Vector2& cursorPos );

	void fill( KeyCode::Key key, 
		int32 state, 
		const wchar_t *ch, 
		uint32 mods, 
		const Vector2& cursorPos );

	void fill( KeyCode::Key key, 
		bool keyDown, 
		uint32 mods, 
		const Vector2& cursorPos );

	/// Resets the event to empty
	void reset() { fill(KeyCode::KEY_NONE, false, 0, Vector2(0,0)); }

	/// Returns the key scan-code that generated this event.
	KeyCode::Key key() const { return key_; }

	/// This method returns whether or not this event was caused by a key being
	///	pressed down.
	bool isKeyDown() const	{ return state_ >= 0; }

	/// This method returns whether or not this event was caused by a key being
	///	released.
	bool isKeyUp() const	{ return state_ < 0; }

	/// This method returns the repeat count for this key. This will be zero 
	/// for the very first key press. It will be -1 if it is a key-up event.
	int32 repeatCount() const { return state_; }

	/// Returns true if this event is due to auto-repeat, false otherwise.
	bool isRepeatedEvent() const { return state_ > 0; }

	/// Returns true if this event is from a mouse button.
	bool isMouseButton() const;

	/**
	 * Returns the character associated with this event. This is represented by
	 * a UTF16 encoded string containing a single character (NULL terminated).
	 * Returns an empty string if no character associated with this event.
	 */
	const wchar_t* utf16Char() const { return character_; }

	/**
	 *	Returns the character associated with this event, encoded in UTF8. If
	 *	this key event did not generate a character, false is returned and output
	 *	is set to an empty string.
	 */
	bool utf8Char( BW::string& output ) const;

	/// Appends the given character to the current character string associated
	/// with this key event.
	void appendChar( const wchar_t* ch );

	/// Returns the modifier key states for this event.
	uint32 modifiers() const { return modifiers_; }

	/// This method returns whether or not either Shift key was down when this
	///	event occurred.
	bool isShiftDown() const
					{ return !!(modifiers_ & MODIFIER_SHIFT); }
	/// This method returns whether or not either Ctrl key was down when this
	///	event occurred.
	bool isCtrlDown() const
					{ return !!(modifiers_ & MODIFIER_CTRL); }
	/// This method returns whether or not either Alt key was down when this
	///	event occurred.
	bool isAltDown() const
					{ return !!(modifiers_ & MODIFIER_ALT); }

	/// Returns the position of the mouse cursor at the time this event occured.
	/// It is in the clip-space of the client window.
	Vector2 cursorPosition() const	
		{ return Vector2( cursorX_, cursorY_ ); }

	BW::string toString() const
	{
		BW_GUARD;
		BW::ostringstream oss;
		oss << "Key " << KeyCode::keyToString( key_ )
			<< " state " << state_
			<< " modifiers " << modifiers_;
		return oss.str();
	}

private:
	KeyCode::Key key_;
	int32 state_;
	uint32 modifiers_;
	wchar_t character_[CHAR_MAX_SIZE];
	float cursorX_, cursorY_;
};

/**
 *	An instance of this class represents a mouse movement event.
 */
class MouseEvent
{
public:
	/// Fills this class with the given parameters.
	void fill( const MouseEvent& o)
	{
		dx_ = o.dx_; dy_ = o.dy_; dz_ = o.dz_; 
		cursorX_ = o.cursorX_; cursorY_ = o.cursorY_;
	}
	void fill( long dx, long dy, long dz, const Vector2& cursorPos )
	{ 
		dx_ = dx; dy_ = dy; dz_ = dz; 
		cursorX_ = cursorPos.x; cursorY_ = cursorPos.y;
	}

	/**
	 *	This method returns the amount the mouse has moved along the x-axis
	 *	during this event.
	 */
	long dx() const		{ return dx_; }

	/**
	 *	This method returns the amount the mouse has moved along the y-axis
	 *	during this event.
	 */
	long dy() const		{ return dy_; }

	/**
	 *	This method returns the amount the mouse has moved along the z-axis
	 *	during this event. The z-axis corresponds to the mouse wheel.
	 */
	long dz() const		{ return dz_; }

	/// Returns the position of the mouse cursor at the time this event occured.
	/// It is in the clip-space of the client window.
	Vector2 cursorPosition() const	{ return Vector2( cursorX_, cursorY_ ); }

private:
	long dx_;
	long dy_;
	long dz_;
	float cursorX_, cursorY_;
};


/**
 *	An instance of this class represents a spring-loaded axis offset
 *	for a certain amount of time.
 */
class AxisEvent
{
public:
	/**
	 *	This enumeration is used to identify different parts of a joypad
	 */
	enum Axis
	{
		AXIS_LX,
		AXIS_LY,
		AXIS_RX,
		AXIS_RY,
		NUM_AXES
	};

	/// Fills this class with the given parameters.
	void fill( const AxisEvent& o )
		{ axis_ = o.axis_; value_ = o.value_; dTime_ = o.dTime_; }
	void fill( Axis axis, float value, float dt )
		{ axis_ = axis; value_ = value; dTime_ = dt; }

	Axis axis() const		{ return axis_; }
	float value() const		{ return value_; }
	float dTime() const		{ return dTime_; }

private:
	Axis	axis_;
	float	value_;
	float	dTime_;
};

/**
 *	Unified representation of an InputEvent. Use the type_ field to determine
 *	which part of the union should be inspected.
 */
class InputEvent
{
public:
	/**
	 *	This enumeration is used to indicate the type of an InputEvent.
	 */
	enum Type
	{
		KEY,	///< The event is a key press/release.
		MOUSE,	///< The event is related to the mouse.
		AXIS,	///< The event is a joystick axis event.
		UNDEFINED
	};

	InputEvent() :
		type_( UNDEFINED ),
		seqId_( ++s_seqId_ )
	{
	}

	BW::string toString() const
	{
		BW_GUARD;
		BW::ostringstream oss;
		if (type_ == KEY)
		{
			oss << key_.toString();
		}
		else
		{
			oss << "Input event";
		}
		return oss.str();
	}

	Type type_;
	union 
	{
		KeyEvent key_;
		MouseEvent mouse_;
		AxisEvent axis_;
	};
	uint32 seqId_;
	static uint32 s_seqId_;
};

/**
 *	This helper class manages the current state of each key.
 */
class KeyStates
{
public:
	KeyStates();

	/**
	 *	This method resets the state of each key.
	 */
	void reset();

	/**
	 *	This method determines whether the given key is currently pressed.
	 */
	bool isKeyDown( KeyCode::Key key ) const { return keyStates_[key] >= 0; }

	/**
	 *	This method gets the auto-repeat count for the given key where 0 
	 *	indicates the key has just been pressed. Returns -1 if the key is 
	 *	not currently pressed.
	 */
	int32 keyRepeatCount( KeyCode::Key key ) const { return keyStates_[key]; }

	/**
	 *	This method updates the given key based on whether the key is down 
	 *	or not.	Essentially this means the repeat count is increased if down 
	 *	is true, and resets to -1 if the key is up. The new repeat count is 
	 *	returned.
	 */
	int32 updateKey( KeyCode::Key key, bool down );

 	/**
	 *	Sets the key-down state for the given button.
	 */
	void setKeyState( KeyCode::Key key, int32 state );

	/**
	 *	Sets the key-down state. This overrides any auto-repeat and always 
	 *	sets the repeat count to 0 if down is true. The new repeat count 
	 *	is returned (either 0 or -1).
	 */
	int32 setKeyStateNoRepeat( KeyCode::Key key, bool down );

	/**
	 *	This method copies the state from src key to dest key. The new 
	 *	state of the destination key is returned.
	 */
	int32 copyKeyState( KeyCode::Key src, KeyCode::Key dest );


private:
	int32 keyStates_[ KeyCode::NUM_KEYS ];
};



/**
 *	This abstract base class is an interface for all input handlers.
 */
class InputHandler
{
public:
	virtual ~InputHandler() {}
	virtual bool handleKeyEvent( const KeyEvent & event );
	virtual bool handleInputLangChangeEvent();
	virtual bool handleIMEEvent( const IMEEvent & event );
	virtual bool handleMouseEvent( const MouseEvent & event );
	virtual bool handleAxisEvent( const AxisEvent & event );
};


/**
 *	This class represents a joystick.
 */
class Joystick
{
public:
	/**
	 *	This class is used to represent a part (or axis) of the joystick. An
	 *	axis, in general, has a value between -1 and 1.
	 */
	class Axis
	{
		public:
			/// Constructor.
			Axis() :
				value_( 0 ),
				enabled_( false ),
				sentZero_( true )		{}

			/// This method returns the value associated with this axis.
			operator float() const			{ return this->value(); }

			/// This method returns the value associated with this axis.
			float value() const				{ return value_; }
			/// This method sets the value associated with this axis.
			void value( float value )		{ value_ = value; }

			/// This method returns whether or not this axis is enabled.
			bool enabled() const			{ return enabled_; }
			/// This method sets whether or not this axis is enabled.
			void enabled( bool enabled )	{ enabled_ = enabled; }

			/// This method returns whether or not a zero position event has been sent.
			bool sentZero() const			{ return sentZero_; }
			/// This method sets whether or not a zero position event has been sent.
			void sentZero( bool sentZero )	{ sentZero_ = sentZero; }

		private:
			float value_;
			bool enabled_;
			bool sentZero_;
	};

	/// @name Constructors and Initialisation
	//@{
	Joystick();

	bool init( IDirectInput8 * pDirectInput,
		void * hWnd );

	bool update();
	//@}

	bool processEvents( InputHandler & handler,
		KeyStates& pKeyStates,
		bool * pLostDataFlag = NULL );

	/// @name Accessors
	//@{
	IDirectInputDevice8 * pDIJoystick() const	{ return pDIJoystick_; }

	Axis & getAxis( AxisEvent::Axis t )				{ return axis_[ t ]; }
	const Axis & getAxis( AxisEvent::Axis t ) const	{ return axis_[ t ]; }

	int stickDirection( int stick ) const		{ return quantJoyDir_[stick]; }
	//@}

private:
	bool updateFromJoystickDevice();

	bool hasJoystick() const					{ return pDIJoystick_ != NULL; }

	void generateKeyEvent( bool isDown, int key, InputHandler & handler,
		KeyStates & keyStates, const Vector2& cursorPosition );

	void generateCommonEvents( InputHandler & handler, KeyStates & keyStates,
								const Vector2& cursorPosition );

private:
	HWND hWnd_;
	IDirectInputDevice8 * pDIJoystick_;
	BW::vector<Axis> axis_;

	uint64		lastProcessedTime_;

	int			quantJoyDir_[2];
};



/**
 *	This class manages the input devices.
 */
class InputDevices : public Singleton< InputDevices >
{
public:
	InputDevices();
	~InputDevices();

	bool init( void * hInst, void * hWnd );

	static bool processEvents( InputHandler & handler );
	static bool handleWindowsMessage( HWND hWnd, 
		UINT msg, 
		WPARAM& wParam, 
		LPARAM& lParam, 
		LRESULT& result );

	static uint32 modifiers();

	static bool isKeyDown( KeyCode::Key key );
	static int32 keyRepeatCount( KeyCode::Key key );
	static bool isAltDown();
	static bool isCtrlDown();
	static bool isShiftDown();

	/// This method returns an object representing the joystick.
	static Joystick & joystick()	{ return instance().joystick_; }

	/// This method sets whether or not the application currently has focus.
	static void setFocus( bool state, InputHandler * handler );
	static bool hasFocus()				{ return pInstance() ? instance().focus_ : focus_; }

	static KeyStates & keyStates()	{ return instance().keyStates_; }

	static void consumeInput();

	static void pushIMECharEvent( wchar_t* chs );

	KeyCode::Key getKeyCodeUp( WPARAM& wParam, bool extendedBit );
	KeyCode::Key getKeyCodeDown( WPARAM& wParam, bool extendedBit );

	void forceKeyUp( KeyCode::Key key, 
		uint32 mods, 
		const Vector2& cursorPos );

	void forceKeysUp();

	void resetKeyStates();
	void clearEventQueue();

	void ignoreNextKeyDown( KeyCode::Key key );

private:
	// Making these void * to avoid including windows.h
	bool privateInit( void * hInst, void * hWnd );

	bool handleWindowsMessagePrivate( HWND hWnd, 
		UINT msg, 
		WPARAM& wParam, 
		LPARAM& lParam, 
		LRESULT& result );

	bool privateProcessEvents( InputHandler & handler );

	void pushKeyEventWin32( KeyCode::Key key, 
		USHORT msg, 
		uint32 mods, 
		const Vector2& cursorPos );

	void pushKeyEvent( KeyCode::Key key, 
		int32 state, 
		uint32 mods, 
		const Vector2& cursorPos );

	void pushKeyEvent( KeyCode::Key key, 
		int32 state, 
		const wchar_t *ch, 
		uint32 mods, 
		const Vector2& cursorPos );

	void pushMouseButtonEvent( KeyCode::Key key, 
		bool state, 
		uint32 mods, 
		const Vector2& cursorPos );

	void pushMouseEvent( long dx, 
		long dy, 
		long dz, 
		const Vector2& cursorPos, 
		bool accumulate=true );





	void updateCursorCapture( HWND hWnd );

	enum lostDataFlags
	{
		NO_DATA_LOST	= 0,
		KEY_DATA_LOST	= 1 << 0,
		MOUSE_DATA_LOST	= 1 << 1,
		JOY_DATA_LOST	= 1 << 2
	};
	void handleLostData( InputHandler & handler, int mask );

	HWND			hWnd_;
	IDirectInput8 * pDirectInput_;

	typedef BW::vector<InputEvent> EventQueue;
	EventQueue eventQueue_;
	bool languageChanged_;

	bool processingEvents_;

	uint32 consumeUpToSeqId_;

	Joystick joystick_;

	KeyStates keyStates_;
	KeyStates keyStatesInternal_;

	USHORT lastRawKeyMakeCode_;
	BW::map<USHORT, KeyCode::Key> rawKeyDownMap_;

	static bool focus_;
	int lostData_;
	KeyCode::Key ignoringKey_;
	bool keyWasIgnored_;
	IME IME_;
};


/**
 *	This class is an interface allowing external files to provide
 *	keyboard devices.
 */
class KeyboardDevice
{
public:
	virtual void update() = 0;
	virtual bool next( KeyEvent & event ) = 0;
};

extern BW::vector<KeyboardDevice*> gVirtualKeyboards;


#ifdef CODE_INLINE
#include "input.ipp"
#endif

BW_END_NAMESPACE

#endif
/*input.hpp*/
