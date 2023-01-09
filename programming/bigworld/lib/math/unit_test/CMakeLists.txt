CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( math_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	test_quaternion.cpp
	main.cpp
	pch.hpp
	test_boundingbox.cpp
	pch.cpp
	test_vector3.cpp
	test_ema.cpp
	test_matrix_data.hpp
	test_matrix.cpp
	test_math_extra.cpp
)

IF( NOT BW_IS_SERVER )
	LIST( APPEND ALL_SRCS
		test_blend_transform.cpp
	)
ENDIF()

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_EXECUTABLE( math_unit_test ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( math_unit_test
	cstdmf
	math
	unit_test_lib
	)

BW_ADD_TEST( math_unit_test )

BW_PRECOMPILED_HEADER( math_unit_test pch.hpp )
BW_PROJECT_CATEGORY( math_unit_test "Unit Tests" )
