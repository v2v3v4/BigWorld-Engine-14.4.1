CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( jit_compiler )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
	-DBW_ASSET_PIPELINE
)

INCLUDE( BWStandardProject )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/WTL/include )

SET( RESOURCE_FILES_SRCS
	res/jit.ico
	res/jit_warning.ico
	res/jit_error.ico
	app.rc
	resource.h
)
SOURCE_GROUP( "Resources" FILES ${RESOURCE_FILES_SRCS} )

SET( GUI_SRCS
	about_dialog.cpp
	about_dialog.hpp
	config_dialog.cpp
	config_dialog.hpp
	details_dialog.cpp
	details_dialog.hpp
	large_task_list_box.cpp
	large_task_list_box.hpp
	main_window.cpp
	main_window.hpp
	options_dialog.cpp
	options_dialog.hpp
	system_tray_icon.cpp
	system_tray_icon.hpp
	task_list_box.cpp
	task_list_box.hpp
	task_list_box_base.cpp
	task_list_box_base.hpp
)
SOURCE_GROUP( "Gui" FILES ${GUI_SRCS} )

SET( PLUGIN_SRCS
	../plugin_system/plugin.hpp
	../plugin_system/plugin_loader.cpp
	../plugin_system/plugin_loader.hpp
)
SOURCE_GROUP( "Plugin" FILES ${PLUGIN_SRCS} )

SET( ALL_SRCS
	jit_compiler.cpp
	jit_compiler.hpp
	jit_compiler_options.cpp
	jit_compiler_options.hpp
	main.cpp
	message_loop.cpp
	message_loop.hpp
	signal.cpp
	signal.hpp
	task_fwd.hpp
	task_info.cpp
	task_info.hpp
	task_store.cpp
	task_store.hpp
	wtl.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${PLUGIN_SRCS}
	${GUI_SRCS}
	${RESOURCE_FILES_SRCS}
	${ALL_SRCS}
)

BW_ADD_ASSETPIPELINE_EXE( jit_compiler WIN32 ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( jit_compiler PRIVATE
	compiler
	conversion
	cstdmf
	re2
	resmgr
)

ADD_DEPENDENCIES( jit_compiler bsp_converter )
ADD_DEPENDENCIES( jit_compiler effect_converter )
ADD_DEPENDENCIES( jit_compiler texformat_converter )
ADD_DEPENDENCIES( jit_compiler texture_converter )
ADD_DEPENDENCIES( jit_compiler hierarchical_config_converter )
ADD_DEPENDENCIES( jit_compiler space_converter )
ADD_DEPENDENCIES( jit_compiler visual_processor )
ADD_DEPENDENCIES( jit_compiler primitive_processor )
