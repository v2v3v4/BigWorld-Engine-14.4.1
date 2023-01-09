CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( baseappmgr_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

BW_ADD_EXECUTABLE( baseappmgr_unit_test ${BLOB_SRCS} )
BW_TARGET_LINK_LIBRARIES( baseappmgr_unit_test
	cstdmf
	)

BW_ADD_TEST( baseappmgr_unit_test )

BW_PROJECT_CATEGORY( baseappmgr_unit_test "Unit Tests" )
