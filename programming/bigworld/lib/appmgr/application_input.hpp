#ifndef APPLICATIONINPUT_HPP
#define APPLICATIONINPUT_HPP

#include <iostream>

#include "input/input.hpp"

BW_BEGIN_NAMESPACE

/**
 * This class handles system-wide input events,
 * namely handling ALT-ENTER for switching windowing modes
 * and SHIFT-F4 to close the application.
 */
class ApplicationInput
{
public:
	ApplicationInput();
	~ApplicationInput();

	static bool handleKeyEvent( const KeyEvent & event );
	static bool handleMouseEvent( const MouseEvent & /*event*/ );
	static bool handleKeyDown( const KeyEvent & event );
	static void disableModeSwitch();
private:
	static bool disableModeSwitch_;
	ApplicationInput(const ApplicationInput&);
	ApplicationInput& operator=(const ApplicationInput&);

	friend std::ostream& operator<<(std::ostream&, const ApplicationInput&);
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "application_input.ipp"
#endif

#endif
/*applicationinput.hpp*/
