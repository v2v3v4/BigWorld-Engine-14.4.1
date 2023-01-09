CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( cellappmgr )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
	bsp_node.cpp 
	bsp_node.hpp 
	cellapp.cpp 
	cellapp.hpp 
	cellapp.ipp 
	cell_app_death_handler.cpp 
	cell_app_death_handler.hpp 
	cell_app_group.cpp 
	cell_app_group.hpp 
	cell_app_groups.cpp 
	cell_app_groups.hpp 
	cell_app_load_config.cpp 
	cell_app_load_config.hpp 
	cellappmgr.cpp 
	cellappmgr.hpp 
	cellappmgr.ipp 
	cellapp_comparer.cpp
	cellapp_comparer.hpp
	cellappmgr_config.cpp 
	cellappmgr_config.hpp 
	cellappmgr_viewer_server.cpp 
	cellappmgr_viewer_server.hpp 
	cellapps.cpp 
	cellapps.hpp 
	cell_data.cpp 
	cell_data.hpp 
	cell_data.ipp 
	cells.cpp 
	cells.hpp 
	entity_bound_levels.cpp
	entity_bound_levels.hpp
	internal_node.cpp 
	internal_node.hpp 
	login_conditions_config.cpp 
	login_conditions_config.hpp 
	message_handlers.cpp 
	shutdown_handler.cpp 
	shutdown_handler.hpp 
	space.cpp 
	space.hpp 
	space.ipp
	watcher_forwarding_cellapp.cpp 
	watcher_forwarding_cellapp.hpp 
	../cellapp/cellapp_interface.cpp 
	../cellapp/cellapp_interface.hpp 
	../baseappmgr/baseappmgr_interface.cpp 
	../baseappmgr/baseappmgr_interface.hpp 
	../../lib/db/dbapp_interface.cpp 
	../../lib/db/dbapp_interface.hpp 
	cellappmgr_interface.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( cellappmgr ${ALL_SRCS} )

TARGET_INCLUDE_DIRECTORIES( cellappmgr PRIVATE ${BW_SOURCE_DIR} )
TARGET_INCLUDE_DIRECTORIES( cellappmgr PRIVATE ${BW_SOURCE_DIR}/server )

BW_TARGET_LINK_LIBRARIES( cellappmgr
	server
	network
	resmgr
	math
	cstdmf
	zip
	db
)

BW_PROJECT_CATEGORY( cellappmgr "Executables" )
