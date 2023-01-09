CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( bwentity_unit_test )

INCLUDE( BWStandardProject )

IF ( NOT BW_USE_BWENTITY_DLL )
	MESSAGE( FATAL_ERROR "BW_USE_BWENTITY_DLL is not set when building bwentity unit tests" )
ENDIF()

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
	-DBWENTITY_DLL
	-DBWENTITY_DLL_IMPORT
)

SET( ALL_SRCS
	main.cpp
	pch.cpp
	pch.hpp
	test_allocation.cpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_EXECUTABLE( bwentity_unit_test ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( bwentity_unit_test
	bwentity
	unit_test_lib
	)

BW_ADD_TEST( bwentity_unit_test )
# We need a copy of the bwentity DLL to run the unit tests
BW_COPY_TARGET( bwentity_unit_test bwentity )

BW_PRECOMPILED_HEADER( bwentity_unit_test pch.hpp )
BW_PROJECT_CATEGORY( bwentity_unit_test "Unit Tests" )
