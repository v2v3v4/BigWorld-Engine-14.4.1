CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( detour )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

BW_REMOVE_COMPILE_FLAGS( /WX )

INCLUDE_DIRECTORIES( include )
INCLUDE_DIRECTORIES( ../DebugUtils/Include )
INCLUDE_DIRECTORIES( ../DetourTileCache/include )

SET( DEBUGUTILS_SRCS
	../DebugUtils/Include/DebugDraw.h
	../DebugUtils/Include/DetourDebugDraw.h
	../DebugUtils/Source/DebugDraw.cpp
	../DebugUtils/Source/DetourDebugDraw.cpp
)
SOURCE_GROUP( "DebugUtils" FILES ${DEBUGUTILS_SRCS} )


SET( TILECACHEBUILDER_SRCS
	../DetourTileCache/Include/DetourTileCacheBuilder.h
	../DetourTileCache/Source/DetourTileCacheBuilder.cpp
)
SOURCE_GROUP( "TileCacheBuilder" FILES ${TILECACHEBUILDER_SRCS} )


SET( DETOUR_SRCS
	Include/DetourAlloc.h
	Include/DetourAssert.h
	Include/DetourCommon.h
	Include/DetourNavMesh.h
	Include/DetourNavMeshBuilder.h
	Include/DetourNavMeshQuery.h
	Include/DetourNode.h
	Include/DetourStatus.h
	Source/DetourAlloc.cpp
	Source/DetourCommon.cpp
	Source/DetourNavMesh.cpp
	Source/DetourNavMeshBuilder.cpp
	Source/DetourNavMeshQuery.cpp
	Source/DetourNode.cpp
)
SOURCE_GROUP( "Detour" FILES ${DETOUR_SRCS} )


ADD_LIBRARY( detour 
	${DEBUGUTILS_SRCS}
	${TILECACHEBUILDER_SRCS}
	${DETOUR_SRCS}
)

BW_PROJECT_CATEGORY( detour "ThirdParty" )
