CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( controls )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

ADD_DEFINITIONS( 
	-D_AFXDLL
)

# TODO: Remove this once virtual hiding warnings fixed
BW_REMOVE_COMPILE_FLAGS( /w34263 )

SET( ALL_SRCS
	defs.hpp
	fwd.hpp
	pch.cpp
	pch.hpp
	user_messages.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( LAYOUT_SRCS
	grid_sizer.cpp
	grid_sizer.hpp
	layout_manager.hpp
	null_sizer.cpp
	null_sizer.hpp
	row_sizer.cpp
	row_sizer.hpp
	sizer.cpp
	sizer.hpp
	wnd_sizer.cpp
	wnd_sizer.hpp
)
SOURCE_GROUP( "Layout" FILES ${LAYOUT_SRCS} )


SET( MISC._CONTROLS_SRCS
	auto_tooltip.hpp
	ccombobox_auto_complete.cpp
	ccombobox_auto_complete.hpp
	color_picker.cpp
	color_picker.hpp
	color_timeline.cpp
	color_timeline.hpp
	dialog_toolbar.cpp
	dialog_toolbar.hpp
	image_control.cpp
	image_control.hpp
	image_control.ipp
	memdc.cpp
	memdc.hpp
	resource_file_dialog.hpp
	search_field.cpp
	search_field.hpp
)
SOURCE_GROUP( "Misc._controls" FILES ${MISC._CONTROLS_SRCS} )


SET( BUTTON_CONTROLS_SRCS
	image_button.cpp
	image_button.hpp
	themed_image_button.cpp
	themed_image_button.hpp
)
SOURCE_GROUP( "Button_controls" FILES ${BUTTON_CONTROLS_SRCS} )


SET( SLIDER_CONTROLS_SRCS
	range_slider_ctrl.cpp
	range_slider_ctrl.hpp
	slider.cpp
	slider.hpp
)
SOURCE_GROUP( "Slider_controls" FILES ${SLIDER_CONTROLS_SRCS} )


SET( STATIC_CONTROLS_SRCS
	color_static.cpp
	color_static.hpp
	separator.cpp
	separator.hpp
	wait_anim.cpp
	wait_anim.hpp
)
SOURCE_GROUP( "Static_controls" FILES ${STATIC_CONTROLS_SRCS} )


SET( MISC._SRCS
	cursor_utils.cpp
	cursor_utils.hpp
	dib_section.hpp
	dib_section.ipp
	dib_section32.hpp
	dib_section8.cpp
	dib_section8.hpp
	dir_dialog.cpp
	dir_dialog.hpp
	drag_image.cpp
	drag_image.hpp
	show_cursor_helper.cpp
	show_cursor_helper.hpp
	utils.cpp
	utils.hpp
	embeddable_cdialog.hpp
	embeddable_cdialog.cpp
)
SOURCE_GROUP( "Misc." FILES ${MISC._SRCS} )


SET( EDIT_CONTROLS_SRCS
	csearch_filter.cpp
	csearch_filter.hpp
	edit_commit.cpp
	edit_commit.hpp
	edit_numeric.cpp
	edit_numeric.hpp
)
SOURCE_GROUP( "Edit_controls" FILES ${EDIT_CONTROLS_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${ALL_SRCS}
	${LAYOUT_SRCS}
	${MISC._CONTROLS_SRCS}
	${BUTTON_CONTROLS_SRCS}
	${SLIDER_CONTROLS_SRCS}
	${STATIC_CONTROLS_SRCS}
	${MISC._SRCS}
	${EDIT_CONTROLS_SRCS}
)
BW_ADD_LIBRARY( controls ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( controls INTERFACE
	cstdmf
	math
	romp
	resmgr
	# external libraries
	shell32
	shlwapi
	)

BW_PRECOMPILED_HEADER( controls pch.hpp )
BW_PROJECT_CATEGORY( controls "BigWorld Libs" )
