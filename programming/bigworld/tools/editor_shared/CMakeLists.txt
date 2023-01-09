CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

SET( SHARED_SOURCE_FILES_SRCS
	app/i_editor_app.hpp
	cursor/cursor.hpp
	cursor/cursor.cpp
	cursor/wait_cursor.hpp
	gui/i_main_frame.hpp
	pages/gui_tab_content.hpp
	dialogs/file_dialog.hpp
	dialogs/folder_guard.hpp
	dialogs/folder_guard.cpp
	dialogs/message_box.hpp 
)
SOURCE_GROUP( "Shared_Source_Files" FILES ${SHARED_SOURCE_FILES_SRCS} )

IF( BW_IS_QT_TOOLS )
	INCLUDE( "CMakeLists.qt.txt" )
ELSE()
	INCLUDE( "CMakeLists.mfc.txt" )
ENDIF()
