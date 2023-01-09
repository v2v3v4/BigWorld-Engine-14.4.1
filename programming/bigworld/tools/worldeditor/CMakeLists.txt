CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( worldeditor )

ADD_DEFINITIONS( 
    -DCSTDMF_IMPORT
)

INCLUDE( BWStandardProject )
INCLUDE( BWStandardMFCProject )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR} )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/worldeditor )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/lib/ual )

GET_FILENAME_COMPONENT( BW_WORLDEDITOR_RESOURCE_DIR "${BW_GAME_DIR}/tools/worldeditor/resources" REALPATH )

SET( COMMON_CODE_SRCS
    ../common/array_properties_helper.cpp
    ../common/array_properties_helper.hpp
    ../common/base_mainframe.hpp
    ../common/base_properties_helper.cpp
    ../common/base_properties_helper.hpp
    ../common/brush_thumb_provider.cpp
    ../common/brush_thumb_provider.hpp
    ../common/bwlockd_connection.cpp
    ../common/bwlockd_connection.hpp
    ../common/cdialog_property_table.cpp
    ../common/cdialog_property_table.hpp
    ../common/directory_check.hpp
    ../common/format.cpp
    ../common/format.hpp
    ../common/graphics_settings_table.cpp
    ../common/graphics_settings_table.hpp
    ../common/grid_coord.cpp
    ../common/grid_coord.hpp
    ../common/material_editor.cpp
    ../common/material_editor.hpp
    ../common/orthographic_camera.cpp
    ../common/orthographic_camera.hpp
    ../common/properties_helper.cpp
    ../common/properties_helper.hpp
    ../common/ual_entity_provider.cpp
    ../common/ual_entity_provider.hpp
)
SOURCE_GROUP( "Common\\Code" FILES ${COMMON_CODE_SRCS} )


SET( DIALOGS_GUI_CODE_SRCS
    gui/dialogs/cvs_info_dialog.cpp
    gui/dialogs/cvs_info_dialog.hpp
    gui/dialogs/edit_space_dlg.hpp
    gui/dialogs/edit_space_dlg.cpp
    gui/dialogs/expand_space_dlg.hpp
    gui/dialogs/expand_space_dlg.cpp
    gui/dialogs/labelled_multitask_progress_dlg.cpp
    gui/dialogs/labelled_multitask_progress_dlg.hpp
    gui/dialogs/labelled_progress_dlg.cpp
    gui/dialogs/labelled_progress_dlg.hpp
    gui/dialogs/low_memory_dlg.cpp
    gui/dialogs/low_memory_dlg.hpp
    gui/dialogs/new_placement_dlg.cpp
    gui/dialogs/new_placement_dlg.hpp
    gui/dialogs/new_space_dlg.cpp
    gui/dialogs/new_space_dlg.hpp
    gui/dialogs/noise_setup_dlg.cpp
    gui/dialogs/noise_setup_dlg.hpp
    gui/dialogs/placement_ctrls_dlg.cpp
    gui/dialogs/placement_ctrls_dlg.hpp
    gui/dialogs/raw_import_dlg.cpp
    gui/dialogs/raw_import_dlg.hpp
    gui/dialogs/recreate_space_dlg.cpp
    gui/dialogs/recreate_space_dlg.hpp
    gui/dialogs/resize_maps_dlg.cpp
    gui/dialogs/resize_maps_dlg.hpp
    gui/dialogs/splash_dialog.cpp
    gui/dialogs/splash_dialog.hpp
    gui/dialogs/string_input_dlg.cpp
    gui/dialogs/string_input_dlg.hpp
    gui/dialogs/take_chunk_photo_dlg.cpp
    gui/dialogs/take_chunk_photo_dlg.hpp
    gui/dialogs/undo_warn_dlg.cpp
    gui/dialogs/undo_warn_dlg.hpp
    gui/dialogs/wait_dialog.cpp
    gui/dialogs/wait_dialog.hpp
)
SOURCE_GROUP( "Dialogs\\GUI\\Code" FILES ${DIALOGS_GUI_CODE_SRCS} )

BW_SET_OPTIONAL_FILES( JOINTS_FX_BIGWORLD_SCRIPTS_SRCS
    ${BW_RESOURCE_DIR}/scripts/client/FX/Joints/DummyModel.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Joints/Entity.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Joints/HardPoint.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Joints/LightSource.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Joints/ModelRoot.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Joints/Node.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Joints/PPScreen.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Joints/__init__.py
)
SOURCE_GROUP( "Joints\\FX\\BigWorld\\Scripts" FILES ${JOINTS_FX_BIGWORLD_SCRIPTS_SRCS} )

BW_SET_OPTIONAL_FILES( RENDERTARGETS_POSTPROCESSING_BIGWORLD_SCRIPTS_SRCS
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/RenderTargets/__init__.py
)
SOURCE_GROUP( "RenderTargets\\PostProcessing\\BigWorld\\Scripts" FILES ${RENDERTARGETS_POSTPROCESSING_BIGWORLD_SCRIPTS_SRCS} )

BW_SET_OPTIONAL_FILES( EFFECTS_POSTPROCESSING_BIGWORLD_SCRIPTS_SRCS
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/Bloom.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/ColourCorrect.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/DepthOfField.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/DistortionTransfer.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/DownSample.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/EdgeDetect.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/Emboss.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/FilmGrain.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/Hatching.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/MultiBlur.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/Posterise.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/Properties.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/Rainbow.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/RegistrationTest.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/ScotopicVision.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/Sharpen.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/Shimmer.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Effects/__init__.py
)
SOURCE_GROUP( "Effects\\PostProcessing\\BigWorld\\Scripts" FILES ${EFFECTS_POSTPROCESSING_BIGWORLD_SCRIPTS_SRCS} )


SET( COLLISIONS_CODE_SRCS
    collisions/closest_obstacle_no_edit_stations.cpp
    collisions/closest_obstacle_no_edit_stations.hpp
    collisions/collision_callbacks.cpp
    collisions/collision_callbacks.hpp
    collisions/collision_terrain_only.cpp
    collisions/collision_terrain_only.hpp
)
SOURCE_GROUP( "Collisions\\Code" FILES ${COLLISIONS_CODE_SRCS} )


BW_SET_OPTIONAL_FILES( LANGUAGES_SRCS
    ${BW_RESOURCE_DIR}/helpers/languages/files_cn.xml
    ${BW_RESOURCE_DIR}/helpers/languages/files_en.xml
    ${BW_RESOURCE_DIR}/helpers/languages/worldeditor_gui_cn.xml
    ${BW_RESOURCE_DIR}/helpers/languages/worldeditor_gui_en.xml
    ${BW_RESOURCE_DIR}/helpers/languages/worldeditor_rc_cn.xml
    ${BW_RESOURCE_DIR}/helpers/languages/worldeditor_rc_en.xml
)
SOURCE_GROUP( "Languages" FILES ${LANGUAGES_SRCS} )


SET( UNDOREDO_CODE_SRCS
    undo_redo/elevation_undo.cpp
    undo_redo/elevation_undo.hpp
    undo_redo/entity_array_undo.cpp
    undo_redo/entity_array_undo.hpp
    undo_redo/entity_user_data_object_link_operation.cpp
    undo_redo/entity_user_data_object_link_operation.hpp
    undo_redo/linker_operations.cpp
    undo_redo/linker_operations.hpp
    undo_redo/merge_graphs_operation.cpp
    undo_redo/merge_graphs_operation.hpp
    undo_redo/station_entity_link_operation.cpp
    undo_redo/station_entity_link_operation.hpp
    undo_redo/station_link_operation.cpp
    undo_redo/station_link_operation.hpp
    undo_redo/terrain_height_map_undo.cpp
    undo_redo/terrain_height_map_undo.hpp
    undo_redo/terrain_hole_map_undo.cpp
    undo_redo/terrain_hole_map_undo.hpp
    undo_redo/terrain_tex_proj_undo.cpp
    undo_redo/terrain_tex_proj_undo.hpp
    undo_redo/terrain_texture_layer_undo.cpp
    undo_redo/terrain_texture_layer_undo.hpp
    undo_redo/tool_change_operation.cpp
    undo_redo/tool_change_operation.hpp
    undo_redo/user_data_object_link_operation.cpp
    undo_redo/user_data_object_link_operation.hpp
)
SOURCE_GROUP( "UndoRedo\\Code" FILES ${UNDOREDO_CODE_SRCS} )


SET( WORLD_CODE_SRCS
    world/editor_chunk_cache.cpp
    world/editor_chunk_cache.hpp
    world/editor_chunk_item_linker.cpp
    world/editor_chunk_item_linker.hpp
    world/editor_chunk_item_linker_manager.cpp
    world/editor_chunk_item_linker_manager.hpp
    world/editor_chunk_link_manager.cpp
    world/editor_chunk_link_manager.hpp
    world/editor_chunk_lock_visualizer.hpp
    world/editor_chunk_lock_visualizer.cpp
    world/editor_chunk_navmesh_cache.cpp
    world/editor_chunk_navmesh_cache.hpp
    world/editor_chunk_overlapper.cpp
    world/editor_chunk_overlapper.hpp
    world/editor_chunk_thumbnail_cache.cpp
    world/editor_chunk_thumbnail_cache.hpp
    world/editor_entity_proxy.cpp
    world/editor_entity_proxy.hpp
    world/editor_user_data_object_link_data_type.cpp
    world/entity_link_proxy.cpp
    world/entity_link_proxy.hpp
    world/entity_property_parser.cpp
    world/entity_property_parser.hpp
    world/entity_property_type_parser.cpp
    world/entity_property_type_parser.hpp
    world/item_info_db.cpp
    world/item_info_db.hpp
    world/station_node_link_proxy.cpp
    world/station_node_link_proxy.hpp
    world/user_data_object_link_proxy.cpp
    world/user_data_object_link_proxy.hpp
    world/vlo_manager.cpp
    world/vlo_manager.hpp
    world/we_chunk_saver.cpp
    world/we_chunk_saver.hpp
    world/world_editor_romp_harness.hpp
    world/world_editor_romp_harness.cpp
    world/world_manager.cpp
    world/world_manager.hpp
)
SOURCE_GROUP( "World\\Code" FILES ${WORLD_CODE_SRCS} )


SET( PAGES_GUI_CODE_SRCS
    ../common/page_messages.cpp
    ../common/page_messages.hpp
    gui/pages/chunk_watcher.cpp
    gui/pages/chunk_watcher.hpp
    gui/pages/page_chunk_texture.cpp
    gui/pages/page_chunk_texture.hpp
    gui/pages/page_chunk_watcher.cpp
    gui/pages/page_chunk_watcher.hpp
    gui/pages/page_decals.cpp
    gui/pages/page_decals.hpp
    gui/pages/page_fences.cpp
    gui/pages/page_fences.hpp
    gui/pages/page_message_impl.cpp
    gui/pages/page_message_impl.hpp
    gui/pages/page_my_panel.cpp
    gui/pages/page_my_panel.hpp
    gui/pages/page_objects.cpp
    gui/pages/page_objects.hpp
    gui/pages/page_options_environment.cpp
    gui/pages/page_options_environment.hpp
    gui/pages/page_options_general.cpp
    gui/pages/page_options_general.hpp
    gui/pages/page_options_hdr_lighting.cpp
    gui/pages/page_options_hdr_lighting.hpp
    gui/pages/page_options_histogram.cpp
    gui/pages/page_options_histogram.hpp
    gui/pages/page_options_navigation.cpp
    gui/pages/page_options_navigation.hpp
    gui/pages/page_options_weather.cpp
    gui/pages/page_options_weather.hpp
    gui/pages/page_post_processing.cpp
    gui/pages/page_post_processing.hpp
    gui/pages/page_project.cpp
    gui/pages/page_project.hpp
    gui/pages/page_properties.cpp
    gui/pages/page_properties.hpp
    gui/pages/page_terrain_base.cpp
    gui/pages/page_terrain_base.hpp
    gui/pages/page_terrain_filter.cpp
    gui/pages/page_terrain_filter.hpp
    gui/pages/page_terrain_height.cpp
    gui/pages/page_terrain_height.hpp
    gui/pages/page_terrain_import.cpp
    gui/pages/page_terrain_import.hpp
    gui/pages/page_terrain_mesh.cpp
    gui/pages/page_terrain_mesh.hpp
    gui/pages/page_terrain_texture.cpp
    gui/pages/page_terrain_texture.hpp
    gui/pages/page_flora_setting_tree.cpp
    gui/pages/page_flora_setting_tree.hpp
    gui/pages/page_flora_setting_secondary_wnd.cpp
    gui/pages/page_flora_setting_secondary_wnd.hpp
    gui/pages/page_flora_setting.cpp
    gui/pages/page_flora_setting.hpp
    gui/pages/page_flora_setting.ipp
    gui/pages/panel_manager.cpp
    gui/pages/panel_manager.hpp
    gui/pages/post_proc_caption_bar.cpp
    gui/pages/post_proc_caption_bar.hpp
    gui/pages/post_processing_chains.cpp
    gui/pages/post_processing_chains.hpp
    gui/pages/post_processing_properties.cpp
    gui/pages/post_processing_properties.hpp
)
SOURCE_GROUP( "Pages\\GUI\\Code" FILES ${PAGES_GUI_CODE_SRCS} )

SET( GRAPH_CODE_SRCS
    graph/edge.cpp
    graph/edge.hpp
    graph/graph.cpp
    graph/graph.hpp
    graph/graph_item_views.cpp
    graph/graph_item_views.hpp
    graph/graph_view.cpp
    graph/graph_view.hpp
    graph/node.cpp
    graph/node.hpp
)
SOURCE_GROUP( "Graph\\Code" FILES ${GRAPH_CODE_SRCS} )


SET( HEIGHT_CODE_SRCS
    height/height_map.cpp
    height/height_map.hpp
    height/height_module.cpp
    height/height_module.hpp
)
SOURCE_GROUP( "Height\\Code" FILES ${HEIGHT_CODE_SRCS} )

BW_SET_OPTIONAL_FILES( POSTPROCESSING_BIGWORLD_SCRIPTS_SRCS
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/ChainView.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/D3DEnums.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/FilterKernels.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/Phases.py
    ${BW_RESOURCE_DIR}/scripts/client/PostProcessing/__init__.py
)
SOURCE_GROUP( "PostProcessing\\BigWorld\\Scripts" FILES ${POSTPROCESSING_BIGWORLD_SCRIPTS_SRCS} )


SET( PROJECT_CODE_SRCS
    project/chunk_photographer.cpp
    project/chunk_photographer.hpp
    project/chunk_walker.cpp
    project/chunk_walker.hpp
    project/lock_map.cpp
    project/lock_map.hpp
    project/forced_lod_map.cpp
    project/forced_lod_map.hpp
    project/nearby_chunk_loader.cpp
    project/nearby_chunk_loader.hpp
    project/project_module.cpp
    project/project_module.hpp
    project/space_helpers.hpp
    project/space_information.hpp
    project/space_map.cpp
    project/space_map.hpp
    project/space_map_debug.cpp
    project/space_map_debug.hpp
    project/space_map_timestamp_cache.cpp
    project/space_map_timestamp_cache.hpp
)
SOURCE_GROUP( "Project\\Code" FILES ${PROJECT_CODE_SRCS} )


SET( CONTROLS_GUI_CODE_SRCS
    gui/controls/chunk_watch_control.cpp
    gui/controls/chunk_watch_control.hpp
    gui/controls/data_section_proxy.hpp
    gui/controls/directory_browser.cpp
    gui/controls/directory_browser.hpp
    gui/controls/file_image_list.cpp
    gui/controls/file_image_list.hpp
    gui/controls/limit_slider.cpp
    gui/controls/limit_slider.hpp
    gui/controls/statistics_control.cpp
    gui/controls/statistics_control.hpp
    gui/controls/terrain_textures_control.cpp
    gui/controls/terrain_textures_control.hpp
    gui/controls/terrain_texture_lod_control.hpp
    gui/controls/terrain_texture_lod_control.cpp
    gui/controls/weather_settings_table.cpp
    gui/controls/weather_settings_table.hpp
)
SOURCE_GROUP( "Controls\\GUI\\Code" FILES ${CONTROLS_GUI_CODE_SRCS} )


BW_SET_OPTIONAL_FILES( BIGWORLD_SCRIPTS_SRCS
    ${BW_RESOURCE_DIR}/scripts/client/Bloom.py
    ${BW_RESOURCE_DIR}/scripts/client/FilterUtilities.py
    ${BW_RESOURCE_DIR}/scripts/client/GraphicsPresets.py
    ${BW_RESOURCE_DIR}/scripts/client/Keys.py
    ${BW_RESOURCE_DIR}/scripts/client/Listener.py
    ${BW_RESOURCE_DIR}/scripts/client/Weather.py
)
SOURCE_GROUP( "BigWorld\\Scripts" FILES ${BIGWORLD_SCRIPTS_SRCS} )

BW_SET_OPTIONAL_FILES( SCRIPTS_SRCS
    ${BW_WORLDEDITOR_RESOURCE_DIR}/scripts/Actions.py
    ${BW_WORLDEDITOR_RESOURCE_DIR}/scripts/Ecotypes.py
    ${BW_WORLDEDITOR_RESOURCE_DIR}/scripts/ItemUIAdapter.py
    ${BW_WORLDEDITOR_RESOURCE_DIR}/scripts/Personality.py
    ${BW_WORLDEDITOR_RESOURCE_DIR}/scripts/PostProcessingUIAdapter.py
    ${BW_WORLDEDITOR_RESOURCE_DIR}/scripts/PyGUI.py
    ${BW_WORLDEDITOR_RESOURCE_DIR}/scripts/TerrainUIAdapter.py
    ${BW_WORLDEDITOR_RESOURCE_DIR}/scripts/ToolbarUIAdapter.py
    ${BW_WORLDEDITOR_RESOURCE_DIR}/scripts/UIAdapter.py
    ${BW_WORLDEDITOR_RESOURCE_DIR}/scripts/UIExt.py
    ${BW_WORLDEDITOR_RESOURCE_DIR}/scripts/WeatherUIAdapter.py
    ${BW_WORLDEDITOR_RESOURCE_DIR}/scripts/WorldEditorDirector.py
)
SOURCE_GROUP( "Scripts" FILES ${SCRIPTS_SRCS} )


BW_SET_OPTIONAL_FILES( ACTORS_FX_BIGWORLD_SCRIPTS_SRCS
    ${BW_RESOURCE_DIR}/scripts/client/FX/Actors/Light.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Actors/Model.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Actors/PPChain.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Actors/ParticleSystem.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Actors/__init__.py
)
SOURCE_GROUP( "Actors\\FX\\BigWorld\\Scripts" FILES ${ACTORS_FX_BIGWORLD_SCRIPTS_SRCS} )


SET( SCENEBROWSER_GUI_CODE_SRCS
    gui/scene_browser/checkbox_helper.cpp
    gui/scene_browser/checkbox_helper.hpp
    gui/scene_browser/column_menu_item.cpp
    gui/scene_browser/column_menu_item.hpp
    gui/scene_browser/group_item.cpp
    gui/scene_browser/group_item.hpp
    gui/scene_browser/list_column.cpp
    gui/scene_browser/list_column.hpp
    gui/scene_browser/list_column_states.cpp
    gui/scene_browser/list_column_states.hpp
    gui/scene_browser/list_group_states.cpp
    gui/scene_browser/list_group_states.hpp
    gui/scene_browser/list_search_filters.cpp
    gui/scene_browser/list_search_filters.hpp
    gui/scene_browser/list_selection.cpp
    gui/scene_browser/list_selection.hpp
    gui/scene_browser/scene_browser.cpp
    gui/scene_browser/scene_browser.hpp
    gui/scene_browser/scene_browser_dlg.cpp
    gui/scene_browser/scene_browser_dlg.hpp
    gui/scene_browser/scene_browser_list.cpp
    gui/scene_browser/scene_browser_list.hpp
    gui/scene_browser/scene_browser_utils.cpp
    gui/scene_browser/scene_browser_utils.hpp
    gui/scene_browser/setup_items_task.cpp
    gui/scene_browser/setup_items_task.hpp
    gui/scene_browser/single_property_editor.cpp
    gui/scene_browser/single_property_editor.hpp
)
SOURCE_GROUP( "SceneBrowser\\GUI\\Code" FILES ${SCENEBROWSER_GUI_CODE_SRCS} )


SET( TERRAIN_CODE_SRCS
    terrain/editor_chunk_terrain.cpp
    terrain/editor_chunk_terrain.hpp
    terrain/editor_chunk_terrain_projector.cpp
    terrain/editor_chunk_terrain_projector.hpp
    terrain/forced_lod_overlay.cpp
    terrain/forced_lod_overlay.hpp
    terrain/mask_overlay.cpp
    terrain/mask_overlay.hpp
    terrain/max_layers_overlay.cpp
    terrain/max_layers_overlay.hpp
    terrain/mouse_drag_handler.cpp
    terrain/mouse_drag_handler.hpp
    terrain/space_height_map.cpp
    terrain/space_height_map.hpp
    terrain/space_recreator.hpp
    terrain/space_recreator.cpp
    terrain/terrain_chunk_texture_tool_view.cpp
    terrain/terrain_chunk_texture_tool_view.hpp
    terrain/terrain_functor.cpp
    terrain/terrain_functor.hpp
    terrain/terrain_height_filter_functor.cpp
    terrain/terrain_height_filter_functor.hpp
    terrain/terrain_hole_functor.cpp
    terrain/terrain_hole_functor.hpp
    terrain/terrain_locator.cpp
    terrain/terrain_locator.hpp
    terrain/terrain_paint_brush.cpp
    terrain/terrain_paint_brush.hpp
    terrain/terrain_set_height_functor.cpp
    terrain/terrain_set_height_functor.hpp
    terrain/terrain_texture_functor.cpp
    terrain/terrain_texture_functor.hpp
    terrain/terrain_texture_tool_view.cpp
    terrain/terrain_texture_tool_view.hpp
    terrain/terrain_texture_utils.cpp
    terrain/terrain_texture_utils.hpp
    terrain/texture_mask_cache.cpp
    terrain/texture_mask_cache.hpp
)
SOURCE_GROUP( "Terrain\\Code" FILES ${TERRAIN_CODE_SRCS} )


SET( EDITOR_CODE_SRCS
    editor/autosnap.cpp
    editor/autosnap.hpp
    editor/chunk_editor.cpp
    editor/chunk_editor.hpp
    editor/chunk_item_placer.cpp
    editor/chunk_item_placer.hpp
    editor/chunk_obstacle_locator.cpp
    editor/chunk_obstacle_locator.hpp
    editor/chunk_placer.cpp
    editor/chunk_placer.hpp
    editor/chunk_placer_prefab.cpp
    editor/editor_property_manager.hpp
    editor/gen_link_view.cpp
    editor/gen_link_view.hpp
    editor/item_editor.cpp
    editor/item_editor.hpp
    editor/item_frustum_locator.cpp
    editor/item_frustum_locator.hpp
    editor/item_locator.cpp
    editor/item_locator.hpp
    editor/item_properties.cpp
    editor/item_properties.hpp
    editor/link_functor.cpp
    editor/link_functor.hpp
    editor/link_gizmo.cpp
    editor/link_gizmo.hpp
    editor/link_property.cpp
    editor/link_property.hpp
    editor/link_property_view.cpp
    editor/link_property_view.hpp
    editor/link_proxy.cpp
    editor/link_proxy.hpp
    editor/link_view.cpp
    editor/link_view.hpp
    editor/pipe_functor.cpp
    editor/pipe_functor.hpp
    editor/snaps.cpp
    editor/snaps.hpp
    editor/station_node_link_locator.cpp
    editor/station_node_link_locator.hpp
    editor/tee_functor.cpp
    editor/tee_functor.hpp
    editor/texture_tool_view.cpp
    editor/texture_tool_view.hpp
    editor/user_data_object_link_locator.cpp
    editor/user_data_object_link_locator.hpp
)
SOURCE_GROUP( "Editor\\Code" FILES ${EDITOR_CODE_SRCS} )


SET( RULER_CODE_SRCS
    gui/pages/page_terrain_ruler.cpp
    gui/pages/page_terrain_ruler.hpp
    terrain/terrain_ruler_functor.cpp
    terrain/terrain_ruler_functor.hpp
    terrain/terrain_ruler_tool_view.cpp
    terrain/terrain_ruler_tool_view.hpp
)
SOURCE_GROUP( "Ruler\\Code" FILES ${RULER_CODE_SRCS} )


SET( POSTPROCESSING_GUI_CODE_SRCS
    gui/post_processing/base_post_processing_node.cpp
    gui/post_processing/base_post_processing_node.hpp
    gui/post_processing/chain_editors.cpp
    gui/post_processing/chain_editors.hpp
    gui/post_processing/effect_edge.cpp
    gui/post_processing/effect_edge.hpp
    gui/post_processing/effect_edge_view.cpp
    gui/post_processing/effect_edge_view.hpp
    gui/post_processing/effect_node.cpp
    gui/post_processing/effect_node.hpp
    gui/post_processing/effect_node_view.cpp
    gui/post_processing/effect_node_view.hpp
    gui/post_processing/node_callback.hpp
    gui/post_processing/node_resource_holder.cpp
    gui/post_processing/node_resource_holder.hpp
    gui/post_processing/phase_edge.cpp
    gui/post_processing/phase_edge.hpp
    gui/post_processing/phase_edge_view.cpp
    gui/post_processing/phase_edge_view.hpp
    gui/post_processing/phase_node.cpp
    gui/post_processing/phase_node.hpp
    gui/post_processing/phase_node_view.cpp
    gui/post_processing/phase_node_view.hpp
    gui/post_processing/popup_drag_target.cpp
    gui/post_processing/popup_drag_target.hpp
    gui/post_processing/post_proc_property_editor.cpp
    gui/post_processing/post_proc_property_editor.hpp
    gui/post_processing/post_proc_undo.cpp
    gui/post_processing/post_proc_undo.hpp
    gui/post_processing/pp_preview.cpp
    gui/post_processing/pp_preview.hpp
    gui/post_processing/ual_effect_provider.cpp
    gui/post_processing/ual_effect_provider.hpp
    gui/post_processing/ual_phase_provider.cpp
    gui/post_processing/ual_phase_provider.hpp
    gui/post_processing/ual_render_target_provider.cpp
    gui/post_processing/ual_render_target_provider.hpp
    gui/post_processing/view_draw_utils.cpp
    gui/post_processing/view_draw_utils.hpp
    gui/post_processing/view_skin.cpp
    gui/post_processing/view_skin.hpp
)
SOURCE_GROUP( "PostProcessing\\GUI\\Code" FILES ${POSTPROCESSING_GUI_CODE_SRCS} )


BW_SET_OPTIONAL_FILES( FX_BIGWORLD_SCRIPTS_SRCS
    ${BW_RESOURCE_DIR}/scripts/client/FX/Actor.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Event.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Joint.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/__init__.py
)
SOURCE_GROUP( "FX\\BigWorld\\Scripts" FILES ${FX_BIGWORLD_SCRIPTS_SRCS} )


SET( IMPORT_CODE_SRCS
    import/elevation_blit.cpp
    import/elevation_blit.hpp
    import/import_codec.cpp
    import/import_codec.hpp
    import/import_codec_bmp.cpp
    import/import_codec_bmp.hpp
    import/import_codec_dted.cpp
    import/import_codec_dted.hpp
    import/import_codec_raw.cpp
    import/import_codec_raw.hpp
    import/import_codec_terragen.cpp
    import/import_codec_terragen.hpp
    import/import_codec_tiff.cpp
    import/import_codec_tiff.hpp
    import/import_image.cpp
    import/import_image.hpp
    import/space_settings.cpp
    import/space_settings.hpp
    import/terrain_utils.cpp
    import/terrain_utils.hpp
    import/texture_mask_blit.cpp
    import/texture_mask_blit.hpp
)
SOURCE_GROUP( "Import\\Code" FILES ${IMPORT_CODE_SRCS} )


BW_SET_OPTIONAL_FILES( EFFECTS_FX_BIGWORLD_SCRIPTS_SRCS
    ${BW_RESOURCE_DIR}/scripts/client/FX/Effects/Buffered.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Effects/OneShot.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Effects/Persistent.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Effects/_Effect.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Effects/_EventTimer.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Effects/__init__.py
)
SOURCE_GROUP( "Effects\\FX\\BigWorld\\Scripts" FILES ${EFFECTS_FX_BIGWORLD_SCRIPTS_SRCS} )


SET( ALL_SRCS
    res/highlands.bmp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( CODE_SRCS
    config.hpp
    forward.hpp
    pch.cpp
    pch.hpp
    resource.h
)
SOURCE_GROUP( "Code" FILES ${CODE_SRCS} )


SET( ITEMS_WORLD_CODE_SRCS
    world/items/chunk_link_segment.cpp
    world/items/chunk_link_segment.hpp
    world/items/editor_chunk_binding.cpp
    world/items/editor_chunk_binding.hpp
    world/items/editor_chunk_entity.cpp
    world/items/editor_chunk_entity.hpp
    world/items/editor_chunk_flare.cpp
    world/items/editor_chunk_flare.hpp
    world/items/editor_chunk_item_tree_node.hpp
    world/items/editor_chunk_light.cpp
    world/items/editor_chunk_light.hpp
    world/items/editor_chunk_link.cpp
    world/items/editor_chunk_link.hpp
    world/items/editor_chunk_marker.cpp
    world/items/editor_chunk_marker.hpp
    world/items/editor_chunk_marker_cluster.cpp
    world/items/editor_chunk_marker_cluster.hpp
    world/items/editor_chunk_meta_data.cpp
    world/items/editor_chunk_meta_data.hpp
    world/items/editor_chunk_model.cpp
    world/items/editor_chunk_model.hpp
    world/items/editor_chunk_particle_system.cpp
    world/items/editor_chunk_particle_system.hpp
    world/items/editor_chunk_point_link.cpp
    world/items/editor_chunk_point_link.hpp
    world/items/editor_chunk_portal.cpp
    world/items/editor_chunk_portal.hpp
    world/items/editor_chunk_model_vlo.cpp
    world/items/editor_chunk_model_vlo.hpp
    world/items/very_large_object_behaviour.cpp
    world/items/very_large_object_behaviour.hpp
    world/items/editor_chunk_station.cpp
    world/items/editor_chunk_station.hpp
    world/items/editor_chunk_substance.hpp
    world/items/editor_chunk_substance.ipp
    world/items/editor_chunk_tree.cpp
    world/items/editor_chunk_tree.hpp
    world/items/editor_chunk_user_data_object.cpp
    world/items/editor_chunk_user_data_object.hpp
    world/items/editor_chunk_user_data_object_link.cpp
    world/items/editor_chunk_user_data_object_link.hpp
    world/items/editor_chunk_vlo.cpp
    world/items/editor_chunk_vlo.hpp
    world/items/editor_chunk_water.cpp
    world/items/editor_chunk_water.hpp
    world/items/editor_chunk_model_vlo_ref.hpp
    world/items/editor_chunk_model_vlo_ref.cpp
    world/items/vlo_existence_operation.hpp
    world/items/vlo_existence_operation.cpp
    world/items/editor_chunk_deferred_decal.cpp
    world/items/editor_chunk_deferred_decal.hpp
)
SOURCE_GROUP( "Items\\World\\Code" FILES ${ITEMS_WORLD_CODE_SRCS} )


BW_SET_OPTIONAL_FILES( EVENTS_FX_BIGWORLD_SCRIPTS_SRCS
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/AOEVictimNodeList.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/AddDecal.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/AlignModel.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/ClearParticles.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/CorrectMotionTriggeredParticles.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/Fade.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/Flicker.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/FlickeringLight.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/ForceParticle.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/PPAnimateProperty.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/PPTranslationProperty.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/ParticleSubSystem.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/PlayAction.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/PlaySound.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/RampTimeTriggeredParticles.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/RandomDelay.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/ResetTimeTriggeredParticles.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/SetBasis.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/SetColour.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/SetOrbitorPoint.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/Shockwave.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/SwarmTargets.py
    ${BW_RESOURCE_DIR}/scripts/client/FX/Events/__init__.py
)
SOURCE_GROUP( "Events\\FX\\BigWorld\\Scripts" FILES ${EVENTS_FX_BIGWORLD_SCRIPTS_SRCS} )


SET( SCRIPTING_CODE_SRCS
    scripting/we_python_adapter.cpp
    scripting/we_python_adapter.hpp
    scripting/world_editor_script.cpp
    scripting/world_editor_script.hpp
)
SOURCE_GROUP( "Scripting\\Code" FILES ${SCRIPTING_CODE_SRCS} )


BW_SET_OPTIONAL_FILES( GUI_DATA_SRCS
    ${BW_WORLDEDITOR_RESOURCE_DIR}/data/filters.xml
    ${BW_WORLDEDITOR_RESOURCE_DIR}/data/gui.xml
    ${BW_WORLDEDITOR_RESOURCE_DIR}/data/modules.xml
    ${BW_WORLDEDITOR_RESOURCE_DIR}/data/options_page.xml
    ${BW_WORLDEDITOR_RESOURCE_DIR}/data/placement.xml
)
SOURCE_GROUP( "GUI_Data" FILES ${GUI_DATA_SRCS} )


SET( MISC_CODE_SRCS
    misc/chunk_row_cache.cpp
    misc/chunk_row_cache.hpp
    misc/cvswrapper.cpp
    misc/cvswrapper.hpp
    misc/editor_renderable.hpp
    misc/editor_tickable.hpp
    misc/fences_functor.cpp
    misc/fences_functor.hpp
    misc/fences_tool_view.cpp
    misc/fences_tool_view.hpp
    misc/light_container_debugger.cpp
    misc/light_container_debugger.hpp
    misc/matrix_filter.cpp
    misc/matrix_filter.hpp
    misc/navmesh_view.cpp
    misc/navmesh_view.hpp
    misc/options_helper.cpp
    misc/options_helper.hpp
    misc/placement_presets.cpp
    misc/placement_presets.hpp
    misc/popup_menu_helper.cpp
    misc/popup_menu_helper.hpp
    misc/progress_bar_helper.cpp
    misc/progress_bar_helper.hpp
    misc/selection_filter.cpp
    misc/selection_filter.hpp
    misc/sync_mode.cpp
    misc/sync_mode.hpp
    misc/world_editor_camera.cpp
    misc/world_editor_camera.hpp
)
SOURCE_GROUP( "Misc\\Code" FILES ${MISC_CODE_SRCS} )


SET( RESOURCE_FILES_SRCS
    res/aboutbox.bmp
    res/check_off.bmp
    res/check_on.bmp
    res/chunk_texture_hide.ico
    res/chunk_texture_notrack.ico
    res/chunk_texture_show.ico
    res/chunk_texture_track.ico
    res/close.bmp
    res/duplicate.bmp
    res/duplicated.bmp
    res/editproj.bmp
    res/editprojd.bmp
    res/effect_node_btn_off.bmp
    res/effect_node_btn_on.bmp
    res/height_picker.cur
    res/impdistoolbar.bmp
    res/imptoolbar.bmp
    res/magnifier.bmp
    res/magnifier_click.bmp
    res/mask_tb.bmp
    res/mask_tb_dis.bmp
    res/newfile.bmp
    res/newfiled.bmp
    res/node_delete.bmp
    res/open.bmp
    res/opend.bmp
    res/page_terrain_linkdown.ico
    res/page_terrain_linkup.ico
    res/placement_link.ico
    res/placement_linkdown.ico
    res/placement_linkup.ico
    res/placement_reset.ico
    res/plus.bmp
    res/pp_3dpreview.ico
    res/pp_3dpreview_checked.ico
    res/pp_delete_all.ico
    res/pp_nozoom.ico
    res/pp_profile.ico
    res/pp_profile_checked.ico
    res/pp_zoom_in.ico
    res/pp_zoom_out.ico
    res/pplayout.ico
    res/savebrush.bmp
    res/savebrushd.bmp
    res/sceneicons.bmp
    res/skyboxd.bmp
    res/skyboxtb.bmp
    res/skydomed.bmp
    res/skydometb.bmp
    res/splashscreen.bmp
    res/working.avi
    res/worldeditor.ico
    res/worldeditor.rc2
    res/worldeditordoc.ico
    world_editor_app.rc
)
SOURCE_GROUP( "Resource_Files" FILES ${RESOURCE_FILES_SRCS} )


SET( FRAMEWORK_CODE_SRCS
    framework/initialisation.cpp
    framework/initialisation.hpp
    framework/mainframe.cpp
    framework/mainframe.hpp
    framework/world_editor_app.cpp
    framework/world_editor_app.hpp
    framework/world_editor_doc.cpp
    framework/world_editor_doc.hpp
    framework/world_editor_view.cpp
    framework/world_editor_view.hpp
)
SOURCE_GROUP( "Framework\\Code" FILES ${FRAMEWORK_CODE_SRCS} )




BW_BLOB_SOURCES( BLOB_SRCS
    ${COMMON_CODE_SRCS}
    ${DIALOGS_GUI_CODE_SRCS}
    ${JOINTS_FX_BIGWORLD_SCRIPTS_SRCS}
    ${RENDERTARGETS_POSTPROCESSING_BIGWORLD_SCRIPTS_SRCS}
    ${EFFECTS_POSTPROCESSING_BIGWORLD_SCRIPTS_SRCS}
    ${COLLISIONS_CODE_SRCS}
    ${LANGUAGES_SRCS}
    ${UNDOREDO_CODE_SRCS}
    ${WORLD_CODE_SRCS}
    ${PAGES_GUI_CODE_SRCS}
    ${GRAPH_CODE_SRCS}
    ${HEIGHT_CODE_SRCS}
    ${POSTPROCESSING_BIGWORLD_SCRIPTS_SRCS}
    ${PROJECT_CODE_SRCS}
    ${CONTROLS_GUI_CODE_SRCS}
    ${BIGWORLD_SCRIPTS_SRCS}
    ${SCRIPTS_SRCS}
    ${ACTORS_FX_BIGWORLD_SCRIPTS_SRCS}
    ${SCENEBROWSER_GUI_CODE_SRCS}
    ${TERRAIN_CODE_SRCS}
    ${EDITOR_CODE_SRCS}
    ${RULER_CODE_SRCS}
    ${POSTPROCESSING_GUI_CODE_SRCS}
    ${FX_BIGWORLD_SCRIPTS_SRCS}
    ${IMPORT_CODE_SRCS}
    ${EFFECTS_FX_BIGWORLD_SCRIPTS_SRCS}
    ${ALL_SRCS}
    ${CODE_SRCS}
    ${ITEMS_WORLD_CODE_SRCS}
    ${EVENTS_FX_BIGWORLD_SCRIPTS_SRCS}
    ${SCRIPTING_CODE_SRCS}
    ${GUI_DATA_SRCS}
    ${MISC_CODE_SRCS}
    ${RESOURCE_FILES_SRCS}
    ${FRAMEWORK_CODE_SRCS}
    )

BW_ADD_TOOL_EXE( worldeditor worldeditor WIN32 ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( worldeditor
    appmgr
    chunk
    chunk_scene_adapter
    controls
    cstdmf
    editor_shared
    gizmo
    guimanager
    math
    moo
    particle
    post_processing
    pyscript
    resmgr
    romp
    space
    terrain
    tools_common
    ual
    # third party
    libpython_tools
    )

IF( BW_FMOD_SUPPORT )
    BW_TARGET_LINK_LIBRARIES( worldeditor
        fmodsound
    )
ENDIF()

BW_PRECOMPILED_HEADER( worldeditor pch.hpp )

