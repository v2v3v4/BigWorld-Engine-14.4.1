CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( asset_pipeline )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	asset_client.cpp
	asset_client.hpp
	asset_lock.cpp
	asset_lock.hpp
	asset_pipe.cpp
	asset_pipe.hpp
	asset_server.cpp
	asset_server.hpp
	pch.cpp
	pch.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( asset_pipeline
	${BLOB_SRCS}
)

BW_TARGET_LINK_LIBRARIES( asset_pipeline INTERFACE
	cstdmf
	# external libraries
	shlwapi
	Psapi
	)

BW_PRECOMPILED_HEADER( asset_pipeline pch.hpp )
BW_PROJECT_CATEGORY( asset_pipeline "BigWorld Libs" )
