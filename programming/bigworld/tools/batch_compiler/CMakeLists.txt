CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( batch_compiler )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
	-DBW_ASSET_PIPELINE
)

INCLUDE( BWStandardProject )

SET( RESOURCE_FILES_SRCS
	app.rc
)
SOURCE_GROUP( "Resources" FILES ${RESOURCE_FILES_SRCS} )

SET( ALL_SRCS
	batch_compiler.cpp
	batch_compiler.hpp
	../plugin_system/plugin.hpp
	../plugin_system/plugin_loader.cpp
	../plugin_system/plugin_loader.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${RESOURCE_FILES_SRCS}
	${ALL_SRCS}
)

BW_ADD_ASSETPIPELINE_EXE( batch_compiler ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( batch_compiler PRIVATE
	compiler
	conversion
	cstdmf
	resmgr
)

ADD_DEPENDENCIES( batch_compiler bsp_converter )
ADD_DEPENDENCIES( batch_compiler effect_converter )
ADD_DEPENDENCIES( batch_compiler texformat_converter )
ADD_DEPENDENCIES( batch_compiler texture_converter )
ADD_DEPENDENCIES( batch_compiler hierarchical_config_converter )
ADD_DEPENDENCIES( batch_compiler space_converter )
ADD_DEPENDENCIES( batch_compiler wot_space_converter )
ADD_DEPENDENCIES( batch_compiler visual_processor )
ADD_DEPENDENCIES( batch_compiler primitive_processor )
