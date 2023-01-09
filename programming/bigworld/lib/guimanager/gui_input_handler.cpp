#include "pch.hpp"
#include "gui_input_handler.hpp"
#include "gui_manager.hpp"
#include "input/input.hpp"
#include <algorithm>
#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

BW::map<char, int> BigworldInputDevice::keyMap_;
BW::map<BW::string, int> BigworldInputDevice::nameMap_;
bool BigworldInputDevice::lastKeyDown[ KeyCode::NUM_KEYS ];

bool BigworldInputDevice::down( const bool* keyDownTable, char ch )
{
	BW_GUARD;

	return ( keyMap_.find( ch ) != keyMap_.end() && keyDownTable[ keyMap_[ ch ] ] );
}

bool BigworldInputDevice::down( const bool* keyDownTable, const BW::string& key )
{
	BW_GUARD;

	BW::string left, right;
	if( _strcmpi(key.c_str(), "WIN" ) == 0 )
		left = "LWIN", right = "RWIN";
	if( _strcmpi(key.c_str(), "CTRL" ) == 0 )
		left = "LCTRL", right = "RCTRL";
	if( _strcmpi(key.c_str(), "SHIFT" ) == 0 )
		left = "LSHIFT", right = "RSHIFT";
	if( _strcmpi(key.c_str(), "ALT" ) == 0 )
		left = "LALT", right = "RALT";
	return left.empty()																	?
		( nameMap_.find( key ) != nameMap_.end() && keyDownTable[ nameMap_[ key ] ] )	:
		( keyDownTable[ nameMap_[ left ] ] || keyDownTable[ nameMap_[ right ] ] );
}

BigworldInputDevice::BigworldInputDevice( const bool* keyDownTable )
	:keyDownTable_( keyDownTable )
{
	BW_GUARD;

	if( keyMap_.empty() )
	{
		keyMap_[ '0' ] = KeyCode::KEY_0, keyMap_[ '1' ] = KeyCode::KEY_1, keyMap_[ '2' ] = KeyCode::KEY_2,
		keyMap_[ '3' ] = KeyCode::KEY_3, keyMap_[ '4' ] = KeyCode::KEY_4, keyMap_[ '5' ] = KeyCode::KEY_5,
		keyMap_[ '6' ] = KeyCode::KEY_6, keyMap_[ '7' ] = KeyCode::KEY_7, keyMap_[ '8' ] = KeyCode::KEY_8,
		keyMap_[ '9' ] = KeyCode::KEY_9;

		keyMap_[ 'A' ] = KeyCode::KEY_A, keyMap_[ 'B' ] = KeyCode::KEY_B, keyMap_[ 'C' ] = KeyCode::KEY_C,
		keyMap_[ 'D' ] = KeyCode::KEY_D, keyMap_[ 'E' ] = KeyCode::KEY_E, keyMap_[ 'F' ] = KeyCode::KEY_F,
		keyMap_[ 'G' ] = KeyCode::KEY_G, keyMap_[ 'H' ] = KeyCode::KEY_H, keyMap_[ 'I' ] = KeyCode::KEY_I,
		keyMap_[ 'J' ] = KeyCode::KEY_J, keyMap_[ 'K' ] = KeyCode::KEY_K, keyMap_[ 'L' ] = KeyCode::KEY_L,
		keyMap_[ 'M' ] = KeyCode::KEY_M, keyMap_[ 'N' ] = KeyCode::KEY_N, keyMap_[ 'O' ] = KeyCode::KEY_O,
		keyMap_[ 'P' ] = KeyCode::KEY_P, keyMap_[ 'Q' ] = KeyCode::KEY_Q, keyMap_[ 'R' ] = KeyCode::KEY_R,
		keyMap_[ 'S' ] = KeyCode::KEY_S, keyMap_[ 'T' ] = KeyCode::KEY_T, keyMap_[ 'U' ] = KeyCode::KEY_U,
		keyMap_[ 'V' ] = KeyCode::KEY_V, keyMap_[ 'W' ] = KeyCode::KEY_W, keyMap_[ 'X' ] = KeyCode::KEY_X,
		keyMap_[ 'Y' ] = KeyCode::KEY_Y, keyMap_[ 'Z' ] = KeyCode::KEY_Z;

		keyMap_[ ',' ] = KeyCode::KEY_COMMA, keyMap_[ '.' ] = KeyCode::KEY_PERIOD, keyMap_[ '/' ] = KeyCode::KEY_SLASH,
		keyMap_[ ';' ] = KeyCode::KEY_SEMICOLON, keyMap_[ '\'' ] = KeyCode::KEY_APOSTROPHE, keyMap_[ '[' ] = KeyCode::KEY_LBRACKET,
		keyMap_[ ']' ] = KeyCode::KEY_RBRACKET, keyMap_[ '`' ] = KeyCode::KEY_GRAVE, keyMap_[ '-' ] = KeyCode::KEY_MINUS,
		keyMap_[ '=' ] = KeyCode::KEY_EQUALS, keyMap_[ '\\' ] = KeyCode::KEY_BACKSLASH;

		keyMap_[ ' ' ] = KeyCode::KEY_SPACE;

		nameMap_[ "LSHIFT" ] = KeyCode::KEY_LSHIFT, nameMap_[ "RSHIFT" ] = KeyCode::KEY_RSHIFT;
		nameMap_[ "LCTRL" ] = KeyCode::KEY_LCONTROL, nameMap_[ "RCTRL" ] = KeyCode::KEY_RCONTROL;
		nameMap_[ "LALT" ] = KeyCode::KEY_LALT, nameMap_[ "RALT" ] = KeyCode::KEY_RALT;
		nameMap_[ "LWIN" ] = KeyCode::KEY_LWIN, nameMap_[ "RWIN" ] = KeyCode::KEY_RWIN;
		nameMap_[ "MENU" ] = KeyCode::KEY_APPS;

		nameMap_[ "CAPSLOCK" ] = KeyCode::KEY_CAPSLOCK;
		nameMap_[ "SCROLLLOCK" ] = KeyCode::KEY_SCROLL;
		nameMap_[ "NUMLOCK" ] = KeyCode::KEY_NUMLOCK;

		nameMap_[ "NUM0" ] = KeyCode::KEY_NUMPAD0, nameMap_[ "NUM1" ] = KeyCode::KEY_NUMPAD1,
		nameMap_[ "NUM2" ] = KeyCode::KEY_NUMPAD2, nameMap_[ "NUM3" ] = KeyCode::KEY_NUMPAD3,
		nameMap_[ "NUM4" ] = KeyCode::KEY_NUMPAD4, nameMap_[ "NUM5" ] = KeyCode::KEY_NUMPAD5,
		nameMap_[ "NUM6" ] = KeyCode::KEY_NUMPAD6, nameMap_[ "NUM7" ] = KeyCode::KEY_NUMPAD7,
		nameMap_[ "NUM8" ] = KeyCode::KEY_NUMPAD8, nameMap_[ "NUM9" ] = KeyCode::KEY_NUMPAD9;

		nameMap_[ "NUMMINUS" ] = KeyCode::KEY_NUMPADMINUS, nameMap_[ "NUMPERIOD" ] = KeyCode::KEY_NUMPADPERIOD,
		nameMap_[ "NUMADD" ] = KeyCode::KEY_ADD, nameMap_[ "NUMSTAR" ] = KeyCode::KEY_NUMPADSTAR,
		nameMap_[ "NUMENTER" ] = KeyCode::KEY_NUMPADENTER, nameMap_[ "NUMSLASH" ] = KeyCode::KEY_NUMPADSLASH,
		nameMap_[ "NUMRETURN" ] = KeyCode::KEY_NUMPADENTER;

		nameMap_[ "RETURN" ] = KeyCode::KEY_RETURN, nameMap_[ "ENTER" ] = KeyCode::KEY_RETURN,
		nameMap_[ "TAB" ] = KeyCode::KEY_TAB, nameMap_[ "ESCAPE" ] = KeyCode::KEY_ESCAPE;

		nameMap_[ "F1" ] = KeyCode::KEY_F1, nameMap_[ "F2" ] = KeyCode::KEY_F2,
		nameMap_[ "F3" ] = KeyCode::KEY_F3, nameMap_[ "F4" ] = KeyCode::KEY_F4,
		nameMap_[ "F5" ] = KeyCode::KEY_F5, nameMap_[ "F6" ] = KeyCode::KEY_F6,
		nameMap_[ "F7" ] = KeyCode::KEY_F7, nameMap_[ "F8" ] = KeyCode::KEY_F8,
		nameMap_[ "F9" ] = KeyCode::KEY_F9, nameMap_[ "F10" ] = KeyCode::KEY_F10,
		nameMap_[ "F11" ] = KeyCode::KEY_F11, nameMap_[ "F12" ] = KeyCode::KEY_F12;

		nameMap_[ "UP" ] = KeyCode::KEY_UPARROW, nameMap_[ "DOWN" ] = KeyCode::KEY_DOWNARROW,
		nameMap_[ "LEFT" ] = KeyCode::KEY_LEFTARROW, nameMap_[ "RIGHT" ] = KeyCode::KEY_RIGHTARROW;

		nameMap_[ "INSERT" ] = KeyCode::KEY_INSERT, nameMap_[ "HOME" ] = KeyCode::KEY_HOME,
		nameMap_[ "PAGEUP" ] = KeyCode::KEY_PGUP, nameMap_[ "PAGEDOWN" ] = KeyCode::KEY_PGDN;
		nameMap_[ "DELETE" ] = KeyCode::KEY_DELETE, nameMap_[ "END" ] = KeyCode::KEY_END,
		nameMap_[ "BACKSPACE" ] = KeyCode::KEY_BACKSPACE;
	}
}

bool BigworldInputDevice::isKeyDown( const BW::string& key )
{
	BW_GUARD;

	bool result = false;


	BW::string shortcut = key;
	shortcut.erase( std::remove( shortcut.begin(), shortcut.end(), ' ' ),
		shortcut.end() );

	if (shortcut.length() > 0)
		_strupr( &shortcut[0] );

	/***********************************************************************
	recognised keys:
	SHIFT, LSHIFT, RSHIFT, CTRL, LCTRL, RCTRL, ALT, LALT, RALT,
	WIN, LWIN, RWIN, MENU,
	CAPSLOCK, SCROLLLOCK, NUMLOCK,
	NUM0, NUM1, ..., NUM9, NUMMINUS, NUMPERIOD, NUMADD, NUMSTAR, NUMENTER, NUMSLASH,
	RETURN, ENTER, TAB, ESCAPE,
	F1, F2, F3, F4, ..., F12,
	UP, DOWN, LEFT, RIGHT,
	INSERT, HOME, PAGEUP, PAGEDOWN, DELETE, END, BACKSPACE
	***********************************************************************/
	BW::set<BW::string> modifiers;
	BW::string keyname;
	while( shortcut.find( '+' ) != shortcut.npos )
	{
		modifiers.insert( shortcut.substr( 0, shortcut.find( '+' ) ) );
		shortcut = shortcut.substr( shortcut.find( '+' ) + 1 );
	}
	keyname = shortcut;
	if( keyname.size() && shortcut.size() )
	{
		// 1. is key down?
		bool keydown = down( keyDownTable_, keyname );
		if( !keydown && keyname.size() == 1 && down( keyDownTable_, keyname[0] ) )
			keydown = true;
		// 2. was key up?
		bool lastKeydown = down( lastKeyDown, keyname );
		if( !lastKeydown && keyname.size() == 1 && down( lastKeyDown, keyname[0] ) )
			lastKeydown = true;
		if( keydown && !lastKeydown )
		{
			size_t totalKeyNum = std::count( keyDownTable_, keyDownTable_ + KeyCode::KEY_MAXIMUM_KEY, true );

			if( totalKeyNum == modifiers.size() + 1 )
			{
				result = true;
				for( BW::set<BW::string>::iterator iter = modifiers.begin();
					iter != modifiers.end(); ++iter )
				{
					if( !down( keyDownTable_, *iter ) )
						return false;
				}
			}
		}
	}
	return result;
}

void BigworldInputDevice::refreshKeyDownState( const bool* keyDown )
{
	BW_GUARD;

	memcpy( lastKeyDown, keyDown, sizeof( lastKeyDown[ 0 ] ) * KeyCode::NUM_KEYS );
}

BW::map<char, int> Win32InputDevice::keyMap_;
BW::map<BW::string, int> Win32InputDevice::nameMap_;
char Win32InputDevice::ch_;

bool Win32InputDevice::down( char ch )
{
	BW_GUARD;

	return keyMap_.find( ch ) != keyMap_.end() && ch_ == keyMap_[ ch ];
}

bool Win32InputDevice::down( const BW::string& key )
{
	BW_GUARD;

	BW::string left, right;
	if( _strcmpi(key.c_str(), "WIN" ) == 0 )
		return ( GetKeyState( VK_LWIN ) & 0x8000 ) != 0 || ( GetKeyState( VK_RWIN ) & 0x8000 ) != 0;
	else if( _strcmpi(key.c_str(), "LWIN" ) == 0 )
		return ( GetKeyState( VK_LWIN ) & 0x8000 ) != 0;
	else if( _strcmpi(key.c_str(), "RWIN" ) == 0 )
		return ( GetKeyState( VK_RWIN ) & 0x8000 ) != 0;
	else if( _strcmpi(key.c_str(), "CTRL" ) == 0 )
		return ( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0;
	else if( _strcmpi(key.c_str(), "LCTRL" ) == 0 )
		return ( GetKeyState( VK_LCONTROL ) & 0x8000 ) != 0;
	else if( _strcmpi(key.c_str(), "RCTRL" ) == 0 )
		return ( GetKeyState( VK_RCONTROL ) & 0x8000 ) != 0;
	else if( _strcmpi(key.c_str(), "SHIFT" ) == 0 )
		return ( GetKeyState( VK_SHIFT ) & 0x8000 ) != 0;
	else if( _strcmpi(key.c_str(), "LSHIFT" ) == 0 )
		return ( GetKeyState( VK_LSHIFT ) & 0x8000 ) != 0;
	else if( _strcmpi(key.c_str(), "RSHIFT" ) == 0 )
		return ( GetKeyState( VK_RSHIFT ) & 0x8000 ) != 0;
	else if( _strcmpi(key.c_str(), "ALT" ) == 0 )
		return ( GetKeyState( VK_LMENU ) & 0x8000 ) != 0 || ( GetKeyState( VK_RMENU ) & 0x8000 ) != 0;
	else if( _strcmpi(key.c_str(), "LALT" ) == 0 )
		return ( GetKeyState( VK_LMENU ) & 0x8000 ) != 0;
	else if( _strcmpi(key.c_str(), "RALT" ) == 0 )
		return ( GetKeyState( VK_RMENU ) & 0x8000 ) != 0;
	else if( nameMap_.find( key ) != nameMap_.end() )
		return nameMap_[ key ] == ch_;
	return false;
}

Win32InputDevice::Win32InputDevice( char ch )
{
	BW_GUARD;

	ch_ = ch;
	if( keyMap_.empty() )
	{
		keyMap_[ '0' ] = KeyCode::KEY_0, keyMap_[ '1' ] = KeyCode::KEY_1, keyMap_[ '2' ] = KeyCode::KEY_2,
		keyMap_[ '3' ] = KeyCode::KEY_3, keyMap_[ '4' ] = KeyCode::KEY_4, keyMap_[ '5' ] = KeyCode::KEY_5,
		keyMap_[ '6' ] = KeyCode::KEY_6, keyMap_[ '7' ] = KeyCode::KEY_7, keyMap_[ '8' ] = KeyCode::KEY_8,
		keyMap_[ '9' ] = KeyCode::KEY_9;

		keyMap_[ 'A' ] = KeyCode::KEY_A, keyMap_[ 'B' ] = KeyCode::KEY_B, keyMap_[ 'C' ] = KeyCode::KEY_C,
		keyMap_[ 'D' ] = KeyCode::KEY_D, keyMap_[ 'E' ] = KeyCode::KEY_E, keyMap_[ 'F' ] = KeyCode::KEY_F,
		keyMap_[ 'G' ] = KeyCode::KEY_G, keyMap_[ 'H' ] = KeyCode::KEY_H, keyMap_[ 'I' ] = KeyCode::KEY_I,
		keyMap_[ 'J' ] = KeyCode::KEY_J, keyMap_[ 'K' ] = KeyCode::KEY_K, keyMap_[ 'L' ] = KeyCode::KEY_L,
		keyMap_[ 'M' ] = KeyCode::KEY_M, keyMap_[ 'N' ] = KeyCode::KEY_N, keyMap_[ 'O' ] = KeyCode::KEY_O,
		keyMap_[ 'P' ] = KeyCode::KEY_P, keyMap_[ 'Q' ] = KeyCode::KEY_Q, keyMap_[ 'R' ] = KeyCode::KEY_R,
		keyMap_[ 'S' ] = KeyCode::KEY_S, keyMap_[ 'T' ] = KeyCode::KEY_T, keyMap_[ 'U' ] = KeyCode::KEY_U,
		keyMap_[ 'V' ] = KeyCode::KEY_V, keyMap_[ 'W' ] = KeyCode::KEY_W, keyMap_[ 'X' ] = KeyCode::KEY_X,
		keyMap_[ 'Y' ] = KeyCode::KEY_Y, keyMap_[ 'Z' ] = KeyCode::KEY_Z;

		keyMap_[ ',' ] = KeyCode::KEY_COMMA, keyMap_[ '.' ] = KeyCode::KEY_PERIOD, keyMap_[ '/' ] = KeyCode::KEY_SLASH,
		keyMap_[ ';' ] = KeyCode::KEY_SEMICOLON, keyMap_[ '\'' ] = KeyCode::KEY_APOSTROPHE, keyMap_[ '[' ] = KeyCode::KEY_LBRACKET,
		keyMap_[ ']' ] = KeyCode::KEY_RBRACKET, keyMap_[ '`' ] = KeyCode::KEY_GRAVE, keyMap_[ '-' ] = KeyCode::KEY_MINUS,
		keyMap_[ '=' ] = KeyCode::KEY_EQUALS, keyMap_[ '\\' ] = KeyCode::KEY_BACKSLASH;

		keyMap_[ ' ' ] = KeyCode::KEY_SPACE;

		nameMap_[ "LSHIFT" ] = KeyCode::KEY_LSHIFT, nameMap_[ "RSHIFT" ] = KeyCode::KEY_RSHIFT;
		nameMap_[ "LCTRL" ] = KeyCode::KEY_LCONTROL, nameMap_[ "RCTRL" ] = KeyCode::KEY_RCONTROL;
		nameMap_[ "LALT" ] = KeyCode::KEY_LALT, nameMap_[ "RALT" ] = KeyCode::KEY_RALT;
		nameMap_[ "LWIN" ] = KeyCode::KEY_LWIN, nameMap_[ "RWIN" ] = KeyCode::KEY_RWIN;
		nameMap_[ "MENU" ] = KeyCode::KEY_APPS;

		nameMap_[ "CAPSLOCK" ] = KeyCode::KEY_CAPSLOCK;
		nameMap_[ "SCROLLLOCK" ] = KeyCode::KEY_SCROLL;
		nameMap_[ "NUMLOCK" ] = KeyCode::KEY_NUMLOCK;

		nameMap_[ "NUM0" ] = KeyCode::KEY_NUMPAD0, nameMap_[ "NUM1" ] = KeyCode::KEY_NUMPAD1,
		nameMap_[ "NUM2" ] = KeyCode::KEY_NUMPAD2, nameMap_[ "NUM3" ] = KeyCode::KEY_NUMPAD3,
		nameMap_[ "NUM4" ] = KeyCode::KEY_NUMPAD4, nameMap_[ "NUM5" ] = KeyCode::KEY_NUMPAD5,
		nameMap_[ "NUM6" ] = KeyCode::KEY_NUMPAD6, nameMap_[ "NUM7" ] = KeyCode::KEY_NUMPAD7,
		nameMap_[ "NUM8" ] = KeyCode::KEY_NUMPAD8, nameMap_[ "NUM9" ] = KeyCode::KEY_NUMPAD9;

		nameMap_[ "NUMMINUS" ] = KeyCode::KEY_NUMPADMINUS, nameMap_[ "NUMPERIOD" ] = KeyCode::KEY_NUMPADPERIOD,
		nameMap_[ "NUMADD" ] = KeyCode::KEY_ADD, nameMap_[ "NUMSTAR" ] = KeyCode::KEY_NUMPADSTAR,
		nameMap_[ "NUMENTER" ] = KeyCode::KEY_NUMPADENTER, nameMap_[ "NUMSLASH" ] = KeyCode::KEY_NUMPADSLASH,
		nameMap_[ "NUMRETURN" ] = KeyCode::KEY_NUMPADENTER;

		nameMap_[ "RETURN" ] = KeyCode::KEY_RETURN, nameMap_[ "ENTER" ] = KeyCode::KEY_RETURN,
		nameMap_[ "TAB" ] = KeyCode::KEY_TAB, nameMap_[ "ESCAPE" ] = KeyCode::KEY_ESCAPE;

		nameMap_[ "F1" ] = KeyCode::KEY_F1, nameMap_[ "F2" ] = KeyCode::KEY_F2,
		nameMap_[ "F3" ] = KeyCode::KEY_F3, nameMap_[ "F4" ] = KeyCode::KEY_F4,
		nameMap_[ "F5" ] = KeyCode::KEY_F5, nameMap_[ "F6" ] = KeyCode::KEY_F6,
		nameMap_[ "F7" ] = KeyCode::KEY_F7, nameMap_[ "F8" ] = KeyCode::KEY_F8,
		nameMap_[ "F9" ] = KeyCode::KEY_F9, nameMap_[ "F10" ] = KeyCode::KEY_F10,
		nameMap_[ "F11" ] = KeyCode::KEY_F11, nameMap_[ "F12" ] = KeyCode::KEY_F12;

		nameMap_[ "UP" ] = KeyCode::KEY_UPARROW, nameMap_[ "DOWN" ] = KeyCode::KEY_DOWNARROW,
		nameMap_[ "LEFT" ] = KeyCode::KEY_LEFTARROW, nameMap_[ "RIGHT" ] = KeyCode::KEY_RIGHTARROW;

		nameMap_[ "INSERT" ] = KeyCode::KEY_INSERT, nameMap_[ "HOME" ] = KeyCode::KEY_HOME,
		nameMap_[ "PAGEUP" ] = KeyCode::KEY_PGUP, nameMap_[ "PAGEDOWN" ] = KeyCode::KEY_PGDN;
		nameMap_[ "DELETE" ] = KeyCode::KEY_DELETE, nameMap_[ "END" ] = KeyCode::KEY_END,
		nameMap_[ "BACKSPACE" ] = KeyCode::KEY_BACKSPACE;
	}
}

bool Win32InputDevice::isKeyDown( const BW::string& key )
{
	BW_GUARD;

	bool result = false;
	BW::string shortcut = key;
	shortcut.erase( std::remove( shortcut.begin(), shortcut.end(), ' ' ),
		shortcut.end() );

	if (shortcut.length() > 0)
		_strupr( &shortcut[0] );// not safe, but we already handled zero lenght string 

	/***********************************************************************
	recognised keys:
	SHIFT, LSHIFT, RSHIFT, CTRL, LCTRL, RCTRL, ALT, LALT, RALT,
	WIN, LWIN, RWIN, MENU,
	CAPSLOCK, SCROLLLOCK, NUMLOCK,
	NUM0, NUM1, ..., NUM9, NUMMINUS, NUMPERIOD, NUMADD, NUMSTAR, NUMENTER, NUMSLASH,
	RETURN, ENTER, TAB, ESCAPE,
	F1, F2, F3, F4, ..., F12,
	UP, DOWN, LEFT, RIGHT,
	INSERT, HOME, PAGEUP, PAGEDOWN, DELETE, END, BACKSPACE
	***********************************************************************/
	BW::set<BW::string> modifiers;
	BW::string keyname;
	while( shortcut.find( '+' ) != shortcut.npos )
	{
		modifiers.insert( shortcut.substr( 0, shortcut.find( '+' ) ) );
		shortcut = shortcut.substr( shortcut.find( '+' ) + 1 );
	}
	keyname = shortcut;
	if( !keyname.empty() )
	{
		bool keydown = down( keyname );
		if( !keydown && keyname.size() == 1 && down( keyname[0] ) )
			keydown = true;

		if( keydown )
		{
			result = true;
			for( BW::set<BW::string>::iterator iter = modifiers.begin();
				iter != modifiers.end(); ++iter )
			{
				if( !down( *iter ) )
					return false;
			}
		}
	}
	return result;
}

static HHOOK s_hook = NULL;
static bool s_hookEntered = false;

static LRESULT CALLBACK GUIKeyboadProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	// Prevent re-entry from key events while we are processing
	if (s_hookEntered)
	{
		return CallNextHookEx( s_hook, nCode, wParam, lParam );
	}
	s_hookEntered = true;

	bool inputWasProcessed = false;

	// NOTE: no input will be processed if the main app window is disabled,
	// i.e. when a modal popup window is running. This prevents keystrokes
	// such as Ctrl+Z from working when a modal window is on top.
	if (nCode == HC_ACTION && ( lParam & 0xc0000000 ) == 0)
	{
		const size_t MAX_CLASS_NAME = 200;
		static wchar_t className[ MAX_CLASS_NAME ];
		GetClassName( GetFocus(), className, MAX_CLASS_NAME );
		if (wcsncmp( className, L"Edit", 4 ) != 0 &&// Don't process for edit box
			AfxGetMainWnd()->IsWindowEnabled())
		{
			GUI::Win32InputDevice inputDevice( char( lParam / 65536 % 256 ));
			inputWasProcessed = GUI::Manager::instance().processInput( &inputDevice );
		}
	}

	// HACK discard input messages that arrived before/during processing
	// Try to prevent keys in InputDevices sticking
	if (inputWasProcessed)
	{
		// Throw away keyboard input that has occurred
		MSG msg;
		while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
			if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)
			{
				continue;
			}
			if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
			{
				continue;
			}
			if (msg.message == WM_COMMAND)
			{
				continue;
			}
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		// Reset input state
		// Send messages to force up the keys already pressed
		// (do before clearing key states)
		InputDevices::instance().forceKeysUp();

		// Clear key states and event queue
		InputDevices::instance().resetKeyStates();
		InputDevices::instance().clearEventQueue();

		// Get if the shortcut key is down and add it to the raw key down map
		Vector2 tmp;
		bool extendedBit = lParam & (1<<24) ? true : false;
		KeyCode::Key key =
			InputDevices::instance().getKeyCodeDown( wParam, extendedBit );

		// Force up ctrl (may or may not have been pressed)
		InputDevices::instance().forceKeyUp( KeyCode::KEY_LCONTROL, 0, tmp );

		// Force up shortcut key that was pressed
		InputDevices::instance().forceKeyUp( key, 0, tmp );

		// Ignore the next key down from the shortcut key
		// as it is the same key press that this hook is processing
		InputDevices::instance().ignoreNextKeyDown( key );
	}

	// Prevent re-entry
	s_hookEntered = false;

	return CallNextHookEx( s_hook, nCode, wParam, lParam );
}

void Win32InputDevice::install()
{
	BW_GUARD;

	s_hook = SetWindowsHookEx( WH_KEYBOARD, GUIKeyboadProc, NULL, GetCurrentThreadId() );
}

void Win32InputDevice::fini()
{
	BW_GUARD;

	if (s_hook != NULL)
	{
		UnhookWindowsHookEx( s_hook );
		s_hook = NULL;
	}
}

END_GUI_NAMESPACE
BW_END_NAMESPACE

