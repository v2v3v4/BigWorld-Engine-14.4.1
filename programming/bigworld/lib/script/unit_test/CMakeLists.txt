CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( script_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	test_scriptobject.cpp
	test_scriptmodule.cpp
	test_scriptargs.cpp
	test_scriptdict.cpp
	test_scriptsequence.cpp
	test_scripttuple.cpp
	test_scriptlist.cpp
	test_scriptmapping.cpp
	main.cpp
	pch.hpp
	pch.cpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_EXECUTABLE( script_unit_test ${BLOB_SRCS} )
BW_TARGET_LINK_LIBRARIES( script_unit_test
	cstdmf
	pyscript
	resmgr
	script
	unit_test_lib
	)

BW_ADD_TEST( script_unit_test )

BW_PRECOMPILED_HEADER( script_unit_test pch.hpp )
BW_PROJECT_CATEGORY( script_unit_test "Unit Tests" )
