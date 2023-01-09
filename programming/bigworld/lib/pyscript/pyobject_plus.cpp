/**
 * 	@file
 *
 * 	This file implements the PyObjectPlus class.
 *
 * 	@ingroup script
 */


#include "pch.hpp"

#include "pyobject_plus.hpp"

#include "cstdmf/debug.hpp"
#include "stl_to_py.hpp"
#include "script.hpp"


DECLARE_DEBUG_COMPONENT2( "Script", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Static data
// -----------------------------------------------------------------------------

// Base needs to be 0.
PY_GENERAL_TYPEOBJECT_WITH_BASE( PyObjectPlus, 0 )

PY_BEGIN_METHODS( PyObjectPlus )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyObjectPlus )
PY_END_ATTRIBUTES()

// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param pType	The type of this object.
 *
 *	@param isInitialised	Specifies whether this object has had its Python
 *		data initialised. If the object was created with new, this should be
 *		false. If it was created with PyType_GenericAlloc or something similar,
 *		this should be true.
 */
PyObjectPlus::PyObjectPlus( PyTypeObject * pType, bool isInitialised )
{
	if (PyType_Ready( pType ) < 0)
	{
		ERROR_MSG( "PyObjectPlus: Type %s is not ready\n", pType->tp_name );
	}

	if (!isInitialised)
	{
		PyObject_Init( this, pType );
	}
}


/**
 *	Destructor should only be called from Python when its reference count
 *	reaches zero.
 */
PyObjectPlus::~PyObjectPlus()
{
	MF_ASSERT_DEV(this->ob_refcnt == 0);
#ifdef Py_TRACE_REFS
	MF_ASSERT( _ob_next == NULL && _ob_prev == NULL );
#endif
}


// -----------------------------------------------------------------------------
// Section: PyObjectPlus - general
// -----------------------------------------------------------------------------

/**
 *	This method returns the attribute with the given name.
 *
 *	@param attr	The name of the attribute.
 *
 *	@return		The value associated with the input name.
 */
ScriptObject PyObjectPlus::pyGetAttribute( const ScriptString & attrObj )
{
	return ScriptObject(
		PyObject_GenericGetAttr( this, attrObj.get() ),
		ScriptObject::FROM_NEW_REFERENCE );
}


/**
 *	This method sets the attribute with the given name to the input value.
 */
bool PyObjectPlus::pySetAttribute( const ScriptString & attrObj,
	const ScriptObject & value )
{
	return (PyObject_GenericSetAttr( this, attrObj.get(), value.get() ) == 0);
}


/**
 *	This method deletes the attribute with the given name.
 */
bool PyObjectPlus::pyDelAttribute( const ScriptString & attrObj )
{
	return this->pySetAttribute( attrObj, ScriptObject() );
}


/**
 *	This method returns the representation of this object as a string.
 */
PyObject * PyObjectPlus::pyRepr()
{
	char	str[512];
	bw_snprintf( str, sizeof(str),
#ifdef _WIN32
			"%s at 0x%p",
#else
			"%s at %p",
#endif
			this->typeName(), this );

	return PyString_FromString( str );
}

/**
 *	This method is called just before the PyObject is deleted. It is useful
 *	for cleaning up resources before the class destructor is called.
 */
void PyObjectPlus::pyDel() 
{
}


/**
 *	Base class implementation of _pyNew. Always fails.
 */
PyObject * PyObjectPlus::_pyNew( PyTypeObject * pType, PyObject *, PyObject * )
{
	PyErr_Format( PyExc_TypeError, "%s() "
		"Cannot directly construct objects of this type",
		pType->tp_name );
	return NULL;
}


const PyGetSetDef * PyObjectPlus::searchAttributes( const PyGetSetDef *attrs,
		const char *name )
{
	for (; attrs->name != NULL; attrs++)
	{
		if (strcmp( attrs->name, name ) == 0)
		{
			return attrs;
		}
	}
	return NULL;
}


const PyMethodDef * PyObjectPlus::searchMethods( const PyMethodDef *methods,
		const char *name )
{
	for (; methods->ml_name != NULL; methods++)
	{
		if (strcmp( methods->ml_name, name ) == 0)
		{
			return methods;
		}
	}
	return NULL;
}


// -----------------------------------------------------------------------------
// Section: PyObjectPlusWithWeakReference
// -----------------------------------------------------------------------------


PY_TYPEOBJECT_WITH_WEAKREF( PyObjectPlusWithWeakReference )

PY_BEGIN_METHODS( PyObjectPlusWithWeakReference )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyObjectPlusWithWeakReference )
PY_END_ATTRIBUTES()

BW_END_NAMESPACE

// pyobject_plus.cpp
