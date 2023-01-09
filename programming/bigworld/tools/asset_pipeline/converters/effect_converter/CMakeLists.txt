CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( effect_converter )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
)

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	effect_converter.cpp
	effect_converter.hpp
	plugin_main.cpp
	../../../plugin_system/plugin.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_ASSETPIPELINE_DLL( effect_converter ${ALL_SRCS} )

BW_TARGET_LINK_LIBRARIES( effect_converter PRIVATE
	asset_pipeline
	compiler
	cstdmf
	conversion
	dependency
	discovery
	moo
	resmgr
	)

