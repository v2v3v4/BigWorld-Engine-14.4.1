CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( waypoint )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	astar.hpp
	astar.ipp
	chunk_navigator.cpp
	chunk_navigator.hpp
	chunk_waypoint.cpp
	chunk_waypoint.hpp
	chunk_waypoint_set.cpp
	chunk_waypoint_set.hpp
	chunk_waypoint_set_data.cpp
	chunk_waypoint_set_data.hpp
	chunk_waypoint_set_state.cpp
	chunk_waypoint_set_state.hpp
	chunk_waypoint_set_state_path.cpp
	chunk_waypoint_set_state_path.hpp
	chunk_waypoint_state.cpp
	chunk_waypoint_state.hpp
	chunk_waypoint_state_path.cpp
	chunk_waypoint_state_path.hpp
	dependent_array.hpp
	girth_grid_element.hpp
	girth_grid_list.cpp
	girth_grid_list.hpp
	girth_grids.hpp
	mapped_vector3.hpp
	mapped_vector3.ipp
	navigator.cpp
	navigator.hpp
	navigator_cache.cpp
	navigator_cache.hpp
	navigator_find_result.hpp
	navloc.cpp
	navloc.hpp
	pch.cpp
	pch.hpp
	search_path_base.hpp
	waypoint.hpp
	waypoint_neighbour_iterator.cpp
	waypoint_neighbour_iterator.hpp
	waypoint_stats.cpp
	waypoint_stats.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( waypoint
	${BLOB_SRCS}
)

BW_TARGET_LINK_LIBRARIES( waypoint INTERFACE
	chunk
	cstdmf
	math
	resmgr
	)

BW_PRECOMPILED_HEADER( waypoint pch.hpp )
BW_PROJECT_CATEGORY( waypoint "BigWorld Libs" )
