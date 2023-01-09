CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( db )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	db_config.cpp
	db_config.hpp
	db_hash_schemes.cpp
	db_hash_schemes.hpp
	dbapp_gateway.cpp
	dbapp_gateway.hpp
	dbapp_interface.cpp
	dbapp_interface.hpp
	dbapp_interface_utils.cpp
	dbapp_interface_utils.hpp
	dbappmgr_interface.cpp
	dbappmgr_interface.hpp
	dbapps_gateway.cpp
	dbapps_gateway.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( db
	${ALL_SRCS}
)

BW_PROJECT_CATEGORY( db "BigWorld Server" )
