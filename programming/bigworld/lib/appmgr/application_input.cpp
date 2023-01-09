#include "pch.hpp"

#include "application_input.hpp"
#include "app.hpp"

#include "moo/render_context.hpp"
#include "romp/console_manager.hpp"

#ifndef CODE_INLINE
#include "application_input.ipp"
#endif

BW_BEGIN_NAMESPACE

bool ApplicationInput::disableModeSwitch_ = false;

/**
 *	Constructor.
 */
ApplicationInput::ApplicationInput()
{
}


/**
 *	Destructor.
 */
ApplicationInput::~ApplicationInput()
{
}


/**
 *	This method handles key events for the global application.
 */
bool ApplicationInput::handleKeyEvent( const KeyEvent & event )
{
	bool handled = ConsoleManager::instance().handleKeyEvent( event );

	if (!handled && event.isKeyDown())
	{
		handled = handleKeyDown( event );
	}

	return handled;
}


/**
 *	This method handles mouse events. Currently, it does nothing.
 *
 *	@param event	Not used.
 *
 *	@return Always returns false.
 */
bool ApplicationInput::handleMouseEvent( const MouseEvent & /*event*/ )
{
	return false;
}


/**
 *	This method handles key and button down events. It is called by
 *	App::handleKeyEvent.
 *
 *	This method gets called by the App, in lieu of any other module handling
 *	these key events.
 *
 *	@see handleKeyEvent
 */
bool ApplicationInput::handleKeyDown( const KeyEvent & event )
{
	bool handled = true;

	switch (event.key())
	{
		case KeyCode::KEY_F4:
		{
			if (event.isAltDown())
			{
				//TEMP! WindowsMain::shutDown();
			}
			else if (event.isShiftDown())
			{
				Moo::rc().screenShot();
			}
			break;
		}

		case KeyCode::KEY_RETURN:
			if ( !disableModeSwitch_ && InputDevices::isAltDown( ))
			{
				if (Moo::rc().device())
				{
					Moo::rc().changeMode( Moo::rc().modeIndex(),
						!Moo::rc().windowed() );
				}
			}
			break;

		default:
			handled = false;
			break;
	}

	return handled;
}

void ApplicationInput::disableModeSwitch()
{
	disableModeSwitch_ = true;
}

/**
 *	This function is the output stream operator for ApplicationInput.
 */
std::ostream& operator<<(std::ostream& o, const ApplicationInput& t)
{
	o << "ApplicationInput\n";
	return o;
}

BW_END_NAMESPACE

// application_input.cpp
