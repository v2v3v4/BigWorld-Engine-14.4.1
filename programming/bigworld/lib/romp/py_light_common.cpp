#include "pch.hpp"

#include "py_light_common.hpp"
#include <moo/moo_math.hpp>

// -----------------------------------------------------------------------------
// Section: Script converters
// -----------------------------------------------------------------------------

BW_BEGIN_NAMESPACE

namespace Script
{
	int setData( PyObject * pObject, Moo::Colour & colour,
		const char * varName )
	{
		int ret = Script::setData( pObject, reinterpret_cast<Vector4&>( colour ), varName );
		if (ret == 0) reinterpret_cast<Vector4&>( colour ) *= 1.f/255.f;
		return ret;
	}

	PyObject * getData( const Moo::Colour & colour )
	{
		return Script::getData( reinterpret_cast<const Vector4&>( colour ) * 255.f );
	}
}

BW_END_NAMESPACE
