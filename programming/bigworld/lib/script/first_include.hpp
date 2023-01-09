#ifndef FIRST_INCLUDE_HPP_
#define FIRST_INCLUDE_HPP_

// Since Python may define some pre-processor definitions which affect the
// standard headers on some systems, you must include Python.h before any
// standard headers are included.
// See https://docs.python.org/2/c-api/intro.html#include-files

// Just in case Python pulls in <inttypes.h> before
// cstdmf/stdmf.hpp has had a chance to.
#define __STDC_FORMAT_MACROS

#if defined( SCRIPT_PYTHON )
#include "Python.h"
#endif

#else	/* FIRST_INCLUDE_HPP_ */
#warning "This file is supposed to be included only once to a precompiled header or directly to a .cpp file"
#endif	/* FIRST_INCLUDE_HPP_ */
