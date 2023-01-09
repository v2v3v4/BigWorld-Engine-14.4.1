#include "pch.hpp"

#include "py_volatile_info.hpp"

#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This method converts the input PyObject to a priority.
 *
 *	@note This method decrements the reference count of pObject.
 */
bool PyVolatileInfo::priorityFromPyObject( PyObject * pObject, float & priority )
{
	bool result = true;

	if (pObject == Py_None)
	{
		priority = -1.f;
	}
	else if (Script::setData( pObject, priority ) == 0)
	{
		if (priority < 0.f)
		{
			priority = -1.f;
		}
		else if (!isEqual( priority, VolatileInfo::ALWAYS ))
		{
			priority *= priority;
		}
	}
	else
	{
		result = false;
	}

	Py_XDECREF( pObject );

	return result;
}


/**
 *	This method converts from a priority to a Python object. We store the
 *	priority as the distance squared but return the distance in script.
 */
PyObject * PyVolatileInfo::pyObjectFromPriority( float priority )
{
	if (priority < 0.f)
	{
		Py_RETURN_NONE;
	}
	else if (isEqual( priority, VolatileInfo::ALWAYS ))
	{
		return PyFloat_FromDouble( priority );
	}
	else
	{
		return PyFloat_FromDouble( sqrt( priority ) );
	}
}


/**
 *	This function converts from a PyObject object to a VolatileInfo object.
 */
int Script::setData( PyObject * pObject, VolatileInfo & rInfo,
		const char * varName )
{
	if (!PySequence_Check( pObject ) || PyObject_Length( pObject ) != 4)
	{
		PyErr_Format(  PyExc_TypeError,
				"%s must be a sequence of length 4", varName );
		return -1;
	}

	float positionPriority = -1.f;
	float yawPriority = -1.f;
	float pitchPriority = -1.f;
	float rollPriority = -1.f;

	bool success = true;
	PyObject * pPos = PySequence_GetItem( pObject, 0 );
	success &= PyVolatileInfo::priorityFromPyObject( pPos, positionPriority );

	PyObject * pYaw = PySequence_GetItem( pObject, 1 );
	success &= PyVolatileInfo::priorityFromPyObject( pYaw, yawPriority );

	PyObject * pPitch = PySequence_GetItem( pObject, 2 );
	success &= PyVolatileInfo::priorityFromPyObject( pPitch, pitchPriority );

	PyObject * pRoll = PySequence_GetItem( pObject, 3 );
	success &= PyVolatileInfo::priorityFromPyObject( pRoll, rollPriority );

	VolatileInfo newInfo( positionPriority, yawPriority, pitchPriority,
		rollPriority );

	if (success && newInfo.isValid())
	{
		rInfo = newInfo;

		return 0;
	}

	PyErr_Format( PyExc_TypeError,
			"%s must be a sequence of 4 float or None objects "
				"(The last 3 values must be descending)", varName );
	return -1;
}


/**
 *	This function converts from a VolatileInfo object to a Python object.
 */
PyObject * Script::getData( const VolatileInfo & info )
{
	PyObject * pTuple = PyTuple_New( 4 );

	PyTuple_SetItem( pTuple, 0,
			PyVolatileInfo::pyObjectFromPriority( info.positionPriority() ) );
	PyTuple_SetItem( pTuple, 1,
			PyVolatileInfo::pyObjectFromPriority( info.yawPriority() ) );
	PyTuple_SetItem( pTuple, 2,
			PyVolatileInfo::pyObjectFromPriority( info.pitchPriority() ) );
	PyTuple_SetItem( pTuple, 3,
			PyVolatileInfo::pyObjectFromPriority( info.rollPriority() ) );

	return pTuple;
}

BW_END_NAMESPACE

// py_volatile_info.cpp
