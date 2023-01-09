CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( test_converter )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
)

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	../../../plugin_system/plugin.hpp
	plugin_main.cpp
	test_converter.cpp
	test_converter.hpp
	test_conversion_rule.cpp
	test_conversion_rule.hpp
	test_root_conversion_rule.cpp
	test_root_conversion_rule.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( test_converter SHARED
	${ALL_SRCS}
)

BW_TARGET_LINK_LIBRARIES( test_converter PRIVATE
	appmgr
	asset_pipeline
	compiler
	conversion
	cstdmf
	dependency
	discovery
	resmgr
	)

SET_TARGET_PROPERTIES( test_converter PROPERTIES
		DEBUG_OUTPUT_NAME
		"testconverter_d" )

SET_TARGET_PROPERTIES( test_converter PROPERTIES
		HYBRID_OUTPUT_NAME
		"testconverter" )

BW_PROJECT_CATEGORY( test_converter "Unit Tests/Asset Pipeline" )

BW_SET_BINARY_DIR( test_converter "${BW_GAME_DIR}/unit_tests/${BW_PLATFORM}" )
