CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( terrain_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	pch.cpp
	main.cpp
	pch.hpp
	test_resource.cpp
)

IF( NOT BW_IS_SERVER )
	LIST( APPEND ALL_SRCS
		test_vertex_lod_manager.cpp
	)
ENDIF()

BW_ADD_EXECUTABLE( terrain_unit_test ${ALL_SRCS} )
BW_TARGET_LINK_LIBRARIES( terrain_unit_test
	chunk
	cstdmf
	moo
	terrain
	unit_test_lib
	)

BW_ADD_TEST( terrain_unit_test )

BW_PRECOMPILED_HEADER( terrain_unit_test pch.hpp )
BW_PROJECT_CATEGORY( terrain_unit_test "Unit Tests" )
