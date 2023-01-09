CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( cellapp_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
	test_buffered_ghost_messages.cpp
	../profile.cpp
	../profile.hpp
	../profile.ipp
	../buffered_ghost_message.cpp
	../buffered_ghost_message.hpp
	../buffered_ghost_message_queue.cpp
	../buffered_ghost_message_queue.hpp
	../buffered_ghost_messages.cpp
	../buffered_ghost_messages.hpp
	../buffered_ghost_messages_for_entity.cpp
	../buffered_ghost_messages_for_entity.hpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

BW_ADD_EXECUTABLE( cellapp_unit_test ${BLOB_SRCS} )
BW_TARGET_LINK_LIBRARIES( cellapp_unit_test
	server 
	resmgr 
	network 
	math 
	cstdmf 
	zip
	)

BW_ADD_TEST( cellapp_unit_test )

BW_PROJECT_CATEGORY( cellapp_unit_test "Unit Tests" )
