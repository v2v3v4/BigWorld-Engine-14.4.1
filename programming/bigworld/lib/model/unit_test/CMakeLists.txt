CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( model_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	pch.cpp
	main.cpp
	pch.hpp
	test_model.cpp
	test_super_model_lodding.cpp
	test_super_model_node_tree.cpp
)

BW_ADD_EXECUTABLE( model_unit_test ${ALL_SRCS} )
BW_TARGET_LINK_LIBRARIES( model_unit_test
	chunk
	cstdmf
	duplo
	model
	moo
	pyscript
	resmgr
	unit_test_lib
)

BW_ADD_TEST( model_unit_test )

BW_PRECOMPILED_HEADER( model_unit_test pch.hpp )
BW_PROJECT_CATEGORY( model_unit_test "Unit Tests" )
