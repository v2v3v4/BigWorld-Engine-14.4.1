CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( compiler_unit_test )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
	-DBW_ASSET_PIPELINE
)

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
	test_compiler.cpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( compiler_unit_test 
	${ALL_SRCS}
)

BW_TARGET_LINK_LIBRARIES( compiler_unit_test PRIVATE
	cstdmf
	resmgr
	test_compiler
	unit_test_lib
	)

BW_ADD_TOOL_TEST( compiler_unit_test )

BW_PROJECT_CATEGORY( compiler_unit_test "Unit Tests/Asset Pipeline" )

ADD_DEPENDENCIES( compiler_unit_test test_converter )  