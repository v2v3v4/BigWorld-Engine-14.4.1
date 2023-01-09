CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( baseapp_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
	pch.hpp
	test_recording.cpp
	test_sqlite_database.cpp
	../replay_data_file_writer.cpp
	../replay_data_file_writer.hpp
	../sqlite_database.cpp
	../sqlite_database.hpp
	../write_to_db_reply.cpp
	../write_to_db_reply.hpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

BW_ADD_EXECUTABLE( baseapp_unit_test ${BLOB_SRCS} )
BW_TARGET_LINK_LIBRARIES( baseapp_unit_test
	chunk
	scene
	connection
	entitydef
	pyscript
	server
	resmgr
	network
	sqlite
	math
	cstdmf
	zip
	)

BW_ADD_TEST( baseapp_unit_test )

BW_PROJECT_CATEGORY( baseapp_unit_test "Unit Tests" )
