CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( texture_converter )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
)

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	texture_converter.cpp
	texture_converter.hpp
	plugin_main.cpp
	../../../plugin_system/plugin.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_ASSETPIPELINE_DLL( texture_converter ${ALL_SRCS} )

BW_TARGET_LINK_LIBRARIES( texture_converter PRIVATE
	asset_pipeline
	compiler
	cstdmf
	conversion
	dependency
	discovery
	moo
	resmgr
	)

