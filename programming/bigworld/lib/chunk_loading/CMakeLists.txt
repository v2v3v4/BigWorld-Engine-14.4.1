CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( chunk_loading )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	chunk_loading_ref_count.cpp
	chunk_loading_ref_count.hpp
	edge_geometry_mapping.cpp
	edge_geometry_mapping.hpp
	edge_geometry_mappings.cpp
	edge_geometry_mappings.hpp
	geometry_mapper.hpp
	loading_column.cpp
	loading_column.hpp
	loading_edge.cpp
	loading_edge.hpp
	preloaded_chunk_space.cpp
	preloaded_chunk_space.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( chunk_loading ${ALL_SRCS} )

BW_PROJECT_CATEGORY( chunk_loading "BigWorld Server" )
