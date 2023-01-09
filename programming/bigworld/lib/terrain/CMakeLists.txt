CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( terrain )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )


SET( ALL_SRCS
	base_terrain_block.cpp
	base_terrain_block.hpp
	base_terrain_block.ipp
	dominant_texture_map.cpp
	dominant_texture_map.hpp
	height_map_compress.cpp
	height_map_compress.hpp
	manager.cpp
	manager.hpp
	pch.cpp
	pch.hpp
	terrain_collision_callback.cpp
	terrain_collision_callback.hpp
	terrain_ray_collision_callback.cpp
	terrain_ray_collision_callback.hpp
	terrain_prism_collision_callback.cpp
	terrain_prism_collision_callback.hpp
	terrain_data.hpp
	terrain_finder.cpp
	terrain_finder.hpp
	terrain_height_map.cpp
	terrain_height_map.hpp
	terrain_settings.cpp
	terrain_settings.hpp
)
IF( NOT BW_IS_SERVER )
	LIST( APPEND ALL_SRCS
		base_render_terrain_block.hpp
		base_terrain_renderer.cpp
		base_terrain_renderer.hpp
		horizon_shadow_map.hpp
		terrain_graphics_options.cpp
		terrain_graphics_options.hpp
		terrain_map.hpp
		terrain_map_holder.hpp
		terrain_map_iter.hpp
	
	
		terrain_texture_layer.cpp
		terrain_texture_layer.hpp
		terrain_scene_view.cpp
		terrain_scene_view.hpp
	)
ELSEIF( BW_IS_SERVER )
	LIST( APPEND ALL_SRCS
		terrain_block_cache.cpp
		terrain_block_cache.hpp
	)
ENDIF()

SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( TERRAIN2_SRCS
	terrain2/common_terrain_block2.cpp
	terrain2/common_terrain_block2.hpp
	terrain2/dominant_texture_map2.cpp
	terrain2/dominant_texture_map2.hpp
	terrain2/terrain_block2.cpp
	terrain2/terrain_block2.hpp
	terrain2/terrain_height_map2.cpp
	terrain2/terrain_height_map2.hpp
	terrain2/terrain_hole_map2.cpp
	terrain2/terrain_hole_map2.hpp
	terrain2/terrain_quad_tree_cell.cpp
	terrain2/terrain_quad_tree_cell.hpp
	terrain2/terrain_texture_layer2.cpp
	terrain2/terrain_texture_layer2.hpp
)
IF( NOT BW_IS_SERVER )
	LIST( APPEND TERRAIN2_SRCS
		terrain2/aliased_height_map.cpp
		terrain2/aliased_height_map.hpp
		terrain2/grid_vertex_buffer.cpp
		terrain2/grid_vertex_buffer.hpp
		terrain2/horizon_shadow_map2.cpp
		terrain2/horizon_shadow_map2.hpp
		terrain2/pch.hpp
		terrain2/resource.hpp
		terrain2/terrain_blends.cpp
		terrain2/terrain_blends.hpp
		terrain2/terrain_index_buffer.cpp
		terrain2/terrain_index_buffer.hpp
		terrain2/terrain_lod_controller.cpp
		terrain2/terrain_lod_controller.hpp
		terrain2/terrain_lod_map2.cpp
		terrain2/terrain_lod_map2.hpp
		terrain2/terrain_normal_map2.cpp
		terrain2/terrain_normal_map2.hpp
		terrain2/terrain_photographer.cpp
		terrain2/terrain_photographer.hpp
		terrain2/terrain_renderer2.cpp
		terrain2/terrain_renderer2.hpp
		terrain2/terrain_vertex_buffer.cpp
		terrain2/terrain_vertex_buffer.hpp
		terrain2/terrain_vertex_lod.cpp
		terrain2/terrain_vertex_lod.hpp
		terrain2/terrain_vertex_lod_consts.hpp
		terrain2/ttl2cache.cpp
		terrain2/ttl2cache.hpp
		terrain2/vertex_lod_entry.cpp
		terrain2/vertex_lod_entry.hpp
		terrain2/vertex_lod_manager.cpp
		terrain2/vertex_lod_manager.hpp
		terrain2/terrain_ao_map2.cpp
		terrain2/terrain_ao_map2.hpp
	)
ENDIF()
SOURCE_GROUP( "terrain2" FILES ${TERRAIN2_SRCS} )


IF( BW_IS_TOOLS )
	SET( EDITOR_SRCS
		editor_base_terrain_block.cpp
		editor_base_terrain_block.hpp
	)
	SOURCE_GROUP( "" FILES ${ALL_SRCS} )
	
	
	SET( EDITOR_TERRAIN2_SRCS
		terrain2/editor_terrain_block2.cpp
		terrain2/editor_terrain_block2.hpp
		terrain2/editor_vertex_lod_manager.cpp
		terrain2/editor_vertex_lod_manager.hpp
	)
	SOURCE_GROUP( "terrain2" FILES ${TERRAIN2_SRCS} )
ENDIF()


BW_BLOB_SOURCES( BLOB_SRCS
	${ALL_SRCS}
	${TERRAIN2_SRCS}
	${EDITOR_SRCS}
	${EDITOR_TERRAIN2_SRCS}
)
BW_ADD_LIBRARY( terrain ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( terrain INTERFACE
	cstdmf
	physics2
	resmgr
	)

IF( NOT BW_IS_SERVER )
	BW_TARGET_LINK_LIBRARIES( terrain INTERFACE
		moo
		)
ENDIF()

BW_PRECOMPILED_HEADER( terrain pch.hpp )
BW_PROJECT_CATEGORY( terrain "BigWorld Libs" )

