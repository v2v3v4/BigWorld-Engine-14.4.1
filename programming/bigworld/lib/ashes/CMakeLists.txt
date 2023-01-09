CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( ashes )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	alpha_gui_shader.cpp
	alpha_gui_shader.hpp
	alpha_gui_shader.ipp
	bounding_box_gui_component.cpp
	bounding_box_gui_component.hpp
	bounding_box_gui_component.ipp
	clip_gui_shader.cpp
	clip_gui_shader.hpp
	clip_gui_shader.ipp
	colour_gui_shader.cpp
	colour_gui_shader.hpp
	colour_gui_shader.ipp
	console_gui_component.cpp
	console_gui_component.hpp
	frame_gui_component.cpp
	frame_gui_component.hpp
	frame_gui_component.ipp
	frame_gui_component2.cpp
	frame_gui_component2.hpp
	gobo_component.cpp
	gobo_component.hpp
	graph_gui_component.cpp
	graph_gui_component.hpp
	gui_attachment.cpp
	gui_attachment.hpp
	gui_progress.cpp
	gui_progress.hpp
	gui_shader.cpp
	gui_shader.hpp
	gui_vertex_format.hpp
	matrix_gui_shader.cpp
	matrix_gui_shader.hpp
	mesh_gui_adaptor.cpp
	mesh_gui_adaptor.hpp
	mesh_gui_adaptor.ipp
	mouse_cursor.cpp
	mouse_cursor.hpp
	pch.cpp
	pch.hpp
	simple_gui.cpp
	simple_gui.hpp
	simple_gui.ipp
	simple_gui_component.cpp
	simple_gui_component.hpp
	simple_gui_component.ipp
	text_gui_component.cpp
	text_gui_component.hpp
	text_gui_component.ipp
	window_gui_component.cpp
	window_gui_component.hpp
	window_gui_component.ipp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( ashes
	${BLOB_SRCS}
)

BW_TARGET_LINK_LIBRARIES( ashes INTERFACE
	cstdmf
	input
	math
	moo
	pyscript
	resmgr
	romp
	)

BW_PRECOMPILED_HEADER( ashes pch.hpp )
BW_PROJECT_CATEGORY( ashes "BigWorld Libs" )
