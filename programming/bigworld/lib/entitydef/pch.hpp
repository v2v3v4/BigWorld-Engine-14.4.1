#ifndef ENTITYDEF_PCH_HPP
#define ENTITYDEF_PCH_HPP

#include "script/first_include.hpp"

#if PCH_SUPPORT

#include "cstdmf/cstdmf_lib.hpp"
#include "network/network_lib.hpp"
#include "pyscript/pyscript_lib.hpp"
#include "resmgr/resmgr_lib.hpp"
#include "script/script_lib.hpp"

#ifndef MF_SERVER

#include "data_description.hpp"
#include "entity_description.hpp"
#include "entity_description_map.hpp"
#include "method_description.hpp"
#include "volatile_info.hpp"
#include "py_volatile_info.hpp"

#endif // ndef MF_SERVER
#endif // PCH_SUPPORT

#endif // ENTITYDEF_PCH_HPP
