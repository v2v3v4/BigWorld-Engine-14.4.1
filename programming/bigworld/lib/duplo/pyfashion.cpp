#include "pch.hpp"
#include "pyfashion.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyFashion
// -----------------------------------------------------------------------------


PY_TYPEOBJECT( PyFashion )

PY_BEGIN_METHODS( PyFashion )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyFashion )
PY_END_ATTRIBUTES()



/**
 *	Default makeCopy method
 */
PyFashion * PyFashion::makeCopy( PyModel * pModel, const char * attrName )
{
	BW_GUARD;
	Py_INCREF( this );
	return this;
}


BW_END_NAMESPACE

// pyfashion.cpp
