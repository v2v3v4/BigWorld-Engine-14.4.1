#ifndef __TERRAIN_PCH_HPP__
#define __TERRAIN_PCH_HPP__

#if PCH_SUPPORT

#include "cstdmf/cstdmf_lib.hpp"
#include "chunk/chunk_lib.hpp"
#include "math/math_lib.hpp"
#include "physics2/worldtri.hpp"
#include "physics2/collision_callback.hpp"
#include "physics2/collision_obstacle.hpp"
#include "resmgr/resmgr_lib.hpp"
#include "scene/scene_provider.hpp"

#ifndef MF_SERVER
#include "moo/moo_lib.hpp"
#endif // MF_SERVER
#endif // PCH_SUPPORT

#endif // __TERRAIN_PCH_HPP__
