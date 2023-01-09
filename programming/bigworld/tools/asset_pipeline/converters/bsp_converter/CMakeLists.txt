CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( bsp_converter )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
)

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	bsp_converter.cpp
	bsp_converter.hpp
	bsp_conversion_rule.cpp
	bsp_conversion_rule.hpp
	plugin_main.cpp
	../../../plugin_system/plugin.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_ASSETPIPELINE_DLL( bsp_converter ${ALL_SRCS} )

BW_TARGET_LINK_LIBRARIES( bsp_converter PRIVATE
	asset_pipeline
	compiler
	conversion
	cstdmf
	dependency
	discovery
	moo
	resmgr
	physics2
	)

