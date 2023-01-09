CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( primitive_processor )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
)

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	primitive_processor.cpp
	primitive_processor.hpp
	plugin_main.cpp
	../../../plugin_system/plugin.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_ASSETPIPELINE_DLL( primitive_processor ${ALL_SRCS} )

BW_TARGET_LINK_LIBRARIES( primitive_processor PRIVATE
	asset_pipeline
	compiler
	cstdmf
	conversion
	dependency
	discovery
	moo
	resmgr
	)

