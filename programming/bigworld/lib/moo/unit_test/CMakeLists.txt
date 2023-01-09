CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( moo_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
	pch.hpp
	pch.cpp
	animation_test_helper.hpp
	animation_test_helper.cpp
	#shader_test.cpp
	test_animation.cpp
	test_animation_channel.cpp
	test_render_queue.cpp
	test_visual.cpp
	test_vertex_format.cpp
	test_vertex_formats_conversions.cpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_EXECUTABLE( moo_unit_test ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( moo_unit_test
	cstdmf
	math
	moo
	physics2
	resmgr
	unit_test_lib
	)

BW_ADD_TEST( moo_unit_test  )

BW_PRECOMPILED_HEADER( moo_unit_test pch.hpp )
BW_PROJECT_CATEGORY( moo_unit_test "Unit Tests" )
