#ifndef PYSCRIPT_PCH_HPP
#define PYSCRIPT_PCH_HPP

#if defined( SCRIPT_PYTHON )
#include "script/first_include.hpp"
#endif // SCRIPT_PYTHON

#if PCH_SUPPORT

#include "cstdmf/cstdmf_lib.hpp"
#include "resmgr/resmgr_lib.hpp"
#include "network/network_lib.hpp"
#include "script/script_lib.hpp"

#include "Python.h"

#endif // PCH_SUPPORT

#endif // PYSCRIPT_PCH_HPP
