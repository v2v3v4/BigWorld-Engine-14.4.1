CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( clear_auto_load )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	clear_auto_load.cpp
	clear_auto_load.hpp
	main.cpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( clear_auto_load ${ALL_SRCS} )

BW_TARGET_LINK_LIBRARIES( clear_auto_load
	chunk
	db_storage
	db_storage_mysql
	entitydef
	pyscript
	script
	server
)

BW_PROJECT_CATEGORY( clear_auto_load "Tools" )
