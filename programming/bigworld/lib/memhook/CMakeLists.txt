CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( memhook )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	memhook.cpp
	memhook.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( memhook 
	${ALL_SRCS}
)

BW_PROJECT_CATEGORY( memhook "BigWorld Libs" )
