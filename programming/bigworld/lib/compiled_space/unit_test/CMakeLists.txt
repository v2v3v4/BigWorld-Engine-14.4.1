CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( compiled_space_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	pch.hpp
	pch.cpp
	main.cpp
	test_compiled_space.cpp
)

SET( BINARY_WRITERS_SRCS
	../binary_writers/binary_format_writer.cpp
	../binary_writers/binary_format_writer.hpp
	
	../binary_writers/space_writer.hpp
	../binary_writers/space_writer.cpp
	../string_table_types.hpp
	../binary_writers/string_table_writer.cpp
	../binary_writers/string_table_writer.hpp

	../asset_list_types.hpp
	../binary_writers/asset_list_writer.cpp
	../binary_writers/asset_list_writer.hpp
	
	../terrain2_types.hpp
	../binary_writers/terrain2_writer.cpp
	../binary_writers/terrain2_writer.hpp
	
	../binary_writers/octree_writer.cpp
	../binary_writers/octree_writer.hpp
)
SOURCE_GROUP( "Binary Writers" FILES ${BINARY_WRITERS_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${ALL_SRCS} 
	${BINARY_WRITERS_SRCS}
)

BW_ADD_EXECUTABLE( compiled_space_unit_test ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( compiled_space_unit_test
	chunk
	compiled_space
	cstdmf
	moo
	resmgr
	unit_test_lib
	)

BW_ADD_TEST( compiled_space_unit_test )

BW_PRECOMPILED_HEADER( compiled_space_unit_test pch.hpp )
BW_PROJECT_CATEGORY( compiled_space_unit_test "Unit Tests" )
