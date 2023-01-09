CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( entitydef_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
	pch.hpp
	pch.cpp
	test_conversion.cpp
	test_datasection.cpp
	test_harness.hpp
	test_method_description.cpp
	test_script_data_sink.cpp
	test_script_data_source.cpp
	test_single_data_sink.cpp
	test_single_data_source.cpp
	test_stream.cpp
	test_streamsizes.hpp
	test_streamsizes.cpp
	test_datatype_const_iterator.cpp
	test_datatype_persistent_only_const_iterator.cpp
	${BW_SOURCE_DIR}/lib/pyscript/unit_test/integer_range_checker.cpp
	${BW_SOURCE_DIR}/lib/pyscript/unit_test/integer_range_checker.hpp
)

IF( BW_IS_SERVER )
	LIST( APPEND ALL_SRCS
		unittest_mailbox.cpp
	)
ENDIF()


BW_ADD_EXECUTABLE( entitydef_unit_test ${ALL_SRCS} )
BW_TARGET_LINK_LIBRARIES( entitydef_unit_test
	chunk
	cstdmf
	entitydef
	moo
	network
	pyscript
	resmgr
	script
	unit_test_lib
)

#This is added to avoid pulling in Umbra as a dependency.
ADD_DEFINITIONS(
	-DINDIE
)

BW_ADD_TEST( entitydef_unit_test )

BW_PRECOMPILED_HEADER( entitydef_unit_test pch.hpp )
BW_PROJECT_CATEGORY( entitydef_unit_test "Unit Tests" )
