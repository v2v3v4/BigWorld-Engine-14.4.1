#include "pch.hpp"
#include "input_manager.hpp"

#include "application_input.hpp"
#include "module.hpp"
#include "module_manager.hpp"

#include "romp/console_manager.hpp"
#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_input_handler.hpp"

#ifndef CODE_INLINE
#include "input_manager.ipp"
#endif

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
InputManager::InputManager()
{
}


/**
 *	Destructor.
 */
InputManager::~InputManager()
{
}


/**
 * This method responds to key events by sending them
 * to the currently active module.  If the module did
 * not handle the event, then it is passed on to the
 * standard application input task.
 *
 * @param event the incoming key event
 *
 * @return true if the event was handled by any module
 */
bool InputManager::handleKeyEvent( const KeyEvent & event )
{
	bool handled = ConsoleManager::instance().handleKeyEvent( event );

	ModulePtr module = ModuleManager::instance().currentModule();

	if ( module && !handled )
	{
		handled = module->handleKeyEvent( event );
	}

	if ( !handled )
	{
		handled = ApplicationInput::handleKeyEvent( event );
	}

	return handled;
}


/**
 * This method handles incoming mouse events.  Like the keyboard
 * events, mouse events are routed first through the current module,
 * and then through the standard application input task ( but only
 * if the current module did not handle the mouse event itself )
 *
 * @param event the incoming mouse event
 *
 * @return true if the event was handled by any module.
 */
bool InputManager::handleMouseEvent( const MouseEvent & event )
{
	ModulePtr module = ModuleManager::instance().currentModule();

	bool handled = false;

	if ( module )
	{
		handled = module->handleMouseEvent( event );
	}

	if ( !handled )
	{
		handled = ApplicationInput::handleMouseEvent( event );
	}

	return handled;
}


/**
 *	Output streaming operator for InputManager.
 */
std::ostream& operator<<(std::ostream& o, const InputManager& t)
{
	o << "InputManager\n";
	return o;
}

BW_END_NAMESPACE

/*input_manager.cpp*/
