CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( snapshot_helper )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	snapshot_helper.cpp
	snapshot_constants.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( snapshot_helper ${ALL_SRCS} )


BW_TARGET_LINK_LIBRARIES( snapshot_helper
	server
)

BW_PROJECT_CATEGORY( snapshot_helper "Tools" )
