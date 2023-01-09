CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( dbappmgr_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

BW_ADD_EXECUTABLE( dbappmgr_unit_test ${BLOB_SRCS} )
BW_TARGET_LINK_LIBRARIES( dbappmgr_unit_test
	cstdmf
	db
	math
	network
	resmgr
	server
	zip
	)

BW_ADD_TEST( dbappmgr_unit_test )

BW_PROJECT_CATEGORY( dbappmgr_unit_test "Unit Tests" )
