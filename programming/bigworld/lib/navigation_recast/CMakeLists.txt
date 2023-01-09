CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( navigation_recast )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/recastnavigation/Recast/Include )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/recastnavigation/Detour/Include )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/recastnavigation/DebugUtils/Include )


SET( ALL_SRCS
	recast_generator.cpp
	recast_generator.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( navigation_recast
	${ALL_SRCS}
)

BW_TARGET_LINK_LIBRARIES( navigation_recast INTERFACE
	chunk
	cstdmf
	detour
	recast
	)

BW_PROJECT_CATEGORY( navigation_recast "BigWorld Libs" )