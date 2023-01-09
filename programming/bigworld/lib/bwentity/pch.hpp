#ifndef BWENTITY_PCH_HPP
#define BWENTITY_PCH_HPP

#if PCH_SUPPORT

#ifdef _WIN32

// no matching delete operator for placement new operators, which can't have
// one in any case. applies to the Aligned class
#pragma warning(disable: 4291)

#endif // _WIN32

#include "connection/baseapp_ext_interface.hpp"
#include "connection/client_interface.hpp"
#include "connection/entity_def_constants.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/string_builder.hpp"
#include "entitydef/base_user_data_object_description.hpp"
#include "entitydef/constants.hpp"
#include "entitydef/entity_description.hpp"
#include "entitydef/entity_description_map.hpp"
#include "entitydef/member_description.hpp"
#include "entitydef/entity_method_descriptions.hpp"
#include "resmgr/bwresource.hpp"

#endif // PCH_SUPPORT

#endif // BWENTITY_PCH_HPP
