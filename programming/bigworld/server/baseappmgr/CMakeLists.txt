CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( baseappmgr )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp 
	baseapp.cpp 
	baseapp.hpp 
	baseappmgr.cpp 
	baseappmgr.hpp 
	baseappmgr_config.cpp 
	baseappmgr_config.hpp 
	login_conditions_config.cpp 
	login_conditions_config.hpp 
	message_handlers.cpp 
	reply_handlers.cpp 
	reply_handlers.hpp 
	watcher_forwarding_baseapp.cpp 
	watcher_forwarding_baseapp.hpp 
	../baseapp/baseapp_int_interface.cpp 
	../baseapp/baseapp_int_interface.hpp 
	../loginapp/login_int_interface.cpp 
	../loginapp/login_int_interface.hpp 
	../../lib/db/dbapp_interface.cpp 
	../../lib/db/dbapp_interface.hpp 
	baseappmgr_interface.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( baseappmgr ${ALL_SRCS} )

TARGET_INCLUDE_DIRECTORIES( baseappmgr PRIVATE ${BW_SOURCE_DIR} )
TARGET_INCLUDE_DIRECTORIES( baseappmgr PRIVATE ${BW_SOURCE_DIR}/server )

BW_TARGET_LINK_LIBRARIES( baseappmgr
	server
	network
	resmgr
	math
	cstdmf
	zip
	db
)

BW_PROJECT_CATEGORY( baseappmgr "Executables" )
