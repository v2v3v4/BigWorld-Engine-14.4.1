CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( db_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
	test_dbapps_gateway.cpp
	test_db_hash_schemes.cpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

BW_ADD_EXECUTABLE( db_unit_test ${BLOB_SRCS} )
BW_TARGET_LINK_LIBRARIES( db_unit_test
	cstdmf
	db
	math
	network
	server
	zip
	)

BW_ADD_TEST( db_unit_test )

BW_PROJECT_CATEGORY( db_unit_test "Unit Tests" )
