#ifndef PY_LIGHT_COMMON_HPP
#define PY_LIGHT_COMMON_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE
namespace Script
{
	int setData( PyObject * pObject, Moo::Colour & colour,
		const char * varName = "" );
	PyObject * getData( const Moo::Colour & colour );
}
BW_END_NAMESPACE

#endif
