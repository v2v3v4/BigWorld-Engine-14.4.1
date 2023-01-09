#include "pch.hpp"
#include "filter_quad.hpp"

#ifndef CODE_INLINE
#include "filter_quad.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "PostProcessing", 0 )


BW_BEGIN_NAMESPACE

namespace PostProcessing
{

// Python statics
PY_TYPEOBJECT( FilterQuad )

PY_BEGIN_METHODS( FilterQuad )	
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( FilterQuad )	
PY_END_ATTRIBUTES()


FilterQuad::FilterQuad( PyTypeObject *pType ):
	PyObjectPlus( pType )
{
}


FilterQuad::~FilterQuad()
{
}


void FilterQuad::draw()
{
}

}	//namespace PostProcessing

PY_SCRIPT_CONVERTERS( PostProcessing::FilterQuad )

BW_END_NAMESPACE

// filter_quad.cpp
