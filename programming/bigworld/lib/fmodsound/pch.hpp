#ifndef FMODSOUND_PCH_HPP
#define FMODSOUND_PCH_HPP

#if PCH_SUPPORT
#include "cstdmf/cstdmf_lib.hpp"
#include "math/vector3.hpp"
#include "pyscript/pyscript_lib.hpp"
#include "resmgr/datasection.hpp"

#include "fmod_config.hpp"
#if FMOD_SUPPORT
#include <fmod.hpp>
#include <fmod_errors.h>
#include <fmod_event.hpp>
#include <fmod_event_net.hpp>
#endif // FMOD_SUPPORT

#endif // PCH_SUPPORT

#endif // FMODSOUND_PCH_HPP
