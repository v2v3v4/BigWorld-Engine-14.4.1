CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( recast )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

BW_REMOVE_COMPILE_FLAGS( /WX )

INCLUDE_DIRECTORIES( include )
INCLUDE_DIRECTORIES( ../DebugUtils/Include )


SET( RECAST_SRCS
	Include/Recast.h
	Include/RecastAlloc.h
	Include/RecastAssert.h
	Source/Recast.cpp
	Source/RecastAlloc.cpp
	Source/RecastArea.cpp
	Source/RecastContour.cpp
	Source/RecastFilter.cpp
	Source/RecastLayers.cpp
	Source/RecastMesh.cpp
	Source/RecastMeshDetail.cpp
	Source/RecastRasterization.cpp
	Source/RecastRegion.cpp
)
SOURCE_GROUP( "Recast" FILES ${RECAST_SRCS} )


SET( DEBUGUTILS_SRCS
	../DebugUtils/Include/DebugDraw.h
	../DebugUtils/Include/RecastDebugDraw.h
	../DebugUtils/Source/DebugDraw.cpp
	../DebugUtils/Source/RecastDebugDraw.cpp
)
SOURCE_GROUP( "DebugUtils" FILES ${DEBUGUTILS_SRCS} )




ADD_LIBRARY( recast 
	${RECAST_SRCS}
	${DEBUGUTILS_SRCS}
)

BW_PROJECT_CATEGORY( recast "ThirdParty" )
