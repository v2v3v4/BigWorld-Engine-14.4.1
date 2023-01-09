#include "pch.hpp"

#include "motor.hpp"
#include "pymodel.hpp"

#include "pyscript/script.hpp"

DECLARE_DEBUG_COMPONENT2( "Motor", 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( Motor )

PY_BEGIN_METHODS( Motor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Motor )
	/*~ attribute Motor.owner
	 *
	 *	This read only attribute is the PyModel which the motor is controlling.
	 *
	 *	@type	Read-Only PyModel
	 */
	PY_ATTRIBUTE( owner )
PY_END_ATTRIBUTES()


/**
 *	Get this model's owner.
 *
 *	Only in the source file so we don't have to include pymodel and script
 *	in the header file.
 */
PyObject * Motor::pyGet_owner()
{
	BW_GUARD;
	return Script::getData( pOwner_ );
}

BW_END_NAMESPACE

// motor.cpp
