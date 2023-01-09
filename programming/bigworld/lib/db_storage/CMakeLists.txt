CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( db_storage )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	billing_system.cpp 
	billing_system.hpp 
	billing_system_creator.cpp 
	billing_system_creator.hpp 
	db_engine_creator.cpp 
	db_engine_creator.hpp 
	db_entitydefs.cpp 
	db_entitydefs.hpp 
	db_status.cpp 
	db_status.hpp 
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( db_storage
	${ALL_SRCS}
)

BW_PROJECT_CATEGORY( db_storage "BigWorld Server" )
