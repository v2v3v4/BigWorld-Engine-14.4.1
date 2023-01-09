#include "pch.hpp"
#include "worldeditor/terrain/mouse_drag_handler.hpp"
#include "input/input.hpp"

BW_BEGIN_NAMESPACE

/** 
 *	MouseDragHandler Constructor, inits all member variables.
 */
MouseDragHandler::MouseDragHandler() :
	draggingLeftMouse_( false ),
	draggingMiddleMouse_( false ),
	draggingRightMouse_( false )
{
}


/** 
 *	This method is called by a client class with "true" when it receives a 
 *	Mouse Key Down event, and is called with "false" when it receives the
 *	Mouse Key Up event top end dragging. NOTE: The "isDragging" method also
 *	updates state variables in case the mouse button was released outside the
 *	client area of the window.
 *
 *	@param event	Contains the mouse button pressed or released
 *	@param tool		Tool to use. Not used in this method
 */
void MouseDragHandler::setDragging( MouseKey key, const bool val )
{
	switch (key)
	{
	case KEY_LEFTMOUSE:
		draggingLeftMouse_ = val;
		break;

	case KEY_MIDDLEMOUSE:
		draggingMiddleMouse_ = val;
		break;

	case KEY_RIGHTMOUSE:
		draggingRightMouse_ = val;
		break;
	}
}


/** 
 *	This method tells if the user is doing a valid drag in the window. A valid
 *	drag is started when the user presses down a mouse button in the window and
 *	moves the mouse inside the window without releasing the button. To start
 *	a drag, a client class calls "setDragging" with "true" when it receives a 
 *	Mouse Key Down event, and calls it with "false" when it receives the
 *	Mouse Key Up event top end dragging.
 *	NOTE: this function also updates state variables in case the mouse button
 *	was released outside the client area of the window.
 *
 *	@param key		Mouse button pressed or released
 *	@return 		True if the user is still dragging with the given key,
 *					false otherwise.
 */
bool MouseDragHandler::isDragging( MouseKey key ) const
{
	BW_GUARD;

	bool mouseKey = false;

	switch ( key )
	{
	case KEY_LEFTMOUSE:
		mouseKey = 
			updateDragging(
				InputDevices::isKeyDown( KeyCode::KEY_LEFTMOUSE ),
				draggingLeftMouse_ );
		break;

	case KEY_MIDDLEMOUSE:
		mouseKey = 
			updateDragging(
				InputDevices::isKeyDown( KeyCode::KEY_MIDDLEMOUSE ),
				draggingMiddleMouse_ );
		break;

	case KEY_RIGHTMOUSE:
		mouseKey = 
			updateDragging(
				InputDevices::isKeyDown( KeyCode::KEY_RIGHTMOUSE ),
				draggingRightMouse_ );
		break;
	}

	return mouseKey;
}


/** 
 *	This private method is used to update the internal mouse button state
 *	variables to track the mouse dragging properly.
 *
 *	@param currentState	Contains the current mouse button state
 *	@param stateVar		A ref. to the corresponding state var. to be updated
 *	@return 			The resulting state after the update.
 */
bool MouseDragHandler::updateDragging( bool currentState, bool & stateVar ) const
{
	stateVar = (!currentState ? false : stateVar);

	return stateVar;
}

BW_END_NAMESPACE

