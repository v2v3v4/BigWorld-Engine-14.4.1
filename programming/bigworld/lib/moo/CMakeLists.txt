CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( moo )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

INCLUDE_DIRECTORIES( . )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/nvtt/src )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/lib/anttweakbar/include )

IF( NOT BW_IS_SERVER )
    SET( TEXTURES_SRCS
        animating_texture.cpp
        animating_texture.hpp
        animating_texture.ipp
        base_texture.cpp
        base_texture.hpp
        base_texture.ipp
        convert_texture_tool.hpp
        convert_texture_tool.cpp
        cube_render_target.cpp
        cube_render_target.hpp
        cube_render_target.ipp
        dds.h
        format_name_map.hpp
        format_name_map.cpp
        image_texture.hpp
        image_texture.ipp
        managed_texture.cpp
        managed_texture.hpp
        managed_texture.ipp
        streaming_texture.cpp
        streaming_texture.hpp
        streaming_texture.ipp
        render_target.cpp
        render_target.hpp
        render_target.ipp
        sys_mem_texture.cpp
        sys_mem_texture.hpp
        texture_aggregator.cpp
        texture_aggregator.hpp
        texture_compressor.cpp
        texture_compressor.hpp
        texture_compressor.ipp
        texture_detail_level.hpp
        texture_detail_level.cpp
        texture_detail_level_manager.hpp
        texture_detail_level_manager.cpp
        texture_exposer.cpp
        texture_exposer.hpp
        texture_helpers.cpp
        texture_helpers.hpp
        texture_loader.cpp
        texture_loader.hpp
        texture_lock_wrapper.cpp
        texture_lock_wrapper.hpp
        texture_manager.cpp
        texture_manager.hpp
        texture_reuse_cache.cpp
        texture_reuse_cache.hpp
        texture_streaming_manager.cpp
        texture_streaming_manager.hpp
        texture_streaming_scene_view.cpp
        texture_streaming_scene_view.hpp
        streaming_manager.cpp
        streaming_manager.hpp
        texture_usage.cpp
        texture_usage.hpp
    )
    SOURCE_GROUP( "Textures" FILES ${TEXTURES_SRCS} )

    SET( GEOMETRICS_SRCS
        custom_mesh.cpp
        custom_mesh.hpp
        custom_mesh.ipp
        debug_geometry.cpp
        debug_geometry.hpp
        geometrics.cpp
        geometrics.hpp
        geometrics.ipp
        line_helper.cpp
        line_helper.hpp
        texture_renderer.cpp
        texture_renderer.hpp
    )
    SOURCE_GROUP( "Geometrics" FILES ${GEOMETRICS_SRCS} )

    SET( DEBUGDRAW_SRCS
        debug_draw.cpp
        debug_draw.hpp
    )
    SOURCE_GROUP( "Debug Draw" FILES ${DEBUGDRAW_SRCS} )
ENDIF()


SET( ALL_SRCS
    pch.cpp
    pch.hpp
    png.cpp
    png.hpp
)
IF( NOT BW_IS_SERVER )
    LIST( APPEND ALL_SRCS
        fog_helper.cpp
        fog_helper.hpp
        forward_declarations.hpp
        graphics_settings_picker.cpp
        graphics_settings_picker.hpp
        lod_settings.cpp
        lod_settings.hpp
        image.hpp
        image.ipp
        init.cpp
        init.hpp
        mrt_support.cpp
        mrt_support.hpp
        node_catalogue.cpp
        node_catalogue.hpp
        reload.cpp
        reload.hpp
        software_skinner.cpp
        software_skinner.hpp
        vertex_declaration.cpp
        vertex_declaration.hpp
    )
ENDIF()
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


IF( NOT BW_IS_SERVER )
    SET( LIGHTS_SRCS
        directional_light.cpp
        directional_light.hpp
        directional_light.ipp
        light_container.cpp
        light_container.hpp
        light_container.ipp
        omni_light.cpp
        omni_light.hpp
        omni_light.ipp
        pulse_light.cpp
        pulse_light.hpp
        spot_light.cpp
        spot_light.hpp
        spot_light.ipp
    )
    SOURCE_GROUP( "Lights" FILES ${LIGHTS_SRCS} )


    SET( MATERIAL_SRCS
        legacy_material_defs.hpp
        material.cpp
        material.hpp
        material.ipp
        material_loader.hpp
        shader_set.cpp
        shader_set.hpp
        shader_set.ipp
        texturestage.cpp
        texturestage.hpp
        texturestage.ipp
    )
    SOURCE_GROUP( "Material" FILES ${MATERIAL_SRCS} )


    SET( EFFECT_FRAMEWORK_SRCS
        effect_compiler.cpp
        effect_compiler.hpp
        effect_constant_value.hpp
        effect_helpers.cpp
        effect_helpers.hpp
        effect_includes.cpp
        effect_includes.hpp
        effect_macro_setting.cpp
        effect_macro_setting.hpp
        effect_manager.cpp
        effect_manager.hpp
        effect_material.cpp
        effect_material.hpp
        effect_material.ipp
        effect_property.cpp
        effect_property.hpp
        effect_state_manager.cpp
        effect_state_manager.hpp
        effect_visual_context.cpp
        effect_visual_context.hpp
        managed_effect.cpp
        managed_effect.hpp
        managed_effect.ipp
    )
    SOURCE_GROUP( "Effect_Framework" FILES ${EFFECT_FRAMEWORK_SRCS} )


    SET( FRAMEWORK_SRCS
        camera.cpp
        camera.hpp
        camera.ipp
        com_object_wrap.hpp
        com_object_wrap.ipp
        device_callback.cpp
        device_callback.hpp
        device_callback.ipp
        draw_context.cpp
        draw_context.hpp
        gpu_info.cpp
        gpu_info.hpp
        gpu_profiler.cpp
        gpu_profiler.hpp
        gpu_profiler.ipp
        graphics_settings.cpp
        graphics_settings.hpp
        moo_dx.cpp
        moo_dx.hpp
        moo_math.hpp
        render_context.cpp
        render_context.hpp
        render_context.ipp
        render_context_callback.cpp
        render_context_callback.hpp
        render_context_debug.cpp
        render_context_debug.hpp
        resource_load_context.cpp
        resource_load_context.hpp
        vertex_element.hpp
        vertex_element.cpp
        vertex_element_special_cases.hpp
        vertex_element_special_cases.cpp
        vertex_element_value.hpp
        vertex_element_value.cpp
        vertex_format.cpp
        vertex_format.hpp
        vertex_format.ipp
        vertex_format_cache.hpp
        vertex_format_cache.ipp
        vertex_format_cache.cpp
        vertex_format_conversions.hpp
        vertex_format_conversions.ipp
        vertex_format_conversions.cpp
        vertex_formats.hpp
    )
    SOURCE_GROUP( "Framework" FILES ${FRAMEWORK_SRCS} )
ENDIF()


SET( VISUAL_SRCS
    node.cpp
    node.hpp
    node.ipp
)
IF( NOT BW_IS_SERVER )
    LIST( APPEND VISUAL_SRCS
        visual.cpp
        visual.hpp
        visual.ipp
        visual_common.hpp
        visual_manager.cpp
        visual_manager.hpp
        visual_manager.ipp
    )
ENDIF()
SOURCE_GROUP( "Visual" FILES ${VISUAL_SRCS} )


IF( NOT BW_IS_SERVER )
    SET( ANIMATION_SRCS
        animation.cpp
        animation.hpp
        animation.ipp
        animation_channel.cpp
        animation_channel.hpp
        animation_channel.ipp
        animation_manager.cpp
        animation_manager.hpp
        animation_manager.ipp
        cue_channel.cpp
        cue_channel.hpp
        discrete_animation_channel.cpp
        discrete_animation_channel.hpp
        discrete_animation_channel.ipp
        interpolated_animation_channel.cpp
        interpolated_animation_channel.hpp
        interpolated_animation_channel.ipp
        streamed_animation_channel.cpp
        streamed_animation_channel.hpp
        streamed_animation_channel.ipp
        streamed_data_cache.cpp
        streamed_data_cache.hpp
        translation_override_channel.cpp
        translation_override_channel.hpp
    )
    SOURCE_GROUP( "Animation" FILES ${ANIMATION_SRCS} )


    SET( HELPERS_SRCS
        camera_planes_setter.cpp
        camera_planes_setter.hpp
        moo_test_window.cpp
        moo_test_window.hpp
        scissors_setter.cpp
        scissors_setter.hpp
        viewport_setter.cpp
        viewport_setter.hpp
    )
    SOURCE_GROUP( "Helpers" FILES ${HELPERS_SRCS} )


    SET( PRIMITIVES_SRCS
        dynamic_index_buffer.cpp
        dynamic_index_buffer.hpp
        dynamic_vertex_buffer.cpp
        dynamic_vertex_buffer.hpp
        dynamic_vertex_buffer.ipp
        index_buffer.hpp
        index_buffer_wrapper.hpp
        primitive.cpp
        primitive.hpp
        primitive.ipp
        primitive_helper.cpp
        primitive_helper.hpp
        primitive_file_structs.hpp
        primitive_manager.cpp
        primitive_manager.hpp
        primitive_manager.ipp
        vertex_buffer.hpp
        vertex_buffer_wrapper.hpp
        vertex_streams.cpp
        vertex_streams.hpp
        vertices.cpp
        vertices.hpp
        vertices.ipp
        vertices_manager.cpp
        vertices_manager.hpp
        vertices_manager.ipp
    )
    SOURCE_GROUP( "Primitives" FILES ${PRIMITIVES_SRCS} )


    SET( DS_SHADOW_DYNAMIC_SRCS
        dynamic_shadow.cpp
        dynamic_shadow.hpp
    )
    SOURCE_GROUP( "DS\\dynamic\\shadow" FILES ${DS_SHADOW_DYNAMIC_SRCS} )


    SET( DS_SHADOW_DYNAMIC_SHADOWS_LIB
        shadows_lib/bounding.cpp
        shadows_lib/bounding.h
        shadows_lib/config.hpp
        shadows_lib/shadow_math.cpp
        shadows_lib/shadow_math.hpp
        shadows_lib/shadows.cpp
        shadows_lib/shadows.hpp
    )
    SOURCE_GROUP( "DS\\dynamic\\shadow\\shadows_lib" FILES ${DS_SHADOW_DYNAMIC_SHADOWS_LIB} )


    SET( DS_SHADOW_SRCS
        shadow_manager.cpp
        shadow_manager.hpp
    )
    SOURCE_GROUP( "DS\\shadow" FILES ${DS_SHADOW_SRCS} )


    SET( DS_DECALS_SRCS
        bsp_tree_2d.hpp
        deferred_decals_manager.cpp
        deferred_decals_manager.hpp
        forward_decal.cpp
        forward_decal.hpp
        texture_atlas.cpp
        texture_atlas.hpp
        dynamic_decal.cpp
        dynamic_decal.hpp
    )
    SOURCE_GROUP( "DS\\decals" FILES ${DS_DECALS_SRCS} )


    SET( DS_CORE_SRCS
        deferred_pipeline.cpp
        deferred_pipeline.hpp
        forward_pipeline.cpp
        forward_pipeline.hpp
        renderer.cpp
        renderer.hpp
    )
    SOURCE_GROUP( "DS\\core" FILES ${DS_CORE_SRCS} )


    SET( DS_LIGHTS_SRCS
        deferred_lights_manager.cpp
        deferred_lights_manager.hpp
    )
    SOURCE_GROUP( "DS\\lights" FILES ${DS_LIGHTS_SRCS} )


    SET( DS_SHADOW_SEMIDYNAMIC_SRCS
        semi_dynamic_shadow_scene_view.cpp
        semi_dynamic_shadow_scene_view.hpp
        semi_dynamic_shadow.cpp
        semi_dynamic_shadow.hpp
    )
    SOURCE_GROUP( "DS\\shadow\\semidynamic" FILES ${DS_SHADOW_SEMIDYNAMIC_SRCS} )


    SET( EXTRAS_SRCS
        complex_effect_material.cpp
        complex_effect_material.hpp
        custom_AA.cpp
        custom_AA.hpp
        fullscreen_quad.cpp
        fullscreen_quad.hpp
        moo_utilities.cpp
        moo_utilities.hpp
        nvapi_wrapper.cpp
        nvapi_wrapper.hpp
        ppaa_support.cpp
        ppaa_support.hpp
        render_event.hpp
        taa_support.cpp
        taa_support.hpp
        utils.cpp
        utils.hpp
        geometry.hpp
        sticker_decal.cpp
        sticker_decal.hpp
        material_kinds_constants.hpp
        decal_utils.ipp
    )
    SOURCE_GROUP( "Extras" FILES ${EXTRAS_SRCS} )
ENDIF()


BW_BLOB_SOURCES( BLOB_SRCS
    ${TEXTURES_SRCS}
    ${GEOMETRICS_SRCS}
    ${DEBUGDRAW_SRCS}
    ${ALL_SRCS}
    ${LIGHTS_SRCS}
    ${MATERIAL_SRCS}
    ${EFFECT_FRAMEWORK_SRCS}
    ${FRAMEWORK_SRCS}
    ${VISUAL_SRCS}
    ${ANIMATION_SRCS}
    ${HELPERS_SRCS}
    ${PRIMITIVES_SRCS}
    ${EXTRAS_SRCS}
    ${DS_SHADOW_DYNAMIC_SRCS}
    ${DS_SHADOW_DYNAMIC_SHADOWS_LIB}
    ${DS_SHADOW_SRCS}
    ${DS_DECALS_SRCS}
    ${DS_CORE_SRCS}
    ${DS_LIGHTS_SRCS}
    ${DS_SHADOW_SEMIDYNAMIC_SRCS}
)

BW_ADD_LIBRARY( moo
    ${BLOB_SRCS}
    )

BW_TARGET_LINK_LIBRARIES( moo INTERFACE
    cstdmf
    math
    physics2
    scene
    # third party
    libpng
)

IF( NOT BW_IS_SERVER )
    BW_TARGET_LINK_LIBRARIES( moo INTERFACE
        romp
        space
    )
ENDIF()

IF( BW_PLATFORM_WINDOWS )
    BW_TARGET_LINK_LIBRARIES( moo INTERFACE
        # external libraries
        d3d9
        d3dx9
        dxguid
        setupapi
        psapi
    )
ENDIF()
    
# dirty hack to remove speedtree from exporter builds as _ITERATOR_DEBUG_LEVEL needs to be 
# specified differently for max plugins and this library is prebuilt    
IF( BW_SPEEDTREE_SUPPORT AND NOT BW_IS_EXPORTERS )
    BW_TARGET_LINK_LIBRARIES( moo INTERFACE speedtree )
ENDIF()

IF( BW_IS_TOOLS OR BW_IS_CLIENT OR BW_IS_NAVGEN )
    BW_TARGET_LINK_LIBRARIES( moo INTERFACE asset_pipeline )
ENDIF()

BW_PRECOMPILED_HEADER( moo pch.hpp )
BW_PROJECT_CATEGORY( moo "BigWorld Libs" )
