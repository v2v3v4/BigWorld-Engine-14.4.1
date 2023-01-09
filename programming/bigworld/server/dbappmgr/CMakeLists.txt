CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( dbappmgr )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp 
	dbapp.cpp
	dbapp.hpp
	dbappmgr.cpp 
	dbappmgr.hpp 
	dbappmgr_config.cpp 
	dbappmgr_config.hpp
	external_interfaces.cpp
	message_handlers.cpp
	../../lib/db/dbapp_interface.cpp
	../../lib/db/dbapp_interface.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( dbappmgr ${ALL_SRCS} )

TARGET_INCLUDE_DIRECTORIES( dbappmgr PRIVATE ${BW_SOURCE_DIR}/server )

BW_TARGET_LINK_LIBRARIES( dbappmgr
	db
	server
	network
	cstdmf
)

BW_PROJECT_CATEGORY( dbappmgr "Executables" )
