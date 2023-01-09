CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( dbapp_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

BW_ADD_EXECUTABLE( dbapp_unit_test ${BLOB_SRCS} )
BW_TARGET_LINK_LIBRARIES( dbapp_unit_test
	cstdmf
	)

BW_ADD_TEST( dbapp_unit_test )

BW_PROJECT_CATEGORY( dbapp_unit_test "Unit Tests" )
