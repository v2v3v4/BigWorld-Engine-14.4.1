CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( visual_processor )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
)

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	visual_processor.cpp
	visual_processor.hpp
	plugin_main.cpp
	../../../plugin_system/plugin.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_ASSETPIPELINE_DLL( visual_processor ${ALL_SRCS} )

BW_TARGET_LINK_LIBRARIES( visual_processor PRIVATE
	asset_pipeline
	compiler
	cstdmf
	conversion
	dependency
	discovery
	moo
	resmgr
	)

