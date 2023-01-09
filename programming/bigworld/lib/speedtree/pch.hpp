#ifndef SPEEDTREE_PCH_HPP
#define SPEEDTREE_PCH_HPP

#if PCH_SUPPORT
#include "cstdmf/cstdmf_lib.hpp"
#include "math/math_lib.hpp"
#include "moo/moo_lib.hpp"
#include "physics2/bsp.hpp"
#include "pyscript/script_math.hpp"
#include "resmgr/resmgr_lib.hpp"

#include "speedtree_config_lite.hpp"
#if SPEEDTREE_SUPPORT
#include <SpeedTreeRT.h>
#include <SpeedTreeAllocator.h>
#endif // SPEEDTREE_SUPPORT
#endif // PCH_SUPPORT

#endif // SPEEDTREE_PCH_HPP
