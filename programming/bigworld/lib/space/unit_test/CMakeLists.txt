CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( space_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	pch.hpp
	pch.cpp
	main.cpp
	test_space.cpp
)

BW_ADD_EXECUTABLE( space_unit_test ${ALL_SRCS} )
BW_TARGET_LINK_LIBRARIES( space_unit_test
	compiled_space
	cstdmf
	unit_test_lib
	)

BW_ADD_TEST( space_unit_test )

BW_PRECOMPILED_HEADER( space_unit_test pch.hpp )
BW_PROJECT_CATEGORY( space_unit_test "Unit Tests" )
