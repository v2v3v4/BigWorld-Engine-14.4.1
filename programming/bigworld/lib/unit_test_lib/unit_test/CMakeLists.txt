CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( unit_test_lib_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
)

IF( BW_PLATFORM_LINUX )
	SET( ALL_SRCS ${ALL_SRCS} 
		test_multi_proc_test_case.cpp
	)
ENDIF()

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

BW_ADD_EXECUTABLE( unit_test_lib_unit_test ${BLOB_SRCS} )
BW_TARGET_LINK_LIBRARIES( unit_test_lib_unit_test
	cstdmf
	)

BW_ADD_TEST( unit_test_lib_unit_test )

BW_PROJECT_CATEGORY( unit_test_lib_unit_test "Unit Tests" )
