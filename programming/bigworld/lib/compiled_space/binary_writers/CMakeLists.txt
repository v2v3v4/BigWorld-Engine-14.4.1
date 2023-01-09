CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( compiled_space_writers )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	pch.hpp
	pch.cpp
	
	space_writer.hpp
	space_writer.cpp
	chunk_converter.hpp
	chunk_converter.cpp

	binary_format_writer.cpp
	binary_format_writer.hpp
	
	../compiled_space_settings_types.hpp
	settings_writer.cpp
	settings_writer.hpp
	
	../string_table_types.hpp
	string_table_writer.cpp
	string_table_writer.hpp

	../asset_list_types.hpp
	asset_list_writer.cpp
	asset_list_writer.hpp
	
	../static_geometry_types.hpp
	static_geometry_writer.cpp
	static_geometry_writer.hpp

	../static_scene_types.hpp
	static_scene_writer.cpp
	static_scene_writer.hpp

	../static_scene_model_types.hpp
	static_scene_model_writer.cpp
	static_scene_model_writer.hpp

	../static_scene_water_types.hpp
	static_scene_water_writer.cpp
	static_scene_water_writer.hpp

	../static_scene_flare_types.hpp
	static_scene_flare_writer.cpp
	static_scene_flare_writer.hpp

	../static_scene_decal_types.hpp
	static_scene_decal_writer.cpp
	static_scene_decal_writer.hpp

	../terrain2_types.hpp
	terrain2_writer.cpp
	terrain2_writer.hpp

	../light_scene_types.cpp
	../light_scene_types.hpp
	light_scene_writer.cpp
	light_scene_writer.hpp

	../static_texture_streaming_types.hpp
	static_texture_streaming_writer.hpp
	static_texture_streaming_writer.cpp
	
	octree_writer.cpp
	octree_writer.hpp

	../particle_system_types.hpp
	particle_system_writer.cpp
	particle_system_writer.hpp

	entity_aggregator.cpp
	entity_aggregator.hpp
	entity_writer.cpp
	entity_writer.hpp
)

IF( BW_SPEEDTREE_SUPPORT )
	SET( ALL_SRCS ${ALL_SRCS}
	../static_scene_speed_tree_types.hpp
	static_scene_speed_tree_writer.cpp
	static_scene_speed_tree_writer.hpp
	)
ENDIF()

SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( compiled_space_writers ${ALL_SRCS} )

BW_PRECOMPILED_HEADER( compiled_space_writers pch.hpp )
BW_PROJECT_CATEGORY( compiled_space_writers "BigWorld Libs" )

IF( BW_CLIENT_UNIT_TESTS_ENABLED )
	ADD_SUBDIRECTORY( unit_test )
ENDIF()
