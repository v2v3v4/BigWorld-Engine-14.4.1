// this file groups all external interfaces, so that the makefile
// doesn't have to include them from other directories.

#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "../baseappmgr/baseappmgr_interface.cpp"
#include "../cellapp/cellapp_interface.cpp"
#include "../cellappmgr/cellappmgr_interface.cpp"

#include "connection/client_interface.cpp"
#include "db/dbapp_interface.cpp"
