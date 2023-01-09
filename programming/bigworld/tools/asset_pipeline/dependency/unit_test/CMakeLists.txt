CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( dependency_unit_test )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
	-DBW_ASSET_PIPELINE
)

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
	test_serialisation.cpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( dependency_unit_test 
	${ALL_SRCS}
)

BW_TARGET_LINK_LIBRARIES( dependency_unit_test
	cstdmf
	dependency
	unit_test_lib
	)

BW_ADD_TOOL_TEST( dependency_unit_test )

BW_PROJECT_CATEGORY( dependency_unit_test "Unit Tests/Asset Pipeline" )
