#include "pch.hpp"
#include "phase.hpp"

#ifndef CODE_INLINE
#include "phase.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "PostProcessing", 0 )


BW_BEGIN_NAMESPACE

namespace PostProcessing
{
// Python statics
PY_TYPEOBJECT( Phase )

PY_BEGIN_METHODS( Phase )	
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Phase )	
PY_END_ATTRIBUTES()


Phase::Phase( PyTypeObject *pType ):
	PyObjectPlus( pType )
{
}


Phase::~Phase()
{
}

} //namespace PostProcessing


PY_SCRIPT_CONVERTERS( PostProcessing::Phase )

BW_END_NAMESPACE

// phase.cpp
