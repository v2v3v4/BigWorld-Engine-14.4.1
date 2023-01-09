#ifndef INPUT_CURSOR_HPP
#define INPUT_CURSOR_HPP

#include "input.hpp"
#include "pyscript/pyobject_plus.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This abstract base class is an interface for 
 *  all cursor like input handlers.
 */
class InputCursor : public PyObjectPlus, public InputHandler
{
	Py_Header( InputCursor, PyObjectPlus )

public:
	virtual void focus( bool focus );
	virtual void activate();
	virtual void deactivate();

protected:
	InputCursor( PyTypeObject * pType = &s_type_ );
	virtual ~InputCursor() {}
};

BW_END_NAMESPACE

#endif // INPUT_CURSOR_HPP
