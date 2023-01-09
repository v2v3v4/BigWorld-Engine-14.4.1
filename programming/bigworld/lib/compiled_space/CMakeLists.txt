CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( compiled_space )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	pch.hpp
	pch.cpp
	compiled_space.cpp
	compiled_space.hpp
	compiled_space_dll.hpp
	compiled_space_lib.hpp
	compiled_space_settings.cpp
	compiled_space_settings.hpp
	compiled_space_settings_types.hpp
	compiled_space_factory.cpp
	compiled_space_factory.hpp
	compiled_space_mapping.cpp
	compiled_space_mapping.hpp
	forward_declarations.hpp
	loader.hpp
	loader.cpp
	binary_format.cpp
	binary_format.hpp
	string_table.cpp
	string_table.hpp
	string_table_types.hpp
	asset_list.cpp
	asset_list.hpp
	asset_list_types.hpp
	static_geometry.hpp
	static_geometry.cpp
	static_geometry_types.hpp
	entity_list.cpp
	entity_list.hpp
	entity_list_types.hpp

	light_scene_provider.cpp
	light_scene_provider.hpp
	light_scene_types.cpp
	light_scene_types.hpp

	static_scene_provider.cpp
	static_scene_provider.hpp
	static_scene_types.hpp
	static_scene_type_handler.hpp
	static_scene_model.cpp
	static_scene_model.hpp
	static_scene_model_types.hpp
	static_scene_speed_tree.cpp
	static_scene_speed_tree.hpp
	static_scene_speed_tree_types.hpp
	static_scene_water.cpp
	static_scene_water.hpp
	static_scene_water_types.hpp
	static_scene_flare.cpp
	static_scene_flare.hpp
	static_scene_flare_types.hpp
	static_scene_decal.cpp
	static_scene_decal.hpp
	static_scene_decal_types.hpp
	
	static_texture_streaming_types.hpp
	static_texture_streaming_scene_provider.hpp
	static_texture_streaming_scene_provider.cpp
	
	terrain2_scene_provider.cpp
	terrain2_scene_provider.hpp
	terrain2_types.hpp
	
	particle_system_loader.cpp
	particle_system_loader.hpp
	particle_system_types.hpp
	
	py_attachment_entity_embodiment.cpp
	py_attachment_entity_embodiment.hpp
	py_model_obstacle_entity_embodiment.cpp
	py_model_obstacle_entity_embodiment.hpp
	
	collision_helpers.cpp
	collision_helpers.hpp
	
	cached_semi_dynamic_shadow_scene_provider.hpp
	cached_semi_dynamic_shadow_scene_provider.cpp
	
	omni_light_embodiment.hpp
	omni_light_embodiment.cpp
	spot_light_embodiment.hpp
	spot_light_embodiment.cpp
	
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( compiled_space ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( compiled_space INTERFACE
	cstdmf
	duplo
	math
	moo
	resmgr
	scene
	space
	terrain
	)

IF( BW_IS_CLIENT OR BW_IS_TOOLS OR BW_IS_NAVGEN )
	BW_TARGET_LINK_LIBRARIES( compiled_space INTERFACE	asset_pipeline )
ENDIF()

BW_PRECOMPILED_HEADER( compiled_space pch.hpp )
BW_PROJECT_CATEGORY( compiled_space "BigWorld Libs" )

