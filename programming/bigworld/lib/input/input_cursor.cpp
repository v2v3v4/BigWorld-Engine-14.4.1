#include "pch.hpp"

#include "input_cursor.hpp"


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( InputCursor )

PY_BEGIN_METHODS( InputCursor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( InputCursor )
PY_END_ATTRIBUTES()


/**
 *	Default constructor.
 */
InputCursor::InputCursor( PyTypeObject * pType ) :
	PyObjectPlus( pType )
{}

/**
 *	Base class focus method, which does nothing
 */
void InputCursor::focus( bool focus )
{}


/**
 *	Base class activation method, which does nothing
 */
void InputCursor::activate()
{}


/**
 *	Base class deactivation method, which does nothing
 */
void InputCursor::deactivate()
{}

BW_END_NAMESPACE

// input_cursor.cpp
