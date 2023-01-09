#include "pch.hpp"
#include "particle.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "particle.ipp"
#endif

namespace 
{
    // The compiler can complain that the .obj file that this file produces
    // has no symbols.  This do-nothing int prevents the warning.
    int noCompilerWarning   = 0;
}

BW_END_NAMESPACE

// particle.cpp
