CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( guimanager )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardMFCProject )
INCLUDE( BWStandardLibrary )

SET( SOURCE_FILES_SRCS
	gui_bitmap.cpp
	gui_functor.cpp
	gui_functor_cpp.cpp
	gui_functor_datasection.cpp
	gui_functor_option.cpp
	gui_functor_python.cpp
	gui_functor_watcher.cpp
	gui_input_handler.cpp
	gui_item.cpp
	gui_manager.cpp
	gui_menu.cpp
	gui_test.cpp
	gui_toolbar.cpp
	pch.cpp
	py_gui_item.cpp
)
SOURCE_GROUP( "Source_Files" FILES ${SOURCE_FILES_SRCS} )


SET( HEADER_FILES_SRCS
	gui_action_maker.hpp
	gui_bitmap.hpp
	gui_forward.hpp
	gui_functor.hpp
	gui_functor_cpp.hpp
	gui_functor_datasection.hpp
	gui_functor_option.hpp
	gui_functor_python.hpp
	gui_functor_watcher.hpp
	gui_input_handler.hpp
	gui_item.hpp
	gui_manager.hpp
	gui_menu.hpp
	gui_textor_maker.hpp
	gui_toolbar.hpp
	gui_updater_maker.hpp
	guimanager_lib.hpp
	pch.hpp
	py_gui_item.hpp
)
SOURCE_GROUP( "Header_Files" FILES ${HEADER_FILES_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${SOURCE_FILES_SRCS}
	${HEADER_FILES_SRCS}
)
BW_ADD_LIBRARY( guimanager ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( guimanager INTERFACE
	cstdmf
	input
	pyscript
	resmgr
	)

BW_PRECOMPILED_HEADER( guimanager pch.hpp )
BW_PROJECT_CATEGORY( guimanager "BigWorld Libs" )
