CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( chunk )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

IF( BW_IS_TOOLS )
    INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools )
ENDIF()

SET( ALL_SRCS
    chunk_grid_size.cpp
    chunk_grid_size.hpp
    pch.cpp
    pch.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( INHABITANTS_SRCS
    base_chunk_tree.cpp
    base_chunk_tree.hpp
    chunk_exit_portal.cpp
    chunk_exit_portal.hpp
    chunk_item.cpp
    chunk_item.hpp
    chunk_model_obstacle.cpp
    chunk_model_obstacle.hpp
    chunk_model_vlo.cpp
    chunk_model_vlo.hpp
    chunk_stationnode.cpp
    chunk_stationnode.hpp
    chunk_terrain_obstacle.cpp
    chunk_terrain_obstacle.hpp
    chunk_user_data_object.cpp
    chunk_user_data_object.hpp
    chunk_vlo_obstacle.cpp
    chunk_vlo_obstacle.hpp
    portal_chunk_item.cpp
    portal_chunk_item.hpp
    station_graph.cpp
    station_graph.hpp
    user_data_object.cpp
    user_data_object.ipp
    user_data_object.hpp
    user_data_object_link_data_type.cpp
    user_data_object_link_data_type.hpp
    user_data_object_type.cpp
    user_data_object_type.ipp
    user_data_object_type.hpp
)
IF( BW_IS_SERVER )
    LIST( APPEND INHABITANTS_SRCS
        server_chunk_model.cpp
        server_chunk_model.hpp
        server_chunk_terrain.cpp
        server_chunk_terrain.hpp
        server_chunk_terrain.ipp
        server_chunk_tree.cpp
        server_chunk_tree.hpp
        server_chunk_water.cpp
        server_chunk_water.hpp
        server_super_model.cpp
        server_super_model.hpp
    )
ELSE()
    LIST( APPEND INHABITANTS_SRCS
        chunk_binding.cpp
        chunk_binding.hpp
        chunk_flare.cpp
        chunk_flare.hpp
        chunk_flora.cpp
        chunk_flora.hpp
        chunk_flora.ipp
        chunk_item_tree_node.cpp
        chunk_item_tree_node.hpp
        chunk_light.cpp
        chunk_light.hpp
        chunk_marker.cpp
        chunk_marker.hpp
        chunk_marker_cluster.cpp
        chunk_marker_cluster.hpp
        chunk_model.cpp
        chunk_model.ipp
        chunk_model.hpp
        chunk_terrain.cpp
        chunk_terrain.hpp
        chunk_terrain.ipp
        chunk_terrain_common.hpp
        chunk_tree.cpp
        chunk_tree.hpp
        chunk_water.cpp
        chunk_water.ipp
        chunk_water.hpp
        collision_scene_provider.cpp
        collision_scene_provider.hpp
    )
ENDIF()
SOURCE_GROUP( "Inhabitants" FILES ${INHABITANTS_SRCS} )


IF( NOT BW_IS_SERVER )
    SET( UMBRA_SRCS
        chunk_umbra.cpp
        chunk_umbra.hpp
        umbra_chunk_item.cpp
        umbra_chunk_item.hpp
        umbra_config.hpp
        umbra_draw_item.cpp
        umbra_draw_item.hpp
        umbra_proxies.cpp
        umbra_proxies.hpp
    )
    SOURCE_GROUP( "Umbra" FILES ${UMBRA_SRCS} )
ENDIF()


SET( FRAMEWORK_SRCS
    base_chunk_space.cpp
    base_chunk_space.hpp
    chunk.cpp
    chunk.hpp
    chunk_boundary.cpp
    chunk_boundary.hpp
    chunk_cache.cpp
    chunk_cache.hpp
    chunk_cache.ipp
    chunk_link.cpp
    chunk_link.hpp
    chunk_loader.cpp
    chunk_loader.hpp
    chunk_obstacle.cpp
    chunk_obstacle.hpp
    chunk_obstacle.ipp
    chunk_overlapper.cpp
    chunk_overlapper.hpp
    chunk_space.cpp
    chunk_space.hpp
    chunk_vlo.cpp
    chunk_vlo.hpp
    geometry_mapping.cpp
    geometry_mapping.hpp
    chunk_model_vlo_ref.hpp
    chunk_model_vlo_ref.cpp
)
IF( BW_IS_SERVER )
    LIST( APPEND FRAMEWORK_SRCS
        server_chunk_manager.cpp
        server_chunk_space.cpp
        server_chunk_space.hpp
    )
ELSE()
    LIST( APPEND FRAMEWORK_SRCS
        chunk_bsp_holder.cpp
        chunk_bsp_holder.hpp
        chunk_clean_flags.cpp
        chunk_clean_flags.hpp
        chunk_format.hpp
        chunk_item_amortise_delete.cpp
        chunk_item_amortise_delete.hpp
        chunk_manager.cpp
        chunk_manager.hpp
        chunk_quad_tree.cpp
        chunk_quad_tree.hpp
        client_chunk_space.cpp
        client_chunk_space.hpp
        grid_traversal.hpp
        invalidate_flags.hpp
        chunk_deferred_decal.cpp
        chunk_deferred_decal.hpp
    )
ENDIF()
SOURCE_GROUP( "Framework" FILES ${FRAMEWORK_SRCS} )

IF( BW_IS_TOOLS )
    SET( EDITOR_SRCS
        editor_chunk_common.cpp
        editor_chunk_common.hpp
        editor_chunk_item.cpp
        editor_chunk_item.hpp
        editor_chunk_item.ipp
    )
    SOURCE_GROUP( "Editor" FILES ${EDITOR_SRCS} )
    
    
    SET( PROCESSOR_SRCS
        chunk_processor.cpp
        chunk_processor.hpp
        chunk_processor_manager.cpp
        chunk_processor_manager.hpp
        clean_chunk_list.cpp
        clean_chunk_list.hpp
        dirty_chunk_list.cpp
        dirty_chunk_list.hpp
        locked_chunks.hpp
        scoped_locked_chunk_holder.cpp
        scoped_locked_chunk_holder.hpp
        scoped_locked_chunk_holder.ipp
        unsaved_chunks.cpp
        unsaved_chunks.hpp
        unsaved_terrain_blocks.cpp
        unsaved_terrain_blocks.hpp
    )
    SOURCE_GROUP( "Processor" FILES ${PROCESSOR_SRCS} )
ENDIF()


BW_BLOB_SOURCES( BLOB_SRCS
    ${ALL_SRCS}
    ${INHABITANTS_SRCS}
    ${UMBRA_SRCS}
    ${FRAMEWORK_SRCS}
    ${EDITOR_SRCS}
    ${PROCESSOR_SRCS}
    )
    
BW_ADD_LIBRARY( chunk
    ${BLOB_SRCS}
    )

BW_TARGET_LINK_LIBRARIES( chunk INTERFACE
    cstdmf
    entitydef
    math
    moo
    physics2
    resmgr
    scene
    script
    )

IF( NOT BW_IS_SERVER )
    BW_TARGET_LINK_LIBRARIES( chunk INTERFACE
        model
        romp
        space
    )
ENDIF()

IF( BW_IS_TOOLS )
    BW_TARGET_LINK_LIBRARIES( chunk INTERFACE
        tools_common
    )
ENDIF()
    
IF( BW_IS_TOOLS )
    BW_TARGET_LINK_LIBRARIES( chunk INTERFACE
        tools_common
    )
ENDIF()
    
IF( BW_FMOD_SUPPORT )
    BW_TARGET_LINK_LIBRARIES( chunk INTERFACE fmodsound )
    TARGET_INCLUDE_DIRECTORIES( chunk PRIVATE ${FMOD_INCLUDE_DIRS} )
ENDIF()

IF( BW_SPEEDTREE_SUPPORT )
    BW_TARGET_LINK_LIBRARIES( chunk INTERFACE speedtree )
ENDIF()

IF( BW_UMBRA_SUPPORT )
    TARGET_INCLUDE_DIRECTORIES( chunk PRIVATE
        "${BW_SOURCE_DIR}/third_party/umbra/interface" )
ENDIF()

BW_PRECOMPILED_HEADER( chunk pch.hpp )
BW_PROJECT_CATEGORY( chunk "BigWorld Libs" )

