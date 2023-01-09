#ifndef BUNDIFF_HPP
#define BUNDIFF_HPP

#include "cstdmf/bw_namespace.hpp"

#include <stdio.h>

BW_BEGIN_NAMESPACE

bool performUndiff( FILE * pDiff, FILE * pSrc, FILE * pDest );

BW_END_NAMESPACE


#endif // BUNDIFF_HPP

