CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( hierarchical_config_converter )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
)

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	hierarchical_config_converter.cpp
	hierarchical_config_converter.hpp
	plugin_main.cpp
	../../../plugin_system/plugin.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_ASSETPIPELINE_DLL( hierarchical_config_converter ${ALL_SRCS} )

BW_TARGET_LINK_LIBRARIES( hierarchical_config_converter PRIVATE
	asset_pipeline
	compiler
	cstdmf
	conversion
	dependency
	discovery
	resmgr
)

