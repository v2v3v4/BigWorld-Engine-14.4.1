CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( resmgr_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	test_bwresource.cpp
	test_file_system.cpp
	test_xml_section.cpp
	test_path_resource.cpp
	test_packed_section.cpp
	test_zip_section.cpp
	main.cpp
	test_data_resource.cpp
	test_harness.hpp
	pch.hpp
	pch.cpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

BW_ADD_EXECUTABLE( resmgr_unit_test ${BLOB_SRCS} )
BW_TARGET_LINK_LIBRARIES( resmgr_unit_test
	cstdmf
	resmgr
	unit_test_lib
	# external libraries
	winmm
	)

BW_ADD_TEST( resmgr_unit_test )

BW_PRECOMPILED_HEADER( resmgr_unit_test pch.hpp )
BW_PROJECT_CATEGORY( resmgr_unit_test "Unit Tests" )
