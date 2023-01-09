CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( physics2_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	pch.cpp
	main.cpp
	pch.hpp
	test_bsptree.cpp
	test_quadtree.cpp
	test_worldtri.cpp
)

BW_ADD_EXECUTABLE( physics2_unit_test ${ALL_SRCS} )
BW_TARGET_LINK_LIBRARIES( physics2_unit_test
	cstdmf
	math
	physics2
	unit_test_lib
	)

BW_ADD_TEST( physics2_unit_test )

BW_PRECOMPILED_HEADER( physics2_unit_test pch.hpp )
BW_PROJECT_CATEGORY( physics2_unit_test "Unit Tests" )
