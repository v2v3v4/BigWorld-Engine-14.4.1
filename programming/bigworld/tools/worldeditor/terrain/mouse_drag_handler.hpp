#ifndef MOUSE_DRAG_HANDLER_HPP
#define MOUSE_DRAG_HANDLER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class handles mouse dragging painting, that is, holding down the mouse
 *	and detecting when the mouse buttons has been pressed or released outside
 *	the main view window.
 */
class MouseDragHandler
{
public:
	enum MouseKey
	{
		KEY_LEFTMOUSE	= 0,
		KEY_MIDDLEMOUSE = 1,
		KEY_RIGHTMOUSE	= 2
	};

	MouseDragHandler();

	void setDragging( MouseKey key, const bool val );
	bool isDragging( MouseKey key ) const;

protected:
	bool updateDragging( bool currentState, bool & stateVar ) const; 

private:
	mutable bool 	draggingLeftMouse_;
	mutable bool 	draggingMiddleMouse_;
	mutable bool 	draggingRightMouse_;
};


BW_END_NAMESPACE

#endif // MOUSE_DRAG_HANDLER_HPP
