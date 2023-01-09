CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( waypoint_generator )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )


SET( ALL_SRCS
	chunk_view.cpp
	chunk_view.hpp
	pch.cpp
	pch.hpp
	waypoint_flood.cpp
	waypoint_flood.hpp
	waypoint_generator.cpp
	waypoint_generator.hpp
	waypoint_view.cpp
	waypoint_view.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${ALL_SRCS}
)
BW_ADD_LIBRARY( waypoint_generator ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( waypoint_generator INTERFACE
	chunk
	cstdmf
	math
	resmgr
	)

BW_PRECOMPILED_HEADER( waypoint_generator pch.hpp )
BW_PROJECT_CATEGORY( waypoint_generator "BigWorld Libs" )
