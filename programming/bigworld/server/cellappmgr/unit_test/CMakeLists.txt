CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( cellappmgr_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

BW_ADD_EXECUTABLE( cellappmgr_unit_test ${BLOB_SRCS} )
BW_TARGET_LINK_LIBRARIES( cellappmgr_unit_test
	cstdmf
	)

BW_ADD_TEST( cellappmgr_unit_test )

BW_PROJECT_CATEGORY( cellappmgr_unit_test "Unit Tests" )
