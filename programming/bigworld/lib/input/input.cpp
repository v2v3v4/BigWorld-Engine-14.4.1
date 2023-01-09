#include "pch.hpp"

#include "input.hpp"
#include "vk_map.hpp"
#include "ime.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/dogwatch.hpp"
#include "cstdmf/string_utils.hpp"

DECLARE_DEBUG_COMPONENT2( "UI", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "input.ipp"
#endif

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif
#ifndef HID_USAGE_GENERIC_KEYBOARD
#define HID_USAGE_GENERIC_KEYBOARD     ((USHORT) 0x06)
#endif

#if !defined(EDITOR_ENABLED) && (!defined(SCALEFORM_IME) || (!SCALEFORM_IME))
#define ENABLE_BW_IME 1
#else
#define ENABLE_BW_IME 0
#endif

namespace {

	inline Vector2 clientToClip( HWND hWnd, const POINT& pt )
	{
		RECT r;
		GetClientRect( hWnd, &r );

		return Vector2(  (float(pt.x)/float(r.right)) * 2.0f - 1.0f, 
						-((float(pt.y)/float(r.bottom)) * 2.0f - 1.0f) );
	}

	inline Vector2 currentCursorPos( HWND hWnd )
	{
		POINT cursorPos;
		GetCursorPos( &cursorPos );
		ScreenToClient( hWnd, &cursorPos );

		return clientToClip( hWnd, cursorPos );
	}

	inline Vector2 lastMessageCursorPos( HWND hWnd )
	{
		DWORD dwCursorPos = GetMessagePos();
		POINT msgCursorPos = { 
			GET_X_LPARAM( dwCursorPos ),
			GET_Y_LPARAM( dwCursorPos )
		};
		ScreenToClient( hWnd, &msgCursorPos );

		return clientToClip( hWnd, msgCursorPos );
	}

	inline const char* win32MsgToString( UINT msg )
	{
		switch(msg)
		{
		case WM_KEYDOWN:		return "WM_KEYDOWN";
		case WM_SYSKEYDOWN:		return "WM_SYSKEYDOWN";
		case WM_KEYUP:			return "WM_KEYUP";
		case WM_SYSKEYUP:		return "WM_SYSKEYUP";
		case WM_LBUTTONDOWN:	return "WM_LBUTTONDOWN";
		case WM_LBUTTONUP:		return "WM_LBUTTONUP";
		case WM_RBUTTONDOWN:	return "WM_RBUTTONDOWN";
		case WM_RBUTTONUP:		return "WM_RBUTTONUP";
		case WM_XBUTTONDOWN:	return "WM_XBUTTONDOWN";
		case WM_XBUTTONUP:		return "WM_XBUTTONUP";
		default:				return "(unknown msg)";
		}

	}
}

// ----------------------------------------------------------------------------
// Section: Data declaration
// ----------------------------------------------------------------------------

/// device input library Singleton
BW_SINGLETON_STORAGE( InputDevices )


const int DIRECT_INPUT_AXIS_MAX = 1000;
const int DIRECT_INPUT_AXIS_DEAD_ZONE = 150;

bool InputDevices::focus_;

/// This static creates an ascending ID for events.
/*static*/ uint32 InputEvent::s_seqId_ = 0;


// ----------------------------------------------------------------------------
// Section: InputDevices
// ----------------------------------------------------------------------------

static const int JOYSTICK_BUFFER_SIZE = 32;

static float s_absDeviceDivider = 50.0f;

/**
 *	InputDevices::InputDevices:
 */
InputDevices::InputDevices() : 
	hWnd_( NULL ),
	pDirectInput_( NULL ),
	languageChanged_( false ),
	processingEvents_( false ),
	consumeUpToSeqId_( 0 ),
	lastRawKeyMakeCode_(),
	lostData_( NO_DATA_LOST ),
	ignoringKey_( KeyCode::KEY_NONE ),
	keyWasIgnored_(false)
{
	BW_GUARD;

	MF_WATCH( "Input/Absolute Device Divider", 
		s_absDeviceDivider, Watcher::WT_READ_WRITE );
}


/**
 *	Destructor
 */
InputDevices::~InputDevices()
{
	BW_GUARD;


	// Release our DirectInput object.
	if (pDirectInput_ != NULL)
	{
		pDirectInput_->Release();
		pDirectInput_ = NULL;
	}
}


/**
 * InputDevices::privateInit:
 */
bool InputDevices::privateInit( void * _hInst, void * _hWnd )
{
	BW_GUARD;
	HINSTANCE hInst = static_cast< HINSTANCE >( _hInst );
	hWnd_ = static_cast< HWND >( _hWnd );

	HRESULT hr;

	// hrm, _hInst is being passed in as null from borland, and dinput doesn't
	// like that
#ifdef EDITOR_ENABLED
	hInst = (HINSTANCE) GetModuleHandle( NULL );
#endif

	// Register with the DirectInput subsystem and get a pointer to a
	// IDirectInput interface we can use.
    hr = DirectInput8Create( hInst,
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(LPVOID*)&pDirectInput_,
		NULL );
	if (SUCCEEDED(hr)) 
	{
		// ****** Joystick initialisation. ******
		if (this->joystick_.init( pDirectInput_, _hWnd ))
		{
			INFO_MSG( "InputDevices::InputDevices: "
				"Joystick initialised\n" );
		}
		else
		{
			INFO_MSG( "InputDevices::InputDevices: "
				"Joystick failed to initialise\n" );
		}
	}
	else
	{
		ERROR_MSG( "Failed to create DirectInput device (hr=0x%lx). "
			"There will be no joystick support.\n", hr );
	}

	// Initialise raw input for mouse and keyboard.
	// We do this in two calls to work around a problem with PerfHUD.
	RAWINPUTDEVICE rawDevice;
	memset( &rawDevice, 0, sizeof(rawDevice) );

	rawDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
	rawDevice.usUsage = HID_USAGE_GENERIC_KEYBOARD;
	rawDevice.dwFlags = 0;
	rawDevice.hwndTarget = hWnd_;

	if ( !RegisterRawInputDevices(&rawDevice, 1, sizeof(rawDevice)) )
	{
		ERROR_MSG( "Failed to register keyboard raw input "
			"device (GetLastError=%lu).\n", GetLastError() );
		return false;
	}

	rawDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
	rawDevice.usUsage = HID_USAGE_GENERIC_MOUSE;
	rawDevice.dwFlags = 0;
	rawDevice.hwndTarget = hWnd_;

	if ( !RegisterRawInputDevices(&rawDevice, 1, sizeof(rawDevice)) )
	{
		ERROR_MSG( "Failed to register mouse raw input "
			"device (GetLastError=%lu).\n", GetLastError() );
		return false;
	}

#if ENABLE_BW_IME
	if ( !IME::instance().initIME( hWnd_ ) )	
	{
		WARNING_MSG( "Failed to initialise IME.\n" );
	}
#endif

	return true;
}


/**
 *	This method processes all device events since it was last called. It asks
 *	the input handler to handle each of these events.
 */
bool InputDevices::privateProcessEvents( InputHandler & handler )
{
	BW_GUARD;

	if (!focus_) return true;

	// Prevent re-entry
	if (processingEvents_)
	{
		ERROR_MSG( "InputDevices::privateProcessEvents: "
					"Tried to re-enter the method.\n" );
		return true;
	}
	processingEvents_ = true;

	// Update the Joystick state when this is called.
	this->joystick_.update();

	bool jbLostData = false;
	this->joystick().processEvents( handler, keyStates_, &jbLostData );
	if ( jbLostData )
		lostData_ |= JOY_DATA_LOST;

	static DogWatch watchHandle( "Event Handlers" );

	{ // DogWatch scope
	ScopedDogWatch watcher( watchHandle );

	// Sometimes new events can be added while handling events in the queue, so
	// we need to iterate by index, and avoid iterators.
	for (size_t i = 0; i < eventQueue_.size(); ++i)
	{
		// Use a copy of the event, in case the vector changes.
		const InputEvent& event = eventQueue_[i];

		if (event.seqId_ <= consumeUpToSeqId_)
		{
			// skip this event, we've been told to consume it.
			continue;
		}

		switch( event.type_ )
		{
		case InputEvent::KEY:
			{
				keyStates_.setKeyState( event.key_.key(), 
										event.key_.repeatCount() );
				handler.handleKeyEvent( event.key_ );
				break;
			}

		case InputEvent::MOUSE:
			handler.handleMouseEvent( event.mouse_ );
			break;

		case InputEvent::AXIS:
			handler.handleAxisEvent( event.axis_ );
			break;

		default:
			break;
		}
	}

	eventQueue_.resize(0);
	consumeUpToSeqId_ = 0;

#if ENABLE_BW_IME
	IME::instance().update();
	IME::instance().processEvents( handler );
#endif

	if (languageChanged_)
	{
		handler.handleInputLangChangeEvent();
		languageChanged_ = false;

		IME::instance().onInputLangChange();
	}

	} // DogWatch scope

	//handle lost data
	if (lostData_ != NO_DATA_LOST)
		handleLostData( handler, lostData_ );

	for (uint i = 0; i < gVirtualKeyboards.size(); i++)
	{
		KeyboardDevice * pKB = gVirtualKeyboards[i];
		pKB->update();

		KeyEvent event;
		while (pKB->next( event ))
		{
			keyStates_.updateKey( event.key(), event.isKeyDown() );
			handler.handleKeyEvent( event );
		}
	}

	processingEvents_ = false;
	return true;
}

/**
 *	This is called by the application's window proc in order to 
 *	allow the input system to to handle input related messages.
 */
bool InputDevices::handleWindowsMessagePrivate( 
	HWND hWnd, UINT msg, WPARAM& wParam, 
	LPARAM& lParam, LRESULT& result )
{
	BW_GUARD;

	result = 0;
	bool handled = false;

	switch (msg)
	{
	case WM_INPUTLANGCHANGE:
		languageChanged_ = true;
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		{
			// Ignore alt+tabs/escape
			if ( msg == WM_SYSKEYDOWN &&
				 (wParam == VK_TAB || wParam == VK_ESCAPE) )
			{
				break;
			}

			// These keys are handled by raw input specifically.
			if ( wParam == VK_SNAPSHOT || wParam == VK_SHIFT )
			{
				break;
			}

			// VK_PROCESSKEY == IME sunk key event.
			if ( wParam == VK_PROCESSKEY )
			{
				break;
			}

			bool extendedBit = lParam & (1<<24) ? true : false;
			KeyCode::Key key = KeyCode::KEY_NONE;

			// If this is a key up event, we use the hardware make code to 
			// convert to the corresponding key down rather than using the 
			// VK we were given here. This is because the translated VK can
			// change between key down and key up, causing the key code to
			// get stuck down. For example, this can happen if you use 
			// numpad up, hit numlock, and then release the key (you'll get
			// key up for 8 rather than up arrow).

			// We can't simply use raw input because of IME support (IME 
			// messages get interleaved with the key events).
			if (msg == WM_KEYUP || msg == WM_SYSKEYUP)
			{
				key = this->getKeyCodeUp( wParam, extendedBit );
			}
			else
			{
				key = this->getKeyCodeDown( wParam, extendedBit );
			}

			/*
			DEBUG_MSG( "%s: wParam=%x, lParam=%x "
				"(extended bit=%d, mapped key=%s)\n",
				win32MsgToString(msg), wParam, lParam,
				extendedBit, KeyCode::keyToString( key ) );
			*/

			// HACK to ignore a key press that was already processed by a hook
			// eg. WorldEditor shortcut key
			if (msg == WM_KEYDOWN && ignoringKey_ == key)
			{
				//DEBUG_MSG( "Ignored\n" );
				ignoringKey_ = KeyCode::KEY_NONE;
				keyWasIgnored_ = true;
				break;
			}
			else
			{
				pushKeyEventWin32( key, msg, modifiers(), 
					lastMessageCursorPos( hWnd ) );
				keyWasIgnored_ = false;
				break;
			}
		}
	case WM_CHAR:
	case WM_SYSCHAR:
		{
			wchar_t utf16chr[2] = { LOWORD(wParam), HIWORD(wParam) };

			// Attach to the most recent event, which works since 
			// WM_CHAR is posted immediately after
			// the key event that caused it
			if (!eventQueue_.empty())
			{
				eventQueue_.back().key_.appendChar( utf16chr );
			}
			else
			{
				// Hopefully this never happens, but shove the 
				// character in anyway using KEY_NONE.
				// Ignore the warning generated (and for some daft reason only
				// the warning) when it follows a key that's been ignored due
				// to being used in a GUI shortcut.
				if (!keyWasIgnored_)
				{
					WARNING_MSG( "InputDevices: Received CHAR message 0x%x with "
						"no corresponding key press (char=0x%x).\n",
						msg, wParam );
				}

				pushKeyEvent( KeyCode::KEY_NONE, 0, utf16chr, 
					modifiers(), lastMessageCursorPos( hWnd ) );
			}

			handled = true;
			break;
		}

	case WM_INPUT:
		{
			UINT dwSize = 0;
			RAWINPUT rid;

			if( GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, 
				&dwSize, sizeof(RAWINPUTHEADER)) != 0)
			{
				ERROR_MSG("InputDevices::WM_INPUT: GetRawInputData "
					"failed to get buffer size.\n");
				break;
			}

			if (dwSize > sizeof(rid))
			{
				// If this error occurs, the client needs to be rebuilt 
				// using a newer version of the Platform SDK.
				ERROR_MSG("GetRawInputData needs size %d which is "
					"greater than sizeof(RAWINPUT)==%d\n.", 
					dwSize, sizeof(RAWINPUT));
				MF_ASSERT(!"GetRawInputData buffer size mismatch.\n");
				break;
			}

			UINT res = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, 
				&rid, &dwSize, sizeof(RAWINPUTHEADER));

			if (res != dwSize)
			{
				ERROR_MSG("InputDevices::WM_INPUT: GetRawInputData failed "
					"to get data (GetLastError: %lu)\n", GetLastError());
				break;
			}

			if (rid.header.dwType == RIM_TYPEKEYBOARD)
			{
				RAWKEYBOARD* rkb = &rid.data.keyboard;

				/*
				DEBUG_MSG( "RIM_TYPEKEYBOARD: Message=%s, MakeCode=0x%x, "
					"Flags=0x%x, VKey=0x%x, Extra=%d\n",
					win32MsgToString(rkb->Message), rkb->MakeCode, 
					rkb->Flags, rkb->VKey, rkb->ExtraInformation );
				*/

				lastRawKeyMakeCode_ = rkb->MakeCode;

				// Use raw input for shift so we can differentiate 
				// between the two shifts.
				if ( rkb->VKey == VK_SHIFT )
				{				
					KeyCode::Key key = VKMap::fromRawKey( rkb );
					if (key == KeyCode::KEY_LSHIFT || 
						key == KeyCode::KEY_RSHIFT)
					{
						if (rkb->Message == WM_KEYUP || 
							rkb->Message == WM_SYSKEYUP)
						{
							forceKeyUp( KeyCode::KEY_PGDN, modifiers(), 
								lastMessageCursorPos( hWnd ) );
							forceKeyUp( KeyCode::KEY_PGUP, modifiers(), 
								lastMessageCursorPos( hWnd ) );
							forceKeyUp( KeyCode::KEY_HOME, modifiers(), 
								lastMessageCursorPos( hWnd ) );
							forceKeyUp( KeyCode::KEY_END,  modifiers(), 
								lastMessageCursorPos( hWnd ) );
						}

						pushKeyEventWin32( key, rkb->Message, modifiers(), 
							lastMessageCursorPos( hWnd ) );
					}
				}
				else if ( rkb->VKey == VK_SNAPSHOT )
				{
					KeyCode::Key key = VKMap::fromRawKey( rkb );
					pushKeyEventWin32( key, rkb->Message, modifiers(), 
						lastMessageCursorPos( hWnd ) );
				}
			}
			else if (rid.header.dwType == RIM_TYPEMOUSE)
			{
				RAWMOUSE* rm = &rid.data.mouse;

				/*
				DWORD dwCursorPos = GetMessagePos();
				POINT msgCursorPos = { 
					GET_X_LPARAM( dwCursorPos ), 
					GET_Y_LPARAM( dwCursorPos ) 
				};
				ScreenToClient(hWnd, &msgCursorPos);

				POINT cursorPos;
				GetCursorPos( &cursorPos );
				ScreenToClient( hWnd, &cursorPos );

				INFO_MSG("RIM_TYPEMOUSE: usFlags: %d, usButtonFlags: %d, "
					"usButtonData: %d, ulRawButtons: %d, lLastX: %d, "
					"lLastY: %d, ulExtraInformation: %d, "
					"GetCursorPos: (%d,%d), GetMessagePos: (%d,%d)\n", 
							rm->usFlags,
							rm->usButtonFlags,
							rm->usButtonData,
							rm->ulRawButtons,
							rm->lLastX,
							rm->lLastY,
							rm->ulExtraInformation,
							cursorPos.x, cursorPos.y,
							msgCursorPos.x, msgCursorPos.y );
				*/
				// Mouse movement
				long dz = rm->usButtonFlags & RI_MOUSE_WHEEL 
							? long(SHORT(rm->usButtonData)) : 0;

				if (( rm->usFlags & MOUSE_MOVE_ABSOLUTE ) != 0)
				{
					// Partial support for absolute devices 
					// (e.g. tablets or virtual machines)
					static LONG longMax = std::numeric_limits<LONG>::max();
					static LONG lastX = longMax;
					static LONG lastY = longMax;

					if (lastX == longMax || lastY == longMax)
					{
						lastX = rm->lLastX;
						lastY = rm->lLastY;
					}

					LONG dx = rm->lLastX - lastX;
					LONG dy = rm->lLastY - lastY;
					lastX = rm->lLastX;
					lastY = rm->lLastY;

					if (dx != 0 || dy != 0)
					{
						if (s_absDeviceDivider > 0)
						{
							dx = (LONG)((float)dx / s_absDeviceDivider);
							dy = (LONG)((float)dy / s_absDeviceDivider);
						}

						pushMouseEvent( dx, dy, dz, currentCursorPos( hWnd ) );
					}

					/*
					DEBUG_MSG( "MOUSE_MOVE_ABSOLUTE: lLastX=%d, "
						"lLastY=%d, dx=%d, dy=%d\n",
						rm->lLastX, rm->lLastY, dx, dy );
					*/
				}
				else // Must be MOUSE_INPUT_RELATIVE
				{
					if ( rm->lLastX != 0 || rm->lLastY != 0 || dz != 0 )
					{
						pushMouseEvent( rm->lLastX, rm->lLastY, dz, 
							currentCursorPos( hWnd ) );
					}
				}
			}

			result = 1;
			handled = true;
			break;
		}

	case WM_LBUTTONDOWN:
		{
			pushMouseButtonEvent( KeyCode::KEY_LEFTMOUSE, true, 
				modifiers(), lastMessageCursorPos( hWnd ) );			
			break;
		}

	case WM_LBUTTONUP:
		{
			pushMouseButtonEvent( KeyCode::KEY_LEFTMOUSE, false, 
				modifiers(), lastMessageCursorPos( hWnd ) );
			break;
		}

	case WM_RBUTTONDOWN:
		{
			pushMouseButtonEvent( KeyCode::KEY_RIGHTMOUSE, true,
				modifiers(), lastMessageCursorPos( hWnd ) );
			break;
		}
	case WM_RBUTTONUP:
		{
			pushMouseButtonEvent( KeyCode::KEY_RIGHTMOUSE, false, 
				modifiers(), lastMessageCursorPos( hWnd ) );
			break;
		}

	case WM_MBUTTONDOWN:
		{
			pushMouseButtonEvent( KeyCode::KEY_MIDDLEMOUSE, true,
				modifiers(), lastMessageCursorPos( hWnd ) );
			break;
		}
	case WM_MBUTTONUP:
		{
			pushMouseButtonEvent( KeyCode::KEY_MIDDLEMOUSE, false, 
				modifiers(), lastMessageCursorPos( hWnd ) );
			break;
		}

	case WM_XBUTTONDOWN:
		{
			KeyCode::Key key = GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? 
									KeyCode::KEY_MOUSE4 : KeyCode::KEY_MOUSE5;

			pushMouseButtonEvent( key, true, modifiers(), 
				lastMessageCursorPos( hWnd ) );
			break;
		}

	case WM_XBUTTONUP:
		{
			KeyCode::Key key = GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? 
									KeyCode::KEY_MOUSE4 : KeyCode::KEY_MOUSE5;

			pushMouseButtonEvent( key, false, modifiers(), 
				lastMessageCursorPos( hWnd ) );
			break;
		}

	case WM_MOUSEMOVE:
		{
			// Find the most recent mouse move event and 
			// attach our position to it.
			for ( EventQueue::reverse_iterator it = eventQueue_.rbegin();
				it != eventQueue_.rend(); it++ )
			{
				InputEvent& event = *it;
				if ( event.type_ == InputEvent::MOUSE )
				{
					POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
					Vector2 cursorPos = clientToClip( hWnd, pt );

					//DEBUG_MSG( "WM_MOUSEMOVE attached: "
					//		"x=%d, y=%d\n", pt.x, pt.y );

					MouseEvent& mevent = event.mouse_;
					mevent.fill( mevent.dx(), mevent.dy(), 
						mevent.dz(), cursorPos );
				}
			}

			break;
		}
	default:
		break;
	}

#if ENABLE_BW_IME
	if(!handled)
	{
		handled = IME::instance().handleWindowsMessage( 
			hWnd, msg, wParam, lParam, result );
	}
#endif

	return handled;
}


/**
 *	Get the key code for a key that is up an remove it from the raw key
 *	down map.
 *	@param wParam
 *	@param extendedBit
 *	@return the key code.
 */
KeyCode::Key InputDevices::getKeyCodeUp( WPARAM& wParam, bool extendedBit )
{
	BW_GUARD;

	KeyCode::Key key = KeyCode::KEY_NONE;

	BW::map<USHORT, KeyCode::Key>::iterator it = 
		rawKeyDownMap_.find( lastRawKeyMakeCode_ );

	if (it != rawKeyDownMap_.end())
	{
		key = it->second;
		rawKeyDownMap_.erase( it );
	}
	else
	{
		// Hopefully this never happens, but leaving commented out
		// DEBUG_MSG in case some strange keyboard driver pops its 
		// head up.
		/*
		DEBUG_MSG( "%s: received key up with no corresponding "
			"key in the raw key down map (VKey=0x%x, "
			"lastRawKeyMakeCode=%d).\n",
			win32MsgToString(msg), 
			(USHORT)wParam, lastRawKeyMakeCode_ );
		*/

		key = VKMap::fromVKey( (USHORT)wParam, extendedBit );
	}

	return key;
}


/**
 *	Get the key code for a key that is down and add it to the raw key down map.
 *	@param wParam
 *	@param extendedBit
 *	@return the key code.
 */
KeyCode::Key InputDevices::getKeyCodeDown( WPARAM& wParam, bool extendedBit )
{
	BW_GUARD;

	KeyCode::Key key = KeyCode::KEY_NONE;

	BW::map<USHORT, KeyCode::Key>::iterator it = 
		rawKeyDownMap_.find( lastRawKeyMakeCode_ );

	// If we are already in the keydown map, we're auto-repeating. 
	// To make sure nothing changes on us half way through a key 
	// press, use the initial key mapping.
	if (it != rawKeyDownMap_.end())
	{
		key = it->second;
	}
	else
	{
		key = VKMap::fromVKey( (USHORT)wParam, extendedBit );
		rawKeyDownMap_[ lastRawKeyMakeCode_ ] = key;
	}

	return key;
}


/**
 *	Captures the Windows cursor if any of the mouse buttons are
 *	currently pressed, and releases it if none are pressed.
 */
void InputDevices::updateCursorCapture( HWND hWnd )
{
	BW_GUARD;
	bool mouseDown = false;

	for( int i = KeyCode::KEY_MINIMUM_MOUSE; 
		 i < KeyCode::KEY_MAXIMUM_MOUSE; i++ )
	{
		if ( keyStatesInternal_.isKeyDown( (KeyCode::Key)i ) )
		{
			mouseDown = true;
			break;
		}
	}

	if (mouseDown)
	{
		SetCapture( hWnd );
	}
	else
	{
		ReleaseCapture();
	}
}

/**
 *	Notifies the input system when the input focus changes to or from
 *	the input window.
 */
void InputDevices::setFocus( bool state, InputHandler * handler )
{
	BW_GUARD;
	bool * pFocus = pInstance() ? &pInstance()->focus_ : &focus_;
	bool losingFocus = (state == false && *pFocus == true);
	*pFocus = state;
	if (losingFocus)
	{
		//DEBUG_MSG( "InputDevices losing focus.\n" );

		// We are losing focus, so we don't want the currently
		// held down keys to remain down.  Especially if the key
		// up event is going to be lost to another application.

		// Send all down keys an up event
		if (handler != NULL)
		{
			for (int32 i=0; i < KeyCode::NUM_KEYS; i++)
			{
				KeyCode::Key k = (KeyCode::Key)i;
				if (pInstance())
				{
					if (keyStates().isKeyDown( k ))
					{
						KeyEvent ke;
						ke.fill( k, -1, 0, 
							currentCursorPos( instance().hWnd_ ) );
						keyStates().updateKey( k, false );
						handler->handleKeyEvent( ke );
					}
				}
			}
		}

		if (pInstance())
		{
			// Clear anything in the event queue, because if it didn't 
			// get pumped out last frame, we don't want to know about it 
			// when we regain focus.
			instance().eventQueue_.resize(0);

			// Now make sure the keyboard state has been reset.
			instance().resetKeyStates();
		}

		ReleaseCapture();
	}	
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
	BW_GUARD;
	HRESULT hr;


	//process any lost joystick button state
	if ( (mask & JOY_DATA_LOST) && joystick_.pDIJoystick() )
	{
		DIJOYSTATE joyState;
		ZeroMemory( &joyState, sizeof( joyState ) );

		hr = joystick_.pDIJoystick()->GetDeviceState( 
			sizeof( joyState ), &joyState );

		if ( SUCCEEDED( hr ) )
		{
			//success.  iterate through valid joystick codes and check the state
			for ( int k = KeyCode::KEY_MINIMUM_JOY;
					k != KeyCode::KEY_MAXIMUM_JOY;
					k++ )
			{
				//success.  iterate through valid key codes and check the state
				for ( int k = KeyCode::KEY_MINIMUM_JOY;
						k != KeyCode::KEY_MAXIMUM_JOY;
						k++ )
				{
					int idx = k - KeyCode::KEY_MINIMUM_JOY;
					bool state = 
						(joyState.rgbButtons[ idx ] & 0x80) ? true : false;

					KeyEvent event;
					event.fill( static_cast<KeyCode::Key>(k), 
								state,
								this->modifiers(), 
								currentCursorPos( hWnd_ ) );

					//pass event to handler if there is a mismatch between
					//immediate device state and our recorded state
					if ( event.isKeyDown() != 
						(keyStates_.isKeyDown( event.key() ) ? true : false ) )
					{
						keyStates_.setKeyStateNoRepeat( 
							event.key(), event.isKeyDown() );
						handler.handleKeyEvent( event );
					}
				}
			}
			lostData_ &= ~JOY_DATA_LOST;
		}
		else
		{
			DEBUG_MSG( "InputDevices::handleLostData::GetDeviceState[joystick] "
				"failed  %lx\n", hr );
		}
	}
}

/**
 *	Insert a key event into the queue, based on the given windows key event.
 */
void InputDevices::pushKeyEventWin32( KeyCode::Key key, USHORT msg, 
									  uint32 mods, const Vector2& cursorPos )
{
	BW_GUARD;
	bool isDown = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
	bool currentlyDown = keyStatesInternal_.isKeyDown(key);

	/*
	DEBUG_MSG( "pushKeyEventWin32: key=%s, msg=%s, mods=%x, "
		"currentlyDown=%d, stack=%s\n",
		KeyCode::keyToString(key), win32MsgToString(msg), mods,
		currentlyDown, StackTracker::buildReport().c_str() );
	*/

	if (!isDown && !currentlyDown)
		return;

	int32 state = keyStatesInternal_.updateKey( key, isDown );	
	pushKeyEvent( key, state, mods, cursorPos );	
}

/**
 *	Insert a key event into the queue.
 */
void InputDevices::pushKeyEvent( KeyCode::Key key, int32 state, 
								 uint32 mods, const Vector2& cursorPos )
{
	BW_GUARD;
	InputEvent event;
	event.type_ = InputEvent::KEY;
	event.key_.fill( key, state, mods, cursorPos );
	eventQueue_.push_back( event );
}

/**
 *	Insert a key event into the queue, with an associated character string.
 */
void InputDevices::pushKeyEvent( KeyCode::Key key, int32 state, 
								 const wchar_t *ch, uint32 mods, 
								 const Vector2& cursorPos )
{
	BW_GUARD;
	InputEvent event;
	event.type_ = InputEvent::KEY;
	event.key_.fill( key, state, ch, mods, cursorPos );
	eventQueue_.push_back( event );
}

/**
 *	Insert a mouse button event into the queue.
 */
void InputDevices::pushMouseButtonEvent( KeyCode::Key key, bool isDown, 
										 uint32 mods,
										 const Vector2& cursorPos)
{
	BW_GUARD;
	if (!isDown && !keyStatesInternal_.isKeyDown(key))
		return;

	int32 state = keyStatesInternal_.updateKey( key, isDown );	
	pushKeyEvent( key, state, mods, cursorPos );
	updateCursorCapture( hWnd_ );
}

/**
 *	Insert a mouse event into the queue. If accumulate is true and the
 *	previous event was also a mouse event, then it accumulates the new 
 *	mouse movement onto the previous mouse movement.
 */
void InputDevices::pushMouseEvent( long dx, long dy, long dz, 
								const Vector2& cursorPos, bool accumulate )
{
	BW_GUARD;
	if ( !accumulate || 
		  eventQueue_.empty() || 
		  eventQueue_.back().type_ != InputEvent::MOUSE )
	{
		InputEvent event;
		event.type_ = InputEvent::MOUSE;
		event.mouse_.fill( dx, dy, dz, cursorPos );
		eventQueue_.push_back( event );
	}
	else
	{
		MouseEvent& mevent = eventQueue_.back().mouse_;
		mevent.fill( 
			mevent.dx() + dx, mevent.dy() + dy,
			mevent.dz() + dz, cursorPos );
	}
}

/**
 *  Send key events to force all keys up.
 */
void InputDevices::forceKeysUp()
{
	BW_GUARD;
	for ( size_t i = KeyCode::KEY_MINIMUM_KEY; i < KeyCode::NUM_KEYS; ++i )
	{
		KeyCode::Key key = static_cast<KeyCode::Key>( i );
		if (this->isKeyDown( key ))
		{
			this->forceKeyUp( key,
				this->modifiers(),
				lastMessageCursorPos( hWnd_ ) );
		}
	}
}


void InputDevices::forceKeyUp( KeyCode::Key key, 
							  uint32 mods, 
							  const Vector2& cursorPos )
{
	BW_GUARD;
	pushKeyEventWin32( key, WM_KEYUP, mods, cursorPos );
}


/**
 *	Do not process the next key press of the given key.
 *	HACK to ignore a key press that was already processed by a hook.
 *	eg. WorldEditor shortcut key
 */
void InputDevices::ignoreNextKeyDown( KeyCode::Key key )
{
	BW_GUARD;
	ignoringKey_ = key;
}

/**
 *	Insert an IME character event into the event queue. This gets posted off to
 *	the input handler as a KeyEvent with the key code set to KEY_IME_CHAR.
 */
/*static*/ void InputDevices::pushIMECharEvent( wchar_t* chs )
{
	BW_GUARD;
	if (InputDevices::pInstance())
	{
		HWND hWnd = IME::instance().hWnd();
		InputDevices::instance().pushKeyEvent( KeyCode::KEY_IME_CHAR, 
			0, chs, 0, lastMessageCursorPos(hWnd) );
	}
}


// -----------------------------------------------------------------------------
// Section: Joystick
// -----------------------------------------------------------------------------

/**
 *	Structure to hold the Direct Input callback objects.
 */
struct EnumJoysticksCallbackData
{
	IDirectInputDevice8 ** ppDIJoystick;
	IDirectInput8 * pDirectInput;
};


/*
 * Called once for each enumerated joystick. If we find one, create a device
 * interface on it so we can play with it.
 */
BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE * pdidInstance,
								void * pData )
{
	BW_GUARD;
	EnumJoysticksCallbackData * pCallbackData =
		reinterpret_cast<EnumJoysticksCallbackData *>( pData );

	// Obtain an interface to the enumerated joystick.
	HRESULT hr = pCallbackData->pDirectInput->CreateDevice(
			GUID_Joystick,
			pCallbackData->ppDIJoystick,
			NULL );

	// If it failed, then we can't use this joystick. (Maybe the user unplugged
	// it while we were in the middle of enumerating it.)

	if( FAILED(hr) )
		return DIENUM_CONTINUE;


	// Stop enumeration. Note: we're just taking the first joystick we get. You
	// could store all the enumerated joysticks and let the user pick.
	return DIENUM_STOP;
}




/*
 *	Callback function for enumerating the axes on a joystick.
 */
BOOL CALLBACK EnumAxesCallback( const DIDEVICEOBJECTINSTANCE * pdidoi,
							void * pJoystickAsVoid )
{
	BW_GUARD;
	Joystick * pJoystick = reinterpret_cast<Joystick *>( pJoystickAsVoid );

	DIPROPRANGE diprg;
	diprg.diph.dwSize       = sizeof(DIPROPRANGE);
	diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.diph.dwObj        = pdidoi->dwOfs; // Specify the enumerated axis
	diprg.lMin              = -DIRECT_INPUT_AXIS_MAX;
	diprg.lMax              = +DIRECT_INPUT_AXIS_MAX;

	// Set the range for the axis
	if( FAILED( pJoystick->pDIJoystick()->
		SetProperty( DIPROP_RANGE, &diprg.diph ) ) )
	{
		return DIENUM_STOP;
	}

	// Set the UI to reflect what axes the joystick supports.
	// These are PlayStation Pelican adapter mappings,
	// see Joystick::updateFromJoystickDevice
	if (pdidoi->dwOfs == DIJOFS_X)
	{
		pJoystick->getAxis( AxisEvent::AXIS_LX ).enabled( true );
	}
	else if (pdidoi->dwOfs == DIJOFS_Y)
	{
		pJoystick->getAxis( AxisEvent::AXIS_LY ).enabled( true );
	}
	else if (pdidoi->dwOfs == DIJOFS_Z)
	{
		pJoystick->getAxis( AxisEvent::AXIS_RX ).enabled( true );
	}
	else if (pdidoi->dwOfs == DIJOFS_RZ)
	{
		pJoystick->getAxis( AxisEvent::AXIS_RY ).enabled( true );
	}

	return DIENUM_CONTINUE;
}



/**
 *	This method initialises the Joystick.
 */
bool Joystick::init( IDirectInput8 * pDirectInput,
					void * hWnd )
{
	BW_GUARD;
	EnumJoysticksCallbackData callbackData =
	{
		&pDIJoystick_,
		pDirectInput
	};

	hWnd_ = (HWND)hWnd;

	// Look for a simple joystick we can use for this sample program.
	if ( FAILED( pDirectInput->EnumDevices( DI8DEVCLASS_GAMECTRL,
		EnumJoysticksCallback,
		&callbackData,
		DIEDFL_ATTACHEDONLY ) ) )
	{
		return false;
	}

	// Make sure we got a joystick
	if( NULL == pDIJoystick_ )
	{
		DEBUG_MSG( "Joystick::init: Joystick not found\n" );

		return false;
	}

	// Set the data format to "simple joystick" - a predefined data format
	//
	// A data format specifies which controls on a device we are interested in,
	// and how they should be reported. This tells DInput that we will be
	// passing a DIJOYSTATE structure to IDirectInputDevice::GetDeviceState().

	if ( FAILED( pDIJoystick_->SetDataFormat( &c_dfDIJoystick ) ) )
	{
		return false;
	}

	// Set the cooperative level to let DInput know how this device should
	// interact with the system and with other DInput applications.

	if ( FAILED( pDIJoystick_->SetCooperativeLevel( hWnd_,
						DISCL_EXCLUSIVE|DISCL_FOREGROUND ) ) )
	{
		return false;
	}

	// IMPORTANT STEP TO USE BUFFERED DEVICE DATA!
	//
	// DirectInput uses unbuffered I/O (buffer size = 0) by default.
	// If you want to read buffered data, you need to set a nonzero
	// buffer size.
	//
	// Set the buffer size to JOYSTICK_BUFFER_SIZE elements.
	//
	// The buffer size is a DWORD property associated with the device.
	DIPROPDWORD dipdw;

	dipdw.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj = 0;
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData = JOYSTICK_BUFFER_SIZE;

	if (FAILED( pDIJoystick_->SetProperty( DIPROP_BUFFERSIZE, &dipdw.diph ) ) )
	{
		return false;
	}

	// Determine the capabilities of the device.

	DIDEVCAPS diDevCaps;
	diDevCaps.dwSize = sizeof( DIDEVCAPS );
	if ( SUCCEEDED( pDIJoystick_->GetCapabilities( &diDevCaps ) ) )
	{
		if ( diDevCaps.dwFlags & DIDC_POLLEDDATAFORMAT )
		{
			DEBUG_MSG( "Joystick::init: Polled data format\n" );
		}
		else
		{
			DEBUG_MSG( "Joystick::init: Not Polled data format\n" );
		}

		if ( diDevCaps.dwFlags & DIDC_POLLEDDEVICE )
		{
			DEBUG_MSG( "Joystick::init: Polled device\n" );
		}
		else
		{
			DEBUG_MSG( "Joystick::init: Not Polled device\n" );
		}
	}
	else
	{
		DEBUG_MSG( "Joystick::init: Did not get capabilities\n" );
	}

	// Enumerate the axes of the joyctick and set the range of each axis. Note:
	// we could just use the defaults, but we're just trying to show an example
	// of enumerating device objects (axes, buttons, etc.).

	pDIJoystick_->EnumObjects( EnumAxesCallback, (VOID*)this, DIDFT_AXIS );

	return true;
}


/*
 *	Simple helper method to convert from the joystick axis coordinates that
 *	DirectInput returns to a float in the range [-1, 1].
 */
static inline float scaleFromDIToUnit( int value )
{
	BW_GUARD;
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
 *	This method updates this object from a joystick device.
 */
bool Joystick::updateFromJoystickDevice()
{
	BW_GUARD;
	if ( pDIJoystick_ )
	{
		const int MAX_ATTEMPTS = 10;
		int attempts = 0;
		HRESULT     hr;
		DIJOYSTATE  js = {};          // DInput joystick state

		do
		{
			// Poll the device to read the current state
			hr = pDIJoystick_->Poll();

			if ( SUCCEEDED( hr ) )
			{
				// Get the input's device state
				hr = pDIJoystick_->GetDeviceState( sizeof(DIJOYSTATE), &js );
			}

			if (hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST)
			{
				// DInput is telling us that the input stream has been
				// interrupted. We aren't tracking any state between polls, so
				// we don't have any special reset that needs to be done. We
				// just re-acquire and try again.

				HRESULT localHR = pDIJoystick_->Acquire();

				if (FAILED( localHR ))
				{
					// DEBUG_MSG( "Joystick::updateFromJoystickDevice: "
					//		"Acquire failed\n" );
					return false;
				}
			}
		}
		while ((hr != DI_OK) && (++attempts < MAX_ATTEMPTS));

		if( FAILED(hr) )
			return false;

		// PlayStation Pelican adapter settings
		// We use a math-like not screen-like coordinate system here
		this->getAxis( AxisEvent::AXIS_LX ).value( scaleFromDIToUnit( js.lX ) );
		this->getAxis( AxisEvent::AXIS_LY ).value(-scaleFromDIToUnit( js.lY ) );

		this->getAxis( AxisEvent::AXIS_RX ).value( scaleFromDIToUnit( js.lZ ) );
		this->getAxis( AxisEvent::AXIS_RY ).value(-scaleFromDIToUnit( js.lRz) );

/*
		// Point of view
		if( g_diDevCaps.dwPOVs >= 1 )
		{
			bw_snwprintf( strText, sizeof(strText)/sizeof(wchar_t), "%ld", js.rgdwPOV[0] );
			SetWindowText( GetDlgItem( hDlg, IDC_POV ), strText );
		}

		// Fill up text with which buttons are pressed
		str = strText;
		for( int i = 0; i < 32; i++ )
		{
			if ( js.rgbButtons[i] & 0x80 )
				str += bw_snprintf( str, sizeof(str), "%02d ", i );
		}
		*str = 0;   // Terminate the string

		SetWindowText( GetDlgItem( hDlg, IDC_BUTTONS ), strText );
*/
	}

	return true;
}


/**
 *	Mapping between direct input joystick button number and
 *	our joystick key events.
 */
static const KeyCode::Key s_joyKeys_PlayStation[32] =
{
	KeyCode::KEY_JOYTRIANGLE,
	KeyCode::KEY_JOYCIRCLE,
	KeyCode::KEY_JOYCROSS,
	KeyCode::KEY_JOYSQUARE,
	KeyCode::KEY_JOYL2,
	KeyCode::KEY_JOYR2,
	KeyCode::KEY_JOYL1,
	KeyCode::KEY_JOYR1,
	KeyCode::KEY_JOYSELECT,
	KeyCode::KEY_JOYSTART,
	KeyCode::KEY_JOYARPUSH,
	KeyCode::KEY_JOYALPUSH,
	KeyCode::KEY_JOYDUP,
	KeyCode::KEY_JOYDRIGHT,
	KeyCode::KEY_JOYDDOWN,
	KeyCode::KEY_JOYDLEFT,

	KeyCode::KEY_JOY16,
	KeyCode::KEY_JOY17,
	KeyCode::KEY_JOY18,
	KeyCode::KEY_JOY19,
	KeyCode::KEY_JOY20,
	KeyCode::KEY_JOY21,
	KeyCode::KEY_JOY22,
	KeyCode::KEY_JOY23,
	KeyCode::KEY_JOY24,
	KeyCode::KEY_JOY25,
	KeyCode::KEY_JOY26,
	KeyCode::KEY_JOY27,
	KeyCode::KEY_JOY28,
	KeyCode::KEY_JOY29,
	KeyCode::KEY_JOY30,
	KeyCode::KEY_JOY31
};


/**
 *	This methods processes the pending joystick events.
 */
bool Joystick::processEvents( InputHandler& handler,
							 KeyStates& keyStates,
							 bool * pLostDataFlag )
{
	BW_GUARD;
	if (!pDIJoystick_)
		return true;

	Vector2 cursorPosition = currentCursorPos( hWnd_ );


	DIDEVICEOBJECTDATA didod[ JOYSTICK_BUFFER_SIZE ];
	DWORD dwElements = 0;
	HRESULT hr;

	dwElements = JOYSTICK_BUFFER_SIZE;
	hr = pDIJoystick_->GetDeviceData( sizeof(DIDEVICEOBJECTDATA),
										didod, &dwElements, 0 );
	switch (hr) {

	case DI_OK:
		break;

	case DI_BUFFEROVERFLOW:
//		DEBUG_MSG( "Joystick::processEvents: joystick buffer overflow\n" );

		if ( pLostDataFlag )
			*pLostDataFlag = true;

		break;

	case DIERR_INPUTLOST:
	case DIERR_NOTACQUIRED:
//		DEBUG_MSG( "Joystick::processEvents: input not acquired, acquiring\n" );
		hr = pDIJoystick_->Acquire();
		if (FAILED(hr)) {
			DEBUG_MSG( "Joystick::processEvents: acquire failed\n" );
			return false;
		}
		dwElements = JOYSTICK_BUFFER_SIZE;
		hr = pDIJoystick_->GetDeviceData( sizeof(DIDEVICEOBJECTDATA),
										didod, &dwElements, 0 );
		if ( pLostDataFlag )
			*pLostDataFlag = true;

		break;

	default:
		DEBUG_MSG( "Joystick::processEvents: unhandled joystick error\n" );
		return false;
	}


	for (DWORD i = 0; i < dwElements; i++)
	{
		DWORD offset = didod[ i ].dwOfs;

		if (DIJOFS_BUTTON0 <= offset && offset <= DIJOFS_BUTTON31)
		{
			this->generateKeyEvent(
				!!(didod[ i ].dwData & 0x80),
				s_joyKeys_PlayStation[ offset - DIJOFS_BUTTON0 ],
				handler,
				keyStates,
				cursorPosition );
		}
		else
		{
			// TODO:PM Not handling joystick events currently.
			// It is all state based.
		}
	}

	this->generateCommonEvents( handler, keyStates, cursorPosition );

	return true;
}

BW_END_NAMESPACE

//input.cpp
