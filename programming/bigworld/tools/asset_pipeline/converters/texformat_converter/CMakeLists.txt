CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( texformat_converter )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
)

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	texformat_converter.cpp
	texformat_converter.hpp
	texformat_conversion_rule.cpp
	texformat_conversion_rule.hpp
	plugin_main.cpp
	../../../plugin_system/plugin.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_ASSETPIPELINE_DLL( texformat_converter ${ALL_SRCS} )

BW_TARGET_LINK_LIBRARIES( texformat_converter PRIVATE
	asset_pipeline
	compiler
	cstdmf
	conversion
	dependency
	discovery
	moo
	resmgr
	)

