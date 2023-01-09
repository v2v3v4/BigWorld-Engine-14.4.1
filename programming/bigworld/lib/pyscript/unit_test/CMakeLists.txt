CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( pyscript_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	test_conversion.cpp
	integer_range_checker.cpp
	test_pyobject_base.cpp
	test_python_integration.cpp
	test_python_and_stl.cpp
	test_extensions.cpp
	test_py_output_writer.hpp
	test_py_output_writer.cpp
	main.cpp
	pch.hpp
	pch.cpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_EXECUTABLE( pyscript_unit_test ${BLOB_SRCS} )
BW_TARGET_LINK_LIBRARIES( pyscript_unit_test
	cstdmf
	pyscript
	unit_test_lib
	)

BW_ADD_TEST( pyscript_unit_test )

BW_PRECOMPILED_HEADER( pyscript_unit_test pch.hpp )
BW_PROJECT_CATEGORY( pyscript_unit_test "Unit Tests" )
