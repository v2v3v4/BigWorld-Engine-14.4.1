CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( post_processing )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

# TODO: Remove this once virtual hiding warnings fixed
BW_REMOVE_COMPILE_FLAGS( /w34263 )

SET( ALL_SRCS
	debug.cpp
	debug.hpp
	debug.ipp
	effect.cpp
	effect.hpp
	effect.ipp
	filter_quad.cpp
	filter_quad.hpp
	filter_quad.ipp
	filter_quad_factory.cpp
	filter_quad_factory.hpp
	manager.cpp
	manager.hpp
	manager.ipp
	pch.cpp
	pch.hpp
	phase.cpp
	phase.hpp
	phase.ipp
	phase_factory.cpp
	phase_factory.hpp
	py_copy_back_buffer.cpp
	py_copy_back_buffer.hpp
	py_copy_back_buffer.ipp
	py_filter_quad.cpp
	py_filter_quad.hpp
	py_filter_quad.ipp
	py_phase.cpp
	py_phase.hpp
	py_phase.ipp
	py_phase_editor.cpp
	py_phase_editor.hpp
	py_phase_editor_proxies.cpp
	py_phase_editor_proxies.hpp
	py_point_sprite_transfer_mesh.cpp
	py_point_sprite_transfer_mesh.hpp
	py_point_sprite_transfer_mesh.ipp
	py_transfer_quad.cpp
	py_transfer_quad.hpp
	py_transfer_quad.ipp
	py_visual_transfer_mesh.cpp
	py_visual_transfer_mesh.hpp
	py_visual_transfer_mesh.ipp
	todo.txt
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( post_processing
	${BLOB_SRCS}
)

BW_TARGET_LINK_LIBRARIES( post_processing INTERFACE
	cstdmf
	math
	moo
	pyscript
	romp
	)

IF( BW_IS_TOOLS )
	BW_TARGET_LINK_LIBRARIES( post_processing INTERFACE gizmo )
ENDIF()

BW_PRECOMPILED_HEADER( post_processing pch.hpp )
BW_PROJECT_CATEGORY( post_processing "BigWorld Libs" )
