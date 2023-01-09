CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( chunk_scene_adapter )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	pch.hpp
	pch.cpp
	
	
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

IF( NOT BW_IS_SERVER )
	SET(FRAMEWORK_SRCS
		client_chunk_space_adapter.cpp
		client_chunk_space_adapter.hpp
	)
	SOURCE_GROUP( "Framework" FILES ${FRAMEWORK_SRCS} )
ENDIF()

BW_BLOB_SOURCES( BLOB_SRCS 
	${ALL_SRCS} 
	${FRAMEWORK_SRCS}
	)
BW_ADD_LIBRARY( chunk_scene_adapter ${BLOB_SRCS} )

IF( NOT BW_IS_SERVER )
	BW_TARGET_LINK_LIBRARIES( chunk_scene_adapter INTERFACE
		cstdmf
		chunk
		duplo
		math
		moo
		scene
		space
		terrain
		romp
	)

	IF( BW_SPEEDTREE_SUPPORT )
		BW_TARGET_LINK_LIBRARIES( chunk_scene_adapter INTERFACE speedtree )
	ENDIF()

ENDIF()



BW_PRECOMPILED_HEADER( chunk_scene_adapter pch.hpp )
BW_PROJECT_CATEGORY( chunk_scene_adapter "BigWorld Libs" )

