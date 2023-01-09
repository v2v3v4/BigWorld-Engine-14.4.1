// this file groups all external interfaces, so that the makefile
// doesn't have to include them from other directories.

#include "../baseapp/baseapp_int_interface.cpp"
#include "../cellappmgr/cellappmgr_interface.cpp"

#include "connection/baseapp_ext_interface.hpp"
#include "db/dbapp_interface.cpp"

#define DEFINE_INTERFACE_HERE
#include "connection/baseapp_ext_interface.hpp"

#include "connection/client_interface.hpp"

#define DEFINE_INTERFACE_HERE
#include "connection/client_interface.hpp"

// external_interfaces.cpp
