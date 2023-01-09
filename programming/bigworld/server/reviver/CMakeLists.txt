CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( reviver )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp 
	component_reviver.cpp 
	component_reviver.hpp 
	reviver.cpp 
	reviver.hpp 
	reviver_config.cpp 
	reviver_config.hpp 
	../baseappmgr/baseappmgr_interface.cpp 
	../baseappmgr/baseappmgr_interface.hpp 
	../cellappmgr/cellappmgr_interface.cpp 
	../cellappmgr/cellappmgr_interface.hpp 
	../../lib/db/dbapp_interface.cpp 
	../../lib/db/dbapp_interface.hpp 
	reviver_interface.hpp
)

SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( reviver ${ALL_SRCS} )

TARGET_INCLUDE_DIRECTORIES( reviver PRIVATE ${BW_SOURCE_DIR} )
TARGET_INCLUDE_DIRECTORIES( reviver PRIVATE ${BW_SOURCE_DIR}/server )

BW_TARGET_LINK_LIBRARIES( reviver
	db
	server
	network
	resmgr
	math
	cstdmf
	zip
)

BW_PROJECT_CATEGORY( reviver "Executables" )
