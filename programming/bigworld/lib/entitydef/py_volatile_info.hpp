#ifndef PY_VOLATILE_INFO_HPP
#define PY_VOLATILE_INFO_HPP

#include "Python.h"

#include "volatile_info.hpp"


BW_BEGIN_NAMESPACE

namespace Script
{
	int setData( PyObject * pObject, VolatileInfo & rInfo,
			const char * varName = "" );
	PyObject * getData( const VolatileInfo & info );
}

namespace PyVolatileInfo
{

	bool priorityFromPyObject( PyObject * pObject, float & priority );
	PyObject * pyObjectFromPriority( float priority );
}

BW_END_NAMESPACE

#endif // PY_VOLATILE_INFO_HPP
