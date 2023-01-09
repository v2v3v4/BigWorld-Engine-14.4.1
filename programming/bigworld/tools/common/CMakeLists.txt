CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( tools_common )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardMFCProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	base_camera.cpp
	base_camera.hpp
	base_camera.ipp
	base_panel_manager.cpp
	base_panel_manager.hpp
	base_property_table.cpp
	base_property_table.hpp
	bw_message_info.cpp
	bw_message_info.hpp
	chop_poly.cpp
	chop_poly.hpp
	chunk_flooder.cpp
	chunk_flooder.hpp
	collision_advance.cpp
	collision_advance.hpp
	command_line.hpp
	compile_time.cpp
	compile_time.hpp
	cooperative_moo.cpp
	cooperative_moo.hpp
	delay_redraw.cpp
	delay_redraw.hpp
	dxenum.cpp
	dxenum.hpp
	editor_chunk_cache_base.cpp
	editor_chunk_cache_base.hpp
	editor_chunk_entity_base.cpp
	editor_chunk_entity_base.hpp
	editor_chunk_model_base.cpp
	editor_chunk_model_base.hpp
	editor_chunk_navmesh_cache_base.cpp
	editor_chunk_navmesh_cache_base.hpp
	editor_chunk_processor_cache.cpp
	editor_chunk_processor_cache.hpp
	editor_chunk_terrain_base.cpp
	editor_chunk_terrain_base.hpp
	editor_chunk_terrain_cache.cpp
	editor_chunk_terrain_cache.hpp
	editor_chunk_terrain_lod_cache.cpp
	editor_chunk_terrain_lod_cache.hpp
	editor_group.cpp
	editor_group.hpp
	editor_views.cpp
	editor_views.hpp
	floor.cpp
	floor.hpp
	girth.cpp
	girth.hpp
	lighting_influence.cpp
	lighting_influence.hpp
	material_properties.cpp
	material_properties.hpp
	material_proxies.cpp
	material_proxies.hpp
	material_utility.cpp
	material_utility.hpp
	mouse_look_camera.cpp
	mouse_look_camera.hpp
	navmesh_processor.cpp
	navmesh_processor.hpp
	orbit_camera.cpp
	orbit_camera.hpp
	pch.cpp
	pch.hpp
	physics_handler.cpp
	physics_handler.hpp
	popup_menu.cpp
	popup_menu.hpp
	property_list.cpp
	property_list.hpp
	property_table.cpp
	property_table.hpp
	python_adapter.cpp
	python_adapter.hpp
	recast_processor.cpp
	recast_processor.hpp
	resource_loader.cpp
	resource_loader.hpp
	romp_harness.cpp
	romp_harness.hpp
	shader_loading_dialog.cpp
	shader_loading_dialog.hpp
	space_mgr.cpp
	space_mgr.hpp
	string_utils.cpp
	string_utils.hpp
	terrain_shadow_processor.cpp
	terrain_shadow_processor.hpp
	tools_camera.cpp
	tools_camera.hpp
	tools_common.cpp
	tools_common.hpp
	user_messages.hpp
	utilities.cpp
	utilities.hpp
	waypoint_annotator.cpp
	waypoint_annotator.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${ALL_SRCS}
)
BW_ADD_LIBRARY( tools_common ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( tools_common INTERFACE
	cstdmf
	editor_shared
	navigation_recast
	waypoint_generator
	)

BW_PRECOMPILED_HEADER( tools_common pch.hpp )
BW_PROJECT_CATEGORY( tools_common "BigWorld Libs" )
