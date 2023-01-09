#include "pch.hpp"
#include "particles.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "particles.ipp"
#endif

// Avoid LNK4221 warning - no public symbols. This happens when this file is
// compiled in non-editor build
extern const int dummyPublicSymbol = 0;

BW_END_NAMESPACE

// particles.cpp
