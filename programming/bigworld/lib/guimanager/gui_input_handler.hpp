#ifndef GUI_INPUT_HANDLER_HPP__
#define GUI_INPUT_HANDLER_HPP__

#include "gui_item.hpp"
#include "input/input.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

class BigworldInputDevice : public GUI::InputDevice
{
	const bool* keyDownTable_;
	static BW::map<char, int> keyMap_;
	static BW::map<BW::string, int> nameMap_;
	static bool lastKeyDown[ KeyCode::NUM_KEYS ];
	static bool down( const bool* keyDownTable, char ch );
	static bool down( const bool* keyDownTable, const BW::string& key );
public:
	BigworldInputDevice( const bool* keyDownTable );
	virtual bool isKeyDown( const BW::string& key );
	static void refreshKeyDownState( const bool* keyDown );
};

class Win32InputDevice : public GUI::InputDevice
{
	static BW::map<char, int> keyMap_;
	static BW::map<BW::string, int> nameMap_;
	static bool down( char ch );
	static bool down( const BW::string& key );
	static char ch_;
public:
	Win32InputDevice( char ch );
	virtual bool isKeyDown( const BW::string& key );

	static void install();

	static void fini();
};

END_GUI_NAMESPACE
BW_END_NAMESPACE

#endif//GUI_INPUT_HANDLER_HPP__
