// input.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: KeyStates
// -----------------------------------------------------------------------------

INLINE KeyStates::KeyStates()
{
	reset();
}

INLINE void KeyStates::reset()
{
	BW_GUARD;
	for ( size_t i = 0; i < KeyCode::NUM_KEYS; i++ )
	{
		keyStates_[ i ] = -1;
	}
}

INLINE int32 KeyStates::updateKey( KeyCode::Key key, bool down ) 
{ 
	keyStates_[key] = down ? keyStates_[key]+1 : -1; 
	return keyStates_[key];
}

INLINE void KeyStates::setKeyState( KeyCode::Key key, int32 state )
{
	keyStates_[key] = state;
}

INLINE int32 KeyStates::setKeyStateNoRepeat( KeyCode::Key key, bool down )
{
	keyStates_[key] = down ? 0 : -1; 
	return keyStates_[key];
}

INLINE int32 KeyStates::copyKeyState( KeyCode::Key src, KeyCode::Key dest )
{
	keyStates_[dest] = keyStates_[src];
	return keyStates_[dest];
}

// -----------------------------------------------------------------------------
// Section: KeyEvent
// -----------------------------------------------------------------------------

INLINE KeyEvent KeyEvent::make( KeyCode::Key key, int32 state, uint32 mods, 
								const Vector2& cursorPos )
{ 
	KeyEvent e; 
	e.fill( key, state, mods, cursorPos ); 
	return e; 
}

INLINE KeyEvent KeyEvent::make( KeyCode::Key key, bool keyDown, uint32 mods, 
								const Vector2& cursorPos )
{ 
	KeyEvent e; 
	e.fill( key, keyDown, mods, cursorPos ); 
	return e; 
}

INLINE KeyEvent KeyEvent::make( KeyCode::Key key, int32 state, const wchar_t *ch, 
								uint32 mods, const Vector2& cursorPos )
{
	KeyEvent e;
	e.fill( key, state, ch, mods, cursorPos );
	return e; 
}

INLINE void KeyEvent::fill( const KeyEvent& o )
{ 
	key_ = o.key_; 
	state_ = o.state_; 
	modifiers_ = o.modifiers_; 
	cursorX_ = o.cursorX_;
	cursorY_ = o.cursorY_;
	wcsncpy( character_, o.character_, ARRAY_SIZE(character_) );
}

INLINE void KeyEvent::fill( KeyCode::Key key, int32 state, 
							uint32 mods, const Vector2& cursorPos ) 
{ 
	key_ = key; 
	state_ = state; 
	modifiers_ = mods; 
	cursorX_ = cursorPos.x;
	cursorY_ = cursorPos.y;
	character_[0] = NULL;
}

INLINE void KeyEvent::fill( KeyCode::Key key, int32 state, const wchar_t *ch, 
							uint32 mods, const Vector2& cursorPos )
{
	key_ = key; 
	state_ = state; 
	modifiers_ = mods;
	cursorX_ = cursorPos.x;
	cursorY_ = cursorPos.y;
	wcsncpy( character_, ch, ARRAY_SIZE(character_) );
}


INLINE void KeyEvent::fill( KeyCode::Key key, bool keyDown, 
							uint32 mods, const Vector2& cursorPos )
{ 
	fill( key, keyDown ? STATE_JUST_PRESSED : STATE_NOT_PRESSED, 
			mods, cursorPos ); 
}

INLINE void KeyEvent::appendChar( const wchar_t* ch )
{
	size_t srclen = wcslen(ch);
	size_t destlen = wcslen(character_);
	
	if (destlen < CHAR_MAX_SIZE - srclen - 1)
	{
		wcsncpy( &character_[destlen], ch, srclen );
		character_[destlen+srclen] = NULL;
	}	
}

INLINE bool KeyEvent::isMouseButton() const
{
	return	key_ == KeyCode::KEY_LEFTMOUSE ||
			key_ == KeyCode::KEY_RIGHTMOUSE ||
			key_ == KeyCode::KEY_MIDDLEMOUSE;
}

// -----------------------------------------------------------------------------
// Section: InputDevices
// -----------------------------------------------------------------------------

/**
 *	This method initialises the input devices.
 *
 *	@param hInst	The Windows HINSTANCE.
 *	@param hWnd		The Windows HWND
 *
 *	@return		True if the initialisation succeeded, false otherwise.
 */
INLINE bool InputDevices::init( void * hInst, void * hWnd )
{
	BW_GUARD;
	return instance().privateInit( hInst, hWnd );
}


/**
 *	This method processes the pending events in the input devices and sends them
 *	to the InputHandler argument. That is, it calls the handleKeyEvent and
 *	handleMouseEvent methods of the interface.
 *
 *	@param handler	The input handler that will process the events.
 *
 *	@return		Returns false if an error occurred.
 */
INLINE bool InputDevices::processEvents( InputHandler & handler )
{
	BW_GUARD;
	return instance().privateProcessEvents( handler );
}

/**
 *	This method processes an incoming Windows message. Doesn't call DefWindowProc
 *	if the message isn't handled. If it returns true, the return code is stored 
 *	in 'result'.
 *
 *	@param msg	The windows message type.
 *	@param wParam	The message WPARAM.
 *	@param lParam	The message LPARAM.
 *	@param [out] result	The return code
 *
 *	@return		Returns true if the message was processed, false otherwise.
 */
INLINE bool InputDevices::handleWindowsMessage( HWND hWnd, UINT msg, WPARAM& wParam, LPARAM& lParam, LRESULT& result )
{
	BW_GUARD;
	if (pInstance() != NULL)
	{
		return instance().handleWindowsMessagePrivate( hWnd, msg, wParam, lParam, result );
	}
	
	return false;
}


/**
 *	This method returns whether or not the input key is down.
 *
 *	@param key	The key to test the state of.
 *
 *	@return		Returns true if the key is down and false otherwise.
 */
INLINE bool InputDevices::isKeyDown( KeyCode::Key key )
{
	return instance().keyStates_.isKeyDown( key );
}

/**
 *	Determines the auto-repeat count for the given key. 
 *
 *	@param key	The key to test the state of.
 *
 *	@return		Returns 0..n if the is down (0 indicating just pressed).
 *				Returns -1 if the key is not pressed.
 */
INLINE int32 InputDevices::keyRepeatCount( KeyCode::Key key )
{
	return instance().keyStates_.keyRepeatCount( key );
}


/**
 *	This helper method returns whether or not either Alt key is down.
 *
 *	@return		Returns true if either Alt key is down and false otherwise.
 */
INLINE bool InputDevices::isAltDown()
{
	return isKeyDown( KeyCode::KEY_LALT ) ||
				isKeyDown( KeyCode::KEY_RALT );
}


/**
 *	This helper method returns whether or not either Ctrl key is down.
 *
 *	@return		Returns true if either Ctrl key is down and false otherwise.
 */
INLINE bool InputDevices::isCtrlDown()
{
	return isKeyDown( KeyCode::KEY_LCONTROL ) ||
				isKeyDown( KeyCode::KEY_RCONTROL );
}


/**
 *	This helper method returns whether or not either Shift keys is down.
 *
 *	@return		Returns true if either Shift key is down and false otherwise.
 */
INLINE bool InputDevices::isShiftDown()
{
	return isKeyDown( KeyCode::KEY_LSHIFT ) ||
				isKeyDown( KeyCode::KEY_RSHIFT );
}


/**
 *	This method returns the current state of the modifier keys.
 */
INLINE uint32 InputDevices::modifiers()
{
	return
		(isShiftDown()	? MODIFIER_SHIFT : 0) |
		(isCtrlDown()	? MODIFIER_CTRL  : 0) |
		(isAltDown()	? MODIFIER_ALT   : 0);
}


/**
 *	This method flags the class so it consumes all events created up to this
 *	moment without processing.
 */
INLINE /*static*/ void InputDevices::consumeInput()
{
	instance().consumeUpToSeqId_ = InputEvent::s_seqId_;
}


// -----------------------------------------------------------------------------
// Section: InputHandler
// -----------------------------------------------------------------------------

/**
 *	Base class key event handler, which never handles it
 */
INLINE
bool InputHandler::handleKeyEvent( const KeyEvent & )
{
	return false;
}

/**
 *	Base class language change event handler, which never handles it
 */
INLINE
bool InputHandler::handleInputLangChangeEvent()
{
	return false;
}

/**
 *	Base class IME event handler, which never handles it
 */
INLINE
bool InputHandler::handleIMEEvent( const IMEEvent & )
{
	return false;
}

/**
 *	Base class mouse event handler, which never handles it
 */
INLINE
bool InputHandler::handleMouseEvent( const MouseEvent & )
{
	return false;
}

/**
 *	Base class axis event handler, which never handles it
 */
INLINE
bool InputHandler::handleAxisEvent( const AxisEvent & )
{
	return false;
}

/**
 *  This method resets the current input state (forces all keys up).
 */
INLINE void InputDevices::resetKeyStates()
{
	keyStates_.reset();
	keyStatesInternal_.reset();
}


/**
 * This method clears the queue of key presses which are to be processed
 * next update.
 */
INLINE void InputDevices::clearEventQueue()
{

	eventQueue_.clear();
}

// input.ipp
