CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( server_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
	test_entity_profiler.cpp
	test_rendezvous_hash_scheme.cpp
	test_updatables.cpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

BW_ADD_EXECUTABLE( server_unit_test ${BLOB_SRCS} )
BW_TARGET_LINK_LIBRARIES( server_unit_test
	server
	network
	cstdmf
	)

BW_ADD_TEST( server_unit_test )

BW_PROJECT_CATEGORY( server_unit_test "Unit Tests" )
