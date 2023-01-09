CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( modeleditor_core )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardMFCProject )
INCLUDE( BWStandardLibrary )


INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR} )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/modeleditor_core )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/modeleditor_core/App )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/modeleditor_core/GUI )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/modeleditor_core/Pages )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/modeleditor_core/Models )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/nvmeshmender )


SET( ALL_SRCS
	i_model_editor_app.hpp
	i_model_editor_app.cpp
	resource.h
	pch.cpp
	pch.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( MODELS_SRCS
	Models/lights.cpp
	Models/lights.hpp
	Models/mutant.cpp
	Models/mutant.hpp
	Models/mutant_actions.cpp
	Models/mutant_animations.cpp
	Models/mutant_lod.cpp
	Models/mutant_materials.cpp
	Models/mutant_render.cpp
	Models/mutant_validation.cpp
	Models/visual_bumper.cpp
	Models/visual_bumper.hpp
	Models/visual_bumper.ipp
)
SOURCE_GROUP( "Models" FILES ${MODELS_SRCS} )


SET( APP_SRCS
	App/grid_coord.cpp
	App/grid_coord.hpp
	App/material_preview.cpp
	App/material_preview.hpp
	App/me_app.cpp
	App/me_app.hpp
	App/me_consts.hpp
	App/me_error_macros.hpp
	App/me_light_proxies.hpp
	App/me_material_proxies.hpp
	App/me_module.cpp
	App/me_module.hpp
	App/me_python_adapter.cpp
	App/me_python_adapter.hpp
	App/me_scripter.cpp
	App/me_scripter.hpp
	App/me_shell.cpp
	App/me_shell.hpp
	App/mru.cpp
	App/mru.hpp
	App/undo_redo.cpp
	App/undo_redo.hpp
)
SOURCE_GROUP( "App" FILES ${APP_SRCS} )


SET( PAGES_SRCS
	../common/page_messages.cpp
	../common/page_messages.hpp
	Pages/page_actions.cpp
	Pages/page_actions.hpp
	Pages/page_animations.cpp
	Pages/page_animations.hpp
	Pages/page_animations_comp.cpp
	Pages/page_animations_impl.hpp
	Pages/page_display.cpp
	Pages/page_display.hpp
	Pages/page_lights.cpp
	Pages/page_lights.hpp
	Pages/page_lod.cpp
	Pages/page_lod.hpp
	Pages/page_materials.cpp
	Pages/page_materials.hpp
	Pages/page_object.cpp
	Pages/page_object.hpp
)
SOURCE_GROUP( "Pages" FILES ${PAGES_SRCS} )

BW_SET_OPTIONAL_FILES( LANGUAGES_SRCS
	${BW_RESOURCE_DIR}/helpers/languages/files_en.xml
	${BW_RESOURCE_DIR}/helpers/languages/modeleditor_gui_en.xml
	${BW_RESOURCE_DIR}/helpers/languages/modeleditor_rc_en.xml
)
SOURCE_GROUP( "Languages" FILES ${LANGUAGES_SRCS} )

SET( RESOURCE_FILES_SRCS
	res/about_box.bmp
	res/add_cursor.cur
	res/cancel.bmp
	res/lod_pat_1.bmp
	res/lod_pat_2.bmp
	res/lod_pat_3.bmp
	res/minus.bmp
	res/modeleditor.ico
	res/modeleditor.rc2
	res/modeleditor_doc.ico
	res/plus.bmp
	res/search.bmp
	res/splash_screen.bmp
)
SOURCE_GROUP( "Resource_Files" FILES ${RESOURCE_FILES_SRCS} )


SET( GUI_SRCS
	GUI/about_box.cpp
	GUI/about_box.hpp
	GUI/choose_anim.cpp
	GUI/choose_anim.hpp
	GUI/foldersetter.hpp
	GUI/loading_dialog.cpp
	GUI/loading_dialog.hpp
	GUI/lod_bar.cpp
	GUI/lod_bar.hpp
	GUI/gl_view.cpp
	GUI/gl_view.hpp
	GUI/main_frm.cpp
	GUI/main_frm.h
	GUI/model_editor_doc.cpp
	GUI/model_editor_doc.h
	GUI/model_editor_view.cpp
	GUI/model_editor_view.h
	GUI/new_tint.cpp
	GUI/new_tint.hpp
	GUI/panel_manager.cpp
	GUI/panel_manager.hpp
	GUI/prefs_dialog.cpp
	GUI/prefs_dialog.hpp
	GUI/splash_dialog.cpp
	GUI/splash_dialog.hpp
	GUI/texture_feed.cpp
	GUI/texture_feed.hpp
	GUI/tree_list.cpp
	GUI/tree_list.hpp
	GUI/tree_list_dlg.cpp
	GUI/tree_list_dlg.hpp
	GUI/trigger_list.cpp
	GUI/trigger_list.hpp
)
SOURCE_GROUP( "GUI" FILES ${GUI_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${ALL_SRCS}
	${RESOURCE_FILES_SRCS}
	${GUI_SRCS}
	${APP_SRCS}
	${MODELS_SRCS}
	${PAGES_SRCS}
	${SCRIPTS_SRCS}
	${LANGUAGES_SRCS}
	)

BW_ADD_LIBRARY( modeleditor_core
	${BLOB_SRCS}
	)

BW_TARGET_LINK_LIBRARIES( modeleditor_core
	INTERFACE
	appmgr
	cstdmf
	chunk_scene_adapter
	editor_shared
	guimanager
	input
	moo
	physics2
	post_processing
	pyscript
	resmgr
	romp
	terrain
	tools_common
	# third party
	nvmeshmender
	)

IF( BW_FMOD_SUPPORT )
	BW_TARGET_LINK_LIBRARIES( modeleditor_core INTERFACE fmodsound )
	TARGET_INCLUDE_DIRECTORIES( modeleditor_core PRIVATE ${FMOD_INCLUDE_DIRS} )
ENDIF()

BW_PRECOMPILED_HEADER( modeleditor_core pch.hpp )
BW_PROJECT_CATEGORY( modeleditor_core "ModelEditor Core" )

