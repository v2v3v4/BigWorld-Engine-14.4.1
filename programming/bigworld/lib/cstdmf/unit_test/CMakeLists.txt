CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( cstdmf_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
	pch.cpp
	pch.hpp
	test_aligned_allocate.cpp
	test_allocate.cpp
	test_asserts.cpp
	test_background_file_writer.cpp
	test_bgtasks.cpp
	test_binary_file.cpp
	test_bit_reader.cpp
	test_bw_containers.cpp
	test_bw_snprintf.cpp
	test_bw_string_ref.cpp
	test_bw_util.cpp
	test_checksum_stream.cpp
	test_circular_queue.cpp
	test_concurrency.cpp
	#test_cstring.cpp
	test_debug.cpp
	test_dependency_graph.cpp
	test_dogwatch.cpp
	test_float.cpp
	test_log_meta_data.cpp
	test_log_msg.cpp
	test_md5.cpp
	test_memory_debug.cpp
	test_static_array.cpp
	#test_string_builder.cpp
	test_string_utils.cpp
	test_stringmap.cpp
	test_time_queue.cpp
)

IF( NOT BW_IS_SERVER )
	LIST( APPEND ALL_SRCS
		test_object_pool.cpp
	)
ENDIF()

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_EXECUTABLE( cstdmf_unit_test ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( cstdmf_unit_test
	cstdmf
	unit_test_lib
	)

# Disable warning 4819: non-ASCII characters in source
SET_SOURCE_FILES_PROPERTIES( test_string_utils.cpp
	PROPERTIES COMPILE_FLAGS "/wd\"4819\"" )

BW_ADD_TEST( cstdmf_unit_test )

BW_PRECOMPILED_HEADER( cstdmf_unit_test pch.hpp )
BW_PROJECT_CATEGORY( cstdmf_unit_test "Unit Tests" )
