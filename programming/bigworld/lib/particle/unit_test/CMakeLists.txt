CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( particle_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	pch.cpp
	main.cpp
	pch.hpp
	test_particle.cpp
	)

BW_ADD_EXECUTABLE( particle_unit_test ${ALL_SRCS} )
BW_TARGET_LINK_LIBRARIES( particle_unit_test
	cstdmf
	particle
	unit_test_lib
	)

BW_ADD_TEST( particle_unit_test )

BW_PRECOMPILED_HEADER( particle_unit_test pch.hpp )
BW_PROJECT_CATEGORY( particle_unit_test "Unit Tests" )
