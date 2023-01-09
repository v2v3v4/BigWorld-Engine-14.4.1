#include "pch.hpp"
#include "key_code.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: KeyMap
// -----------------------------------------------------------------------------
class KeyMap
{
public:
	KeyMap();

	KeyCode::Key stringToKey( const BW::string & str ) const;
	const char * keyToString( const KeyCode::Key & key ) const;

	static KeyMap& instance() { static KeyMap inst; return inst; }

private:
	StringHashMap<KeyCode::Key> map_;
};

#define KEY_ADD( key )	map_[ #key ] = KeyCode::KEY_##key;

/**
 *	Constructor
 */
KeyMap::KeyMap()
{
	map_[ "ESCAPE" ] = KeyCode::KEY_ESCAPE;

		KEY_ADD(ESCAPE          )
		KEY_ADD(1               )
		KEY_ADD(2               )
		KEY_ADD(3               )
		KEY_ADD(4               )
		KEY_ADD(5               )
		KEY_ADD(6               )
		KEY_ADD(7               )
		KEY_ADD(8               )
		KEY_ADD(9               )
		KEY_ADD(0               )
		KEY_ADD(MINUS           )
		KEY_ADD(EQUALS          )
		KEY_ADD(BACKSPACE       )
		KEY_ADD(TAB             )
		KEY_ADD(Q               )
		KEY_ADD(W               )
		KEY_ADD(E               )
		KEY_ADD(R               )
		KEY_ADD(T               )
		KEY_ADD(Y               )
		KEY_ADD(U               )
		KEY_ADD(I               )
		KEY_ADD(O               )
		KEY_ADD(P               )
		KEY_ADD(LBRACKET        )
		KEY_ADD(RBRACKET        )
		KEY_ADD(RETURN          )
		KEY_ADD(LCONTROL        )
		KEY_ADD(A               )
		KEY_ADD(S               )
		KEY_ADD(D               )
		KEY_ADD(F               )
		KEY_ADD(G               )
		KEY_ADD(H               )
		KEY_ADD(J               )
		KEY_ADD(K               )
		KEY_ADD(L               )
		KEY_ADD(SEMICOLON       )
		KEY_ADD(APOSTROPHE      )
		KEY_ADD(GRAVE           )
		KEY_ADD(LSHIFT          )
		KEY_ADD(BACKSLASH       )
		KEY_ADD(Z               )
		KEY_ADD(X               )
		KEY_ADD(C               )
		KEY_ADD(V               )
		KEY_ADD(B               )
		KEY_ADD(N               )
		KEY_ADD(M               )
		KEY_ADD(COMMA           )
		KEY_ADD(PERIOD          )
		KEY_ADD(SLASH           )
		KEY_ADD(RSHIFT          )
		KEY_ADD(NUMPADSTAR      )
		KEY_ADD(LALT            )
		KEY_ADD(SPACE           )
		KEY_ADD(CAPSLOCK        )
		KEY_ADD(F1              )
		KEY_ADD(F2              )
		KEY_ADD(F3              )
		KEY_ADD(F4              )
		KEY_ADD(F5              )
		KEY_ADD(F6              )
		KEY_ADD(F7              )
		KEY_ADD(F8              )
		KEY_ADD(F9              )
		KEY_ADD(F10             )
		KEY_ADD(NUMLOCK         )
		KEY_ADD(SCROLL          )
		KEY_ADD(NUMPAD7         )
		KEY_ADD(NUMPAD8         )
		KEY_ADD(NUMPAD9         )
		KEY_ADD(NUMPADMINUS     )
		KEY_ADD(NUMPAD4         )
		KEY_ADD(NUMPAD5         )
		KEY_ADD(NUMPAD6         )
		KEY_ADD(ADD             )
		KEY_ADD(NUMPAD1         )
		KEY_ADD(NUMPAD2         )
		KEY_ADD(NUMPAD3         )
		KEY_ADD(NUMPAD0         )
		KEY_ADD(NUMPADPERIOD    )
		KEY_ADD(OEM_102         )
		KEY_ADD(F11             )
		KEY_ADD(F12             )

		KEY_ADD(F13             )
		KEY_ADD(F14             )
		KEY_ADD(F15             )

		KEY_ADD(KANA            )
		KEY_ADD(ABNT_C1         )
		KEY_ADD(CONVERT         )
		KEY_ADD(NOCONVERT       )
		KEY_ADD(YEN             )
		KEY_ADD(ABNT_C2         )
		KEY_ADD(NUMPADEQUALS    )
		KEY_ADD(PREVTRACK   )
		KEY_ADD(AT              )
		KEY_ADD(COLON           )
		KEY_ADD(UNDERLINE       )
		KEY_ADD(KANJI           )
		KEY_ADD(STOP            )
		KEY_ADD(AX              )
		KEY_ADD(UNLABELED       )
		KEY_ADD(NEXTTRACK       )
		KEY_ADD(NUMPADENTER     )
		KEY_ADD(RCONTROL        )
		KEY_ADD(MUTE            )
		KEY_ADD(CALCULATOR      )
		KEY_ADD(PLAYPAUSE       )
		KEY_ADD(MEDIASTOP       )
		KEY_ADD(VOLUMEDOWN      )
		KEY_ADD(VOLUMEUP        )
		KEY_ADD(WEBHOME         )
		KEY_ADD(NUMPADCOMMA     )
		KEY_ADD(NUMPADSLASH     )
		KEY_ADD(SYSRQ           )
		KEY_ADD(RALT            )
		KEY_ADD(PAUSE           )
		KEY_ADD(HOME            )
		KEY_ADD(UPARROW         )
		KEY_ADD(PGUP            )
		KEY_ADD(LEFTARROW       )
		KEY_ADD(RIGHTARROW      )
		KEY_ADD(END             )
		KEY_ADD(DOWNARROW       )
		KEY_ADD(PGDN            )
		KEY_ADD(INSERT          )
		KEY_ADD(DELETE          )
		KEY_ADD(LWIN            )
		KEY_ADD(RWIN            )
		KEY_ADD(APPS            )
		KEY_ADD(POWER           )
		KEY_ADD(SLEEP           )
		KEY_ADD(WAKE            )
		KEY_ADD(WEBSEARCH       )
		KEY_ADD(WEBFAVORITES    )
		KEY_ADD(WEBREFRESH      )
		KEY_ADD(WEBSTOP         )
		KEY_ADD(WEBFORWARD      )
		KEY_ADD(WEBBACK         )
		KEY_ADD(MYCOMPUTER      )
		KEY_ADD(MAIL            )
		KEY_ADD(MEDIASELECT     )

		KEY_ADD(MOUSE0          )
		KEY_ADD(LEFTMOUSE       )
		KEY_ADD(MOUSE1          )
		KEY_ADD(RIGHTMOUSE      )
		KEY_ADD(MOUSE2          )
		KEY_ADD(MIDDLEMOUSE     )
		KEY_ADD(MOUSE3          )
		KEY_ADD(MOUSE4          )
		KEY_ADD(MOUSE5          )
		KEY_ADD(MOUSE6          )
		KEY_ADD(MOUSE7          )

		KEY_ADD(JOYDUP			)
		KEY_ADD(JOYDDOWN		)
		KEY_ADD(JOYDLEFT		)
		KEY_ADD(JOYDRIGHT		)
		KEY_ADD(JOYSTART		)
		KEY_ADD(JOYBACK			)
		KEY_ADD(JOYALPUSH		)
		KEY_ADD(JOYARPUSH		)

		KEY_ADD(JOYA			)
		KEY_ADD(JOYB			)
		KEY_ADD(JOYX			)
		KEY_ADD(JOYY			)

		KEY_ADD(JOYBLACK		)
		KEY_ADD(JOYWHITE		)

		KEY_ADD(JOYLTRIGGER		)
		KEY_ADD(JOYRTRIGGER		)

		KEY_ADD(JOYALUP			)
		KEY_ADD(JOYALDOWN		)
		KEY_ADD(JOYALLEFT		)
		KEY_ADD(JOYALRIGHT		)
		KEY_ADD(JOYALUP			)
		KEY_ADD(JOYALDOWN		)
		KEY_ADD(JOYALLEFT		)
		KEY_ADD(JOYALRIGHT		)
		KEY_ADD(DEBUG			)

		KEY_ADD(IME_CHAR		)
}

const char * KeyMap::keyToString( const KeyCode::Key & key ) const
{
	StringHashMap<KeyCode::Key>::const_iterator iter = map_.begin();
	while (iter != map_.end())
	{
		if (iter->second == key) return iter->first.c_str();
		iter++;
	}

	static const char * nullString = "";
	return nullString;
}

KeyCode::Key KeyMap::stringToKey( const BW::string & str ) const
{
	StringHashMap<KeyCode::Key>::const_iterator foundIter = map_.find( str );

	if (foundIter != map_.end())
	{
		return foundIter->second;
	}
	else
	{
		// We do not have the string in the map.
		return KeyCode::KEY_NOT_FOUND;
	}
}


// -----------------------------------------------------------------------------
// Section: KeyCode
// -----------------------------------------------------------------------------

namespace KeyCode {

	/**
	 *	Returns a key associated with the input string.
	 *
	 *	@param str	The string to search for.
	 *
	 *	@return	The associated Key identifier.
	 */
	Key stringToKey( const BW::string & str )
	{
		BW_GUARD;
		return KeyMap::instance().stringToKey( str );
	}

	/**
	 *	Returns the name associated with the input key
	 *
	 *	@param str	The string to search for.
	 *
	 *	@return	The associated Key identifier.
	 */
	const char * keyToString( const KeyCode::Key key )
	{ 
		BW_GUARD;
		return KeyMap::instance().keyToString( key ); 
	}
}

BW_END_NAMESPACE

// key_code.cpp
