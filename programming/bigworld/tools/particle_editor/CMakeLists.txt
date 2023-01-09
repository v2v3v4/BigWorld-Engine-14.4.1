CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( particle_editor )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
)

INCLUDE( BWStandardProject )
INCLUDE( BWStandardMFCProject )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR} )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/particle_editor )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/lib/ual )

GET_FILENAME_COMPONENT( BW_PARTILEEDITOR_RESOURCE_DIR "${BW_GAME_DIR}/tools/particleeditor/resources" REALPATH )

SET( SHELL_SRCS
	shell/pe_module.cpp
	shell/pe_module.hpp
	shell/pe_scripter.cpp
	shell/pe_scripter.hpp
	shell/pe_shell.cpp
	shell/pe_shell.hpp
)
SOURCE_GROUP( "Shell" FILES ${SHELL_SRCS} )


SET( GUI_SRCS
	../common/page_messages.cpp
	../common/page_messages.hpp
	gui/action_selection.cpp
	gui/action_selection.hpp
	gui/dialogs/splash_dialog.cpp
	gui/dialogs/splash_dialog.hpp
	gui/gui_utilities.cpp
	gui/gui_utilities.hpp
	gui/panel_manager.cpp
	gui/panel_manager.hpp
	gui/ps_node.cpp
	gui/ps_node.hpp
	gui/ps_tree.cpp
	gui/ps_tree.hpp
	gui/vector_generator_custodian.cpp
	gui/vector_generator_custodian.hpp
	gui/vector_generator_proxies.cpp
	gui/vector_generator_proxies.hpp
)
SOURCE_GROUP( "GUI" FILES ${GUI_SRCS} )


SET( PROPERTY_DIALOGS_GUI_SRCS
	gui/propdlgs/mps_properties.cpp
	gui/propdlgs/mps_properties.hpp
	gui/propdlgs/ps_properties.cpp
	gui/propdlgs/ps_properties.hpp
	gui/propdlgs/ps_renderer_properties.cpp
	gui/propdlgs/ps_renderer_properties.hpp
	gui/propdlgs/psa_barrier_properties.cpp
	gui/propdlgs/psa_barrier_properties.hpp
	gui/propdlgs/psa_collide_properties.cpp
	gui/propdlgs/psa_collide_properties.hpp
	gui/propdlgs/psa_empty_properties.cpp
	gui/propdlgs/psa_empty_properties.hpp
	gui/propdlgs/psa_flare_properties.cpp
	gui/propdlgs/psa_flare_properties.hpp
	gui/propdlgs/psa_force_properties.cpp
	gui/propdlgs/psa_force_properties.hpp
	gui/propdlgs/psa_jitter_properties.cpp
	gui/propdlgs/psa_jitter_properties.hpp
	gui/propdlgs/psa_magnet_properties.cpp
	gui/propdlgs/psa_magnet_properties.hpp
	gui/propdlgs/psa_matrixswarm_properties.cpp
	gui/propdlgs/psa_matrixswarm_properties.hpp
	gui/propdlgs/psa_nodeclamp_properties.cpp
	gui/propdlgs/psa_nodeclamp_properties.hpp
	gui/propdlgs/psa_orbitor_properties.cpp
	gui/propdlgs/psa_orbitor_properties.hpp
	gui/propdlgs/psa_properties.cpp
	gui/propdlgs/psa_properties.hpp
	gui/propdlgs/psa_scaler_properties.cpp
	gui/propdlgs/psa_scaler_properties.hpp
	gui/propdlgs/psa_sink_properties.cpp
	gui/propdlgs/psa_sink_properties.hpp
	gui/propdlgs/psa_source_properties.cpp
	gui/propdlgs/psa_source_properties.hpp
	gui/propdlgs/psa_splat_properties.cpp
	gui/propdlgs/psa_splat_properties.hpp
	gui/propdlgs/psa_stream_properties.cpp
	gui/propdlgs/psa_stream_properties.hpp
	gui/propdlgs/psa_tint_shader_properties.cpp
	gui/propdlgs/psa_tint_shader_properties.hpp
)
SOURCE_GROUP( "property_dialogs\\GUI" FILES ${PROPERTY_DIALOGS_GUI_SRCS} )


BW_SET_OPTIONAL_FILES( LANGUAGES_SRCS
	${BW_RESOURCE_DIR}/helpers/languages/files_en.xml
	${BW_RESOURCE_DIR}/helpers/languages/particleeditor_gui_en.xml
	${BW_RESOURCE_DIR}/helpers/languages/particleeditor_rc_en.xml
)
SOURCE_GROUP( "Languages" FILES ${LANGUAGES_SRCS} )


SET( BIGBANG_COPIES_SRCS
	bigbang/grid_coord.cpp
	bigbang/grid_coord.hpp
)
SOURCE_GROUP( "BigBang_copies" FILES ${BIGBANG_COPIES_SRCS} )


SET( CONTROLS_GUI_SRCS
	gui/controls/color_picker_dialog.cpp
	gui/controls/color_picker_dialog.hpp
	gui/controls/drag_listbox.cpp
	gui/controls/drag_listbox.hpp
	gui/controls/drop_target.cpp
	gui/controls/drop_target.hpp
	gui/controls/tree_control.cpp
	gui/controls/tree_control.hpp
)
SOURCE_GROUP( "controls\\GUI" FILES ${CONTROLS_GUI_SRCS} )

BW_SET_OPTIONAL_FILES( SCRIPTS_SRCS
	${BW_PARTILEEDITOR_RESOURCE_DIR}/scripts/MenuUIAdapter.py
	${BW_PARTILEEDITOR_RESOURCE_DIR}/scripts/ParticleEditorDirector.py
	${BW_PARTILEEDITOR_RESOURCE_DIR}/scripts/ToolbarUIAdapter.py
	${BW_PARTILEEDITOR_RESOURCE_DIR}/scripts/UIAdapter.py
)
SOURCE_GROUP( "Scripts" FILES ${SCRIPTS_SRCS} )

BW_SET_OPTIONAL_FILES( GUI_DATA_SRCS
	${BW_PARTILEEDITOR_RESOURCE_DIR}/data/gui.xml
)
SOURCE_GROUP( "GUI_Data" FILES ${GUI_DATA_SRCS} )

SET( RESOURCE_FILES_SRCS
	particle_editor.rc
	res/Toolbar.bmp
	res/aboutBoxPE.bmp
	res/actionseltb.bmp
	res/addact.bmp
	res/addactd.bmp
	res/clearfilter.bmp
	res/empty.bmp
	res/logo.bmp
	res/open.bmp
	res/opend.bmp
	res/particle_editor.ico
	res/particle_editor.rc2
	res/particle_editorDoc.ico
	res/save.bmp
	res/searchfilter.bmp
	res/splashscreen.bmp
)
SOURCE_GROUP( "Resource_Files" FILES ${RESOURCE_FILES_SRCS} )


SET( MAIN_SRCS
	about_dlg.cpp
	about_dlg.hpp
	fwd.hpp
	main_frame.cpp
	main_frame.hpp
	particle_editor.cpp
	particle_editor.hpp
	particle_editor_doc.cpp
	particle_editor_doc.hpp
	particle_editor_view.cpp
	particle_editor_view.hpp
	pch.cpp
	pch.hpp
	pe_app.cpp
	pe_app.hpp
	resource.h
	undoredo_op.cpp
	undoredo_op.hpp
)
SOURCE_GROUP( "Main" FILES ${MAIN_SRCS} )




BW_BLOB_SOURCES( BLOB_SRCS
	${SHELL_SRCS}
	${GUI_SRCS}
	${GUI_DATA_SRCS}
	${PROPERTY_DIALOGS_GUI_SRCS}
	${LANGUAGES_SRCS}
	${BIGBANG_COPIES_SRCS}
	${CONTROLS_GUI_SRCS}
	${SCRIPTS_SRCS}
	${RESOURCE_FILES_SRCS}
	${MAIN_SRCS}
)

BW_ADD_TOOL_EXE( particle_editor particleeditor WIN32 ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( particle_editor PRIVATE
	appmgr
	chunk
	chunk_scene_adapter
	controls
	cstdmf
	gizmo
	guimanager
	moo
	particle
	post_processing
	romp
	tools_common
	ual
	)

IF( BW_FMOD_SUPPORT )
	BW_TARGET_LINK_LIBRARIES( particle_editor
		fmodsound
	)
ENDIF()

# Resetting the target names as they are different from the project name.
# This is so we can make use of the BW_ADD_TOOL_EXE
# without adding code specific to this project.
SET_TARGET_PROPERTIES( particle_editor PROPERTIES
		DEBUG_OUTPUT_NAME
		"particleeditor_d" )

SET_TARGET_PROPERTIES( particle_editor PROPERTIES
		HYBRID_OUTPUT_NAME
		"particleeditor" )

BW_PRECOMPILED_HEADER( particle_editor pch.hpp )

