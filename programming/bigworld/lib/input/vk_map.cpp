#include "pch.hpp"
#include "vk_map.hpp"


BW_BEGIN_NAMESPACE

namespace VKMap 
{
	const KeyCode::Key VKMap[] = {
		KeyCode::KEY_NONE,			// unassigned		0x00
		KeyCode::KEY_LEFTMOUSE,		// VK_LBUTTON       0x01
		KeyCode::KEY_RIGHTMOUSE,	// VK_RBUTTON       0x02
		KeyCode::KEY_NONE,			// VK_CANCEL        0x03
		KeyCode::KEY_MIDDLEMOUSE,	// VK_MBUTTON       0x04
		KeyCode::KEY_NONE,			// VK_XBUTTON1      0x05
		KeyCode::KEY_NONE,			// VK_XBUTTON2      0x06
		KeyCode::KEY_NONE,			// unassigned       0x07
		KeyCode::KEY_BACKSPACE,		// VK_BACK          0x08
		KeyCode::KEY_TAB,			// VK_TAB           0x09
		KeyCode::KEY_NONE,			// reserved         0x0A
		KeyCode::KEY_NONE,			// reserved         0x0B
		KeyCode::KEY_NONE,			// VK_CLEAR         0x0C
		KeyCode::KEY_RETURN,		// VK_RETURN        0x0D
		KeyCode::KEY_NONE,			// unassigned       0x0E
		KeyCode::KEY_NONE,			// unassigned       0x0F		
		KeyCode::KEY_NONE,			// VK_SHIFT         0x10  Shift/Control is mapped by left/right specific keys
		KeyCode::KEY_NONE,			// VK_CONTROL       0x11  Ditto for control and menu
		KeyCode::KEY_NONE,			// VK_MENU          0x12
		KeyCode::KEY_PAUSE,			// VK_PAUSE         0x13
		KeyCode::KEY_CAPSLOCK,		// VK_CAPITAL       0x14
		KeyCode::KEY_KANA,			// VK_KANA          0x15
		KeyCode::KEY_NONE,			// unassigned       0x16
		KeyCode::KEY_NONE,			// VK_JUNJA         0x17
		KeyCode::KEY_NONE,			// VK_FINAL         0x18
		KeyCode::KEY_KANJI,			// VK_KANJI         0x19
		KeyCode::KEY_NONE,			// unassigned       0x1A
		KeyCode::KEY_ESCAPE,		// VK_ESCAPE        0x1B
		KeyCode::KEY_CONVERT,		// VK_CONVERT       0x1C
		KeyCode::KEY_NOCONVERT,		// VK_NONCONVERT    0x1D
		KeyCode::KEY_NONE,			// VK_ACCEPT        0x1E
		KeyCode::KEY_NONE,			// VK_MODECHANGE    0x1F
		KeyCode::KEY_SPACE,			// VK_SPACE         0x20
		KeyCode::KEY_PGUP,			// VK_PRIOR         0x21
		KeyCode::KEY_PGDN,			// VK_NEXT          0x22
		KeyCode::KEY_END,			// VK_END           0x23
		KeyCode::KEY_HOME,			// VK_HOME          0x24
		KeyCode::KEY_LEFTARROW,		// VK_LEFT          0x25
		KeyCode::KEY_UPARROW,		// VK_UP            0x26
		KeyCode::KEY_RIGHTARROW,	// VK_RIGHT         0x27
		KeyCode::KEY_DOWNARROW,		// VK_DOWN          0x28
		KeyCode::KEY_NONE,			// VK_SELECT        0x29
		KeyCode::KEY_NONE,			// VK_PRINT         0x2A
		KeyCode::KEY_NONE,			// VK_EXECUTE       0x2B
		KeyCode::KEY_SYSRQ,			// VK_SNAPSHOT      0x2C
		KeyCode::KEY_INSERT,		// VK_INSERT        0x2D
		KeyCode::KEY_DELETE,		// VK_DELETE        0x2E
		KeyCode::KEY_NONE,			// VK_HELP          0x2F
		KeyCode::KEY_0,				// '0' to '9'       0x30
		KeyCode::KEY_1,
		KeyCode::KEY_2,
		KeyCode::KEY_3,
		KeyCode::KEY_4,
		KeyCode::KEY_5,
		KeyCode::KEY_6,
		KeyCode::KEY_7,
		KeyCode::KEY_8,
		KeyCode::KEY_9,				//                  0x39
		KeyCode::KEY_NONE,			//  unassigned      0x3A
		KeyCode::KEY_NONE,			//  unassigned      0x3B
		KeyCode::KEY_NONE,			//  unassigned      0x3C
		KeyCode::KEY_NONE,			//  unassigned      0x3D
		KeyCode::KEY_NONE,			//  unassigned      0x3E
		KeyCode::KEY_NONE,			//  unassigned      0x3F
		KeyCode::KEY_NONE,			//  unassigned      0x40
		KeyCode::KEY_A,				// 'A' to 'Z'       0x41
		KeyCode::KEY_B,
		KeyCode::KEY_C,
		KeyCode::KEY_D,
		KeyCode::KEY_E,
		KeyCode::KEY_F,
		KeyCode::KEY_G,
		KeyCode::KEY_H,
		KeyCode::KEY_I,
		KeyCode::KEY_J,
		KeyCode::KEY_K,
		KeyCode::KEY_L,
		KeyCode::KEY_M,
		KeyCode::KEY_N,
		KeyCode::KEY_O,
		KeyCode::KEY_P,
		KeyCode::KEY_Q,
		KeyCode::KEY_R,
		KeyCode::KEY_S,
		KeyCode::KEY_T,
		KeyCode::KEY_U,
		KeyCode::KEY_V,
		KeyCode::KEY_W,
		KeyCode::KEY_X,
		KeyCode::KEY_Y,
		KeyCode::KEY_Z,				//                  0x5A
		KeyCode::KEY_LWIN,			// VK_LWIN          0x5B
		KeyCode::KEY_RWIN,			// VK_RWIN          0x5C
		KeyCode::KEY_APPS,			// VK_APPS          0x5D
		KeyCode::KEY_NONE,			// reserved         0x5E	    
		KeyCode::KEY_SLEEP,			// VK_SLEEP         0x5F
		KeyCode::KEY_NUMPAD0,		// VK_Numpad0       0x60
		KeyCode::KEY_NUMPAD1,		// VK_Numpad1       0x61
		KeyCode::KEY_NUMPAD2,		// VK_Numpad2       0x62
		KeyCode::KEY_NUMPAD3,		// VK_Numpad3       0x63
		KeyCode::KEY_NUMPAD4,		// VK_Numpad4       0x64
		KeyCode::KEY_NUMPAD5,		// VK_Numpad5       0x65
		KeyCode::KEY_NUMPAD6,		// VK_Numpad6       0x66
		KeyCode::KEY_NUMPAD7,		// VK_Numpad7       0x67
		KeyCode::KEY_NUMPAD8,		// VK_Numpad8       0x68
		KeyCode::KEY_NUMPAD9,		// VK_Numpad9       0x69
		KeyCode::KEY_NUMPADSTAR,	// VK_MULTIPLY      0x6A
		KeyCode::KEY_ADD,			// VK_ADD           0x6B
		KeyCode::KEY_NONE,			// VK_SEPARATOR     0x6C
		KeyCode::KEY_NUMPADMINUS,	// VK_Subtract      0x6D
		KeyCode::KEY_NUMPADPERIOD,	// VK_DECIMAL       0x6E
		KeyCode::KEY_NUMPADSLASH,	// VK_DIVIDE        0x6F
		KeyCode::KEY_F1,			// VK_F1            0x70
		KeyCode::KEY_F2,			// VK_F2            0x71
		KeyCode::KEY_F3,			// VK_F3            0x72
		KeyCode::KEY_F4,			// VK_F4            0x73
		KeyCode::KEY_F5,			// VK_F5            0x74
		KeyCode::KEY_F6,			// VK_F6            0x75
		KeyCode::KEY_F7,			// VK_F7            0x76
		KeyCode::KEY_F8,			// VK_F8            0x77
		KeyCode::KEY_F9,			// VK_F9            0x78
		KeyCode::KEY_F10,			// VK_F10           0x79
		KeyCode::KEY_F11,			// VK_F11           0x7A
		KeyCode::KEY_F12,			// VK_F12           0x7B
		KeyCode::KEY_F13,			// VK_F13           0x7C
		KeyCode::KEY_F14,			// VK_F14           0x7D
		KeyCode::KEY_F15,			// VK_F15           0x7E
		KeyCode::KEY_NONE,			// VK_F16           0x7F
		KeyCode::KEY_NONE,			// VK_F17           0x80
		KeyCode::KEY_NONE,			// VK_F18           0x81
		KeyCode::KEY_NONE,			// VK_F19           0x82
		KeyCode::KEY_NONE,			// VK_F20           0x83
		KeyCode::KEY_NONE,			// VK_F21           0x84
		KeyCode::KEY_NONE,			// VK_F22           0x85
		KeyCode::KEY_NONE,			// VK_F23           0x86
		KeyCode::KEY_NONE,			// VK_F24           0x87
		KeyCode::KEY_NONE,			// unassigned       0x88
		KeyCode::KEY_NONE,			// unassigned       0x89
		KeyCode::KEY_NONE,			// unassigned       0x8A
		KeyCode::KEY_NONE,			// unassigned       0x8B
		KeyCode::KEY_NONE,			// unassigned       0x8C
		KeyCode::KEY_NONE,			// unassigned       0x8D
		KeyCode::KEY_NONE,			// unassigned       0x8E
		KeyCode::KEY_NONE,			// unassigned       0x8F
		KeyCode::KEY_NUMLOCK,		// VK_NUMLOCK       0x90
		KeyCode::KEY_SCROLL,		// VK_SCROLL        0x91
		KeyCode::KEY_NONE,			// VK_OEM_NEC_EQUAL		0x92
		KeyCode::KEY_NONE,			// VK_OEM_FJ_MASSHOU	0x93
		KeyCode::KEY_NONE,			// VK_OEM_FJ_TOUROKU	0x94
		KeyCode::KEY_NONE,			// VK_OEM_FJ_LOYA		0x95
		KeyCode::KEY_NONE,			// VK_OEM_FJ_ROYA		0x96
		KeyCode::KEY_NONE,			// unassigned			0x97
		KeyCode::KEY_NONE,			// unassigned			0x98
		KeyCode::KEY_NONE,			// unassigned			0x99
		KeyCode::KEY_NONE,			// unassigned			0x9A
		KeyCode::KEY_NONE,			// unassigned			0x9B
		KeyCode::KEY_NONE,			// unassigned			0x9C
		KeyCode::KEY_NONE,			// unassigned			0x9D
		KeyCode::KEY_NONE,			// unassigned			0x9E
		KeyCode::KEY_NONE,			// unassigned			0x9F
		KeyCode::KEY_LSHIFT,		// VK_LSHIFT			0xA0
		KeyCode::KEY_RSHIFT,		// VK_RSHIFT			0xA1
		KeyCode::KEY_LCONTROL,		// VK_LCONTROL			0xA2
		KeyCode::KEY_RCONTROL,		// VK_RCONTROL			0xA3
		KeyCode::KEY_LALT,			// VK_LMENU				0xA4
		KeyCode::KEY_RALT,			// VK_RMENU				0xA5
		KeyCode::KEY_WEBFORWARD,	// VK_BROWSER_BACK      0xA6
		KeyCode::KEY_WEBBACK,		// VK_BROWSER_FORWARD   0xA7
		KeyCode::KEY_WEBREFRESH,	// VK_BROWSER_REFRESH   0xA8
		KeyCode::KEY_WEBSTOP,		// VK_BROWSER_STOP      0xA9
		KeyCode::KEY_WEBSEARCH,		// VK_BROWSER_SEARCH    0xAA
		KeyCode::KEY_WEBFAVORITES,	// VK_BROWSER_FAVORITES 0xAB
		KeyCode::KEY_WEBHOME,		// VK_BROWSER_HOME        0xAC
		KeyCode::KEY_MUTE,			// VK_Volume_MUTE         0xAD
		KeyCode::KEY_VOLUMEDOWN,	// VK_Volume_DOWN         0xAE
		KeyCode::KEY_VOLUMEUP,		// VK_Volume_UP           0xAF
		KeyCode::KEY_NEXTTRACK,		// VK_MEDIA_NEXT_TRACK    0xB0
		KeyCode::KEY_PREVTRACK,		// VK_MEDIA_PREV_TRACK    0xB1
		KeyCode::KEY_MEDIASTOP,		// VK_MEDIA_STOP          0xB2
		KeyCode::KEY_PLAYPAUSE,		// VK_MEDIA_PLAY_PAUSE    0xB3
		KeyCode::KEY_MAIL,			// VK_LAUNCH_MAIL         0xB4
		KeyCode::KEY_MEDIASELECT,	// VK_LAUNCH_MEDIA_SELECT 0xB5
		KeyCode::KEY_NONE,			// VK_LAUNCH_APP1         0xB6
		KeyCode::KEY_NONE,			// VK_LAUNCH_APP2         0xB7
		KeyCode::KEY_NONE,			// reserved				0xB8
		KeyCode::KEY_NONE,			// reserved			0xB9
		KeyCode::KEY_SEMICOLON,		// VK_OEM_1          0xBA  // ';:' for US
		KeyCode::KEY_EQUALS,		// VK_OEM_PLUS       0xBB  // '+' any country
		KeyCode::KEY_COMMA,			// VK_OEM_Comma      0xBC  // ',' any country
		KeyCode::KEY_MINUS,			// VK_OEM_MINUS      0xBD  // '-' any country
		KeyCode::KEY_PERIOD,		// VK_OEM_Period     0xBE  // '.' any country
		KeyCode::KEY_SLASH,			// VK_OEM_2          0xBF  // '/?' for US// '/?' for US
		KeyCode::KEY_GRAVE,			// VK_OEM_3          0xC0  // '`~' for US

		KeyCode::KEY_NONE,			//  reserved        0xC1
		KeyCode::KEY_NONE,			//  reserved        0xC2
		KeyCode::KEY_NONE,			//  reserved        0xC3
		KeyCode::KEY_NONE,			//  reserved        0xC4
		KeyCode::KEY_NONE,			//  reserved        0xC5
		KeyCode::KEY_NONE,			//  reserved        0xC6
		KeyCode::KEY_NONE,			//  reserved        0xC7
		KeyCode::KEY_NONE,			//  reserved        0xC8
		KeyCode::KEY_NONE,			//  reserved        0xC9
		KeyCode::KEY_NONE,			//  reserved        0xCA
		KeyCode::KEY_NONE,			//  reserved        0xCB
		KeyCode::KEY_NONE,			//  reserved        0xCC
		KeyCode::KEY_NONE,			//  reserved        0xCD
		KeyCode::KEY_NONE,			//  reserved        0xCE
		KeyCode::KEY_NONE,			//  reserved        0xCF
		KeyCode::KEY_NONE,			//  reserved        0xD0
		KeyCode::KEY_NONE,			//  reserved        0xD1
		KeyCode::KEY_NONE,			//  reserved        0xD2
		KeyCode::KEY_NONE,			//  reserved        0xD3
		KeyCode::KEY_NONE,			//  reserved        0xD4
		KeyCode::KEY_NONE,			//  reserved        0xD5
		KeyCode::KEY_NONE,			//  reserved        0xD6
		KeyCode::KEY_NONE,			//  reserved        0xD7
		KeyCode::KEY_NONE,			//  reserved        0xD8
		KeyCode::KEY_NONE,			//  reserved        0xD9
		KeyCode::KEY_NONE,			//  reserved        0xDA

		KeyCode::KEY_LBRACKET,		// VK_OEM_4          0xDB  //  '[{' for US
		KeyCode::KEY_BACKSLASH,		// VK_OEM_5          0xDC  //  '\|' for US
		KeyCode::KEY_RBRACKET,		// VK_OEM_6          0xDD  //  ']}' for US
		KeyCode::KEY_APOSTROPHE,	// VK_OEM_7          0xDE  //  ''"' for US
		KeyCode::KEY_NONE			// VK_OEM_8          0xDF
	};

	const USHORT MAX_VK_CONSTANT = ARRAY_SIZE(VKMap);


	KeyCode::Key fromVKey( USHORT vkey, bool extended )
	{
		if (vkey == VK_EREOF)
		{
			// for some reason VK_APPS gets mapped to this
			return KeyCode::KEY_APPS;
		}

		if (vkey >= MAX_VK_CONSTANT)
		{
			return KeyCode::KEY_NONE;
		}

		// Remap to specific left/right keys (shift is a little more message specific so is handled by the caller).
		if (vkey == VK_CONTROL || vkey == VK_LCONTROL)
		{
			vkey = extended ? VK_RCONTROL : VK_LCONTROL;
		}
		else if (vkey == VK_MENU || vkey == VK_LMENU)
		{
			vkey = extended ? VK_RMENU : VK_LMENU;
		}
		else if (vkey == VK_RETURN)
		{
			// There's no specific VK_ code for numpad enter, so just shortcut it here.
			if (extended)
			{
				return KeyCode::KEY_NUMPADENTER;
			}
		}

		return VKMap[vkey];
	}

	KeyCode::Key fromRawKey( RAWKEYBOARD* rkb )
	{
		// Special case key handling.
		if ( rkb->VKey == VK_SNAPSHOT )
		{
			return KeyCode::KEY_SYSRQ;
		}

		USHORT vkey = rkb->VKey;
		//for shift the rkb doesn't contain the data whether its left or right
		if (vkey == VK_SHIFT) 
		{
			//we couldn't use this for all keys as it doesn't map correctly for 
			//numpad keys when num lock is on.
			vkey = MapVirtualKey( rkb->MakeCode, 0x03 ); // 0x03 == MAPVK_VSC_TO_VK_EX
		}

		bool extended = (rkb->Flags & RI_KEY_E0) ? true : false;

		return fromVKey( vkey, extended );
	}

	USHORT toVKey( KeyCode::Key key )
	{
		// For what we need this for, this is good enough. Would be
		// nicer to have a proper reverse LUT however.
		for( USHORT i = 0; i < MAX_VK_CONSTANT; i++ )
		{
			if (VKMap[i] == key)
			{
				return (USHORT)i;
			}
		}

		return 0;
	}

} // namespace VKMap

BW_END_NAMESPACE

// vk_map.cpp

