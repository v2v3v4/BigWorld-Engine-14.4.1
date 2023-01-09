#ifndef BWCLIENT_PCH_HPP
#define BWCLIENT_PCH_HPP

#if PCH_SUPPORT

#include "cstdmf/cstdmf_lib.hpp"
#include "cstdmf/binary_stream.hpp"
#include "chunk/chunk_lib.hpp"
#include "compiled_space/entity_udo_factory.hpp"

#include "connection/avatar_filter_helper.hpp"
#include "connection/filter_environment.hpp"
#include "connection/log_on_status.hpp"
#include "connection/login_handler.hpp"
#include "connection/login_interface.hpp"
#include "connection/movement_filter.hpp"
#include "connection/movement_filter_target.hpp"
#include "connection/replay_controller.hpp"
#include "connection/replay_data_file_reader.hpp"
#include "connection/replay_header.hpp"
#include "connection/server_connection_handler.hpp"
#include "connection/smart_server_connection.hpp"

#include "duplo/duplo_lib.hpp"
#include "duplo/chunk_attachment.hpp"
#include "input/input_lib.hpp"
#include "math/math_lib.hpp"
#include "model/model_lib.hpp"
#include "moo/moo_lib.hpp"
#include "network/network_lib.hpp"
#include "particle/particle_lib.hpp"
#include "physics2/worldpoly.hpp"
#include "physics2/worldtri.hpp"
#include "pyscript/pyscript_lib.hpp"
#include "resmgr/resmgr_lib.hpp"
#include "romp/romp_lib.hpp"
#include "scene/scene_lib.hpp"
#include "script/script_lib.hpp"
#include "terrain/terrain_lib.hpp"

#include "cstdmf/cstdmf_windows.hpp"

#include "Python.h"

#endif // PCH_SUPPORT

#endif // BWCLIENT_PCH_HPP
