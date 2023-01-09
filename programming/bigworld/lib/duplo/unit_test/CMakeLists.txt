CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( duplo_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
	pch.hpp
	pch.cpp
	test_ik_constraint_system.cpp
)

BW_ADD_EXECUTABLE( duplo_unit_test ${ALL_SRCS} )
BW_TARGET_LINK_LIBRARIES( duplo_unit_test
	chunk
	cstdmf
	duplo
	math
	moo
	model
	resmgr
	pyscript
	unit_test_lib
)

BW_ADD_TEST( duplo_unit_test  )

BW_PRECOMPILED_HEADER( duplo_unit_test pch.hpp )
BW_PROJECT_CATEGORY( duplo_unit_test "Unit Tests" )
