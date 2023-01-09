CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( space_converter )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
	-DSPACECONVERTER_EXPORT
)

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	space_converter_dll.hpp
	space_converter.cpp
	space_converter.hpp
	plugin_main.cpp
	../../../plugin_system/plugin.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

BW_ADD_ASSETPIPELINE_DLL( space_converter ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( space_converter PRIVATE
	asset_pipeline
	chunk
	compiled_space_writers
	compiler
	cstdmf
	conversion
	dependency
	discovery
	math
	moo
	resmgr
	physics2
	space
	terrain
	)

