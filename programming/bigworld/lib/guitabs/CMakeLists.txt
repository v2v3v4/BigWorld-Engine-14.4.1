CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( guitabs )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardMFCProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	content.hpp
	content_container.cpp
	content_container.hpp
	content_factory.hpp
	datatypes.hpp
	dock.cpp
	dock.hpp
	dock_node.cpp
	dock_node.hpp
	docked_panel_node.cpp
	docked_panel_node.hpp
	drag_manager.cpp
	drag_manager.hpp
	floater.cpp
	floater.hpp
	guitabs.hpp
	guitabs_content.hpp
	main_view_node.cpp
	main_view_node.hpp
	manager.cpp
	manager.hpp
	nice_splitter_wnd.cpp
	nice_splitter_wnd.hpp
	panel.cpp
	panel.hpp
	pch.cpp
	pch.hpp
	splitter_node.cpp
	splitter_node.hpp
	tab.cpp
	tab.hpp
	tab_ctrl.cpp
	tab_ctrl.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${ALL_SRCS}
)
BW_ADD_LIBRARY( guitabs ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( guitabs INTERFACE
	cstdmf
	resmgr
	)

BW_PRECOMPILED_HEADER( guitabs pch.hpp )
BW_PROJECT_CATEGORY( guitabs "BigWorld Libs" )
