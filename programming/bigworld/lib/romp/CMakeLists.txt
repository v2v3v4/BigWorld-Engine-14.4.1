CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( romp )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
    chunk_omni_light_embodiment.hpp
    chunk_omni_light_embodiment.cpp
    chunk_spot_light_embodiment.hpp
    chunk_spot_light_embodiment.cpp
    effect_parameter_cache.cpp
    effect_parameter_cache.hpp
    environment_cube_map.cpp
    environment_cube_map.hpp
    light_map.cpp
    light_map.hpp
    occlusion_query_helper.cpp
    occlusion_query_helper.hpp
    pch.cpp
    pch.hpp
    py_light_common.hpp
    py_light_common.cpp
    py_omni_light.hpp
    py_omni_light.cpp
    py_spot_light.hpp
    py_spot_light.cpp
    py_d3d_enums.cpp
    py_d3d_enums.hpp
    py_graphics_setting.cpp
    py_graphics_setting.hpp
    py_material.cpp
    py_material.hpp
    py_material.ipp
    py_render_target.cpp
    py_render_target.hpp
    py_render_target.ipp
    py_resource_refs.cpp
    py_resource_refs.hpp
    py_texture_provider.cpp
    py_texture_provider.hpp
    resource_ref.cpp
    resource_ref.hpp
    romp_collider.cpp
    romp_collider.hpp
    romp_lib.hpp
    texture_feeds.cpp
    texture_feeds.hpp
    watermark.cpp
    watermark.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( FLORA_SRCS
    animation_grid.cpp
    animation_grid.hpp
    ecotype.cpp
    ecotype.hpp
    ecotype.ipp
    ecotype_generators.cpp
    ecotype_generators.hpp
    flora.cpp
    flora.hpp
    flora.ipp
    flora_block.cpp
    flora_block.hpp
    flora_constants.hpp
    flora_renderer.cpp
    flora_renderer.hpp
    flora_texture.cpp
    flora_texture.hpp
    flora_vertex_type.hpp
)
SOURCE_GROUP( "Flora" FILES ${FLORA_SRCS} )


SET( CONSOLE_SRCS
    console.cpp
    console.hpp
    console.ipp
    console_manager.cpp
    console_manager.hpp
    console_manager.ipp
    engine_statistics.cpp
    engine_statistics.hpp
    engine_statistics.ipp
    frame_logger.cpp
    frame_logger.hpp
    lineeditor.cpp
    lineeditor.hpp
    lineeditor.ipp
    log_console.cpp
    log_console.hpp
    profile_graph.cpp
    profile_graph.hpp
    resource_manager_stats.cpp
    resource_manager_stats.hpp
    resource_statistics.cpp
    resource_statistics.hpp
    xconsole.cpp
    xconsole.hpp
    xconsole.ipp
)
SOURCE_GROUP( "Console" FILES ${CONSOLE_SRCS} )


SET( SKY_SRCS
    fog_controller.cpp
    fog_controller.hpp
    fog_controller.ipp
    sky_dome_occluder.cpp
    sky_dome_occluder.hpp
    sky_dome_shadows.cpp
    sky_dome_shadows.hpp
    sky_gradient_dome.cpp
    sky_gradient_dome.hpp
    sky_gradient_dome.ipp
    sky_light_map.cpp
    sky_light_map.hpp
    sun_and_moon.cpp
    sun_and_moon.hpp
    sun_and_moon.ipp
    weather.cpp
    weather.hpp
)
SOURCE_GROUP( "Sky" FILES ${SKY_SRCS} )


SET( LENS_EFFECTS_SRCS
    handle_pool.cpp
    handle_pool.hpp
    lens_effect.cpp
    lens_effect.hpp
    lens_effect.ipp
    lens_effect_manager.cpp
    lens_effect_manager.hpp
    lens_effect_manager.ipp
    photon_occluder.cpp
    photon_occluder.hpp
    photon_occluder.ipp
    terrain_occluder.cpp
    terrain_occluder.hpp
    terrain_occluder.ipp
    z_attenuation_occluder.cpp
    z_attenuation_occluder.hpp
    z_buffer_occluder.cpp
    z_buffer_occluder.hpp
    z_buffer_occluder.ipp
)
SOURCE_GROUP( "Lens_Effects" FILES ${LENS_EFFECTS_SRCS} )


SET( WATER_SRCS
    py_water_volume.cpp
    py_water_volume.hpp
    sea.cpp
    sea.hpp
    water.cpp
    water.hpp
    water.ipp
    water_scene_renderer.cpp
    water_scene_renderer.hpp
    water_simulation.cpp
    water_simulation.hpp
    water_simulation.ipp
)
SOURCE_GROUP( "Water" FILES ${WATER_SRCS} )


SET( TIME_OF_DAY_SRCS
    time_of_day.cpp
    time_of_day.hpp
    time_of_day.ipp
)
SOURCE_GROUP( "Time_Of_Day" FILES ${TIME_OF_DAY_SRCS} )


SET( FILTERS_SRCS
    distortion.cpp
    distortion.hpp
    histogram_provider.cpp
    histogram_provider.hpp
)
SOURCE_GROUP( "Filters" FILES ${FILTERS_SRCS} )


SET( PROGRESS_SRCS
    progress.cpp
    progress.hpp
)
SOURCE_GROUP( "Progress" FILES ${PROGRESS_SRCS} )


SET( ENVIRO_MINDER_SRCS
    enviro_minder.cpp
    enviro_minder.hpp
    enviro_minder.ipp
)
SOURCE_GROUP( "Enviro_Minder" FILES ${ENVIRO_MINDER_SRCS} )


SET( FONT_SRCS
    font.cpp
    font.hpp
    font.ipp
    font_manager.cpp
    font_manager.hpp
    font_metrics.cpp
    font_metrics.hpp
    glyph_cache.cpp
    glyph_cache.hpp
    glyph_reference_holder.cpp
    glyph_reference_holder.hpp
    labels.hpp
    py_font.cpp
    py_font.hpp
    py_font.ipp
)
SOURCE_GROUP( "Font" FILES ${FONT_SRCS} )


SET( EXTRAS_SRCS
    hdr_settings.cpp
    hdr_settings.hpp
    hdr_support.cpp
    hdr_support.hpp
    ssao_settings.cpp
    ssao_settings.hpp
    ssao_support.cpp
    ssao_support.hpp
    sun_visibility_constant.cpp
    sun_visibility_constant.hpp
)
SOURCE_GROUP( "Extras" FILES ${EXTRAS_SRCS} )


BW_BLOB_SOURCES( BLOB_SRCS
    ${ALL_SRCS}
    ${FLORA_SRCS}
    ${CONSOLE_SRCS}
    ${SKY_SRCS}
    ${LENS_EFFECTS_SRCS}
    ${WATER_SRCS}
    ${TIME_OF_DAY_SRCS}
    ${SUPERMODEL_SRCS}
    ${FILTERS_SRCS}
    ${PROGRESS_SRCS}
    ${ENVIRO_MINDER_SRCS}
    ${FONT_SRCS}
    ${EXTRAS_SRCS}
)

BW_ADD_LIBRARY( romp
    ${BLOB_SRCS}
    )

BW_TARGET_LINK_LIBRARIES( romp INTERFACE
    chunk
    cstdmf
    input
    math
    moo
    resmgr
    particle
    pyscript
    scene
    space
    terrain
    )

IF( BW_IS_TOOLS )
    BW_TARGET_LINK_LIBRARIES( romp INTERFACE appmgr )
ENDIF()

BW_PRECOMPILED_HEADER( romp pch.hpp )
BW_PROJECT_CATEGORY( romp "BigWorld Libs" )
