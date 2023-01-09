CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( loginapp )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp 
	bw_config_login_challenge_config.cpp
	bw_config_login_challenge_config.cpp
	client_login_request.cpp
	client_login_request.hpp
	database_reply_handler.cpp 
	database_reply_handler.hpp 
	loginapp.cpp 
	loginapp.hpp 
	loginapp_config.cpp 
	loginapp_config.hpp 
	message_handlers.cpp 
	status_check_watcher.cpp 
	status_check_watcher.hpp 
	../../lib/db/dbapp_interface.cpp 
	../../lib/db/dbapp_interface.hpp 
	../../lib/db/dbappmgr_interface.cpp 
	../../lib/db/dbappmgr_interface.hpp 
	login_int_interface.hpp
)

SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( loginapp ${ALL_SRCS} )

TARGET_INCLUDE_DIRECTORIES( loginapp PRIVATE ${BW_SOURCE_DIR} )
TARGET_INCLUDE_DIRECTORIES( loginapp PRIVATE ${BW_SOURCE_DIR}/server )

BW_TARGET_LINK_LIBRARIES( loginapp
	connection
	server
	network
	resmgr
	math
	cstdmf
	zip
	db
)

BW_PROJECT_CATEGORY( loginapp "Executables" )
