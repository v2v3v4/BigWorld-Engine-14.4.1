CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( scene )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	pch.hpp
	pch.cpp
	forward_declarations.hpp
	scene_lib.hpp
	scene.hpp
	scene.cpp
	scene_dll.hpp
	scene_listener.hpp
	scene_listener.cpp
	scene_object.hpp
	scene_object.cpp
	scene_object_flags.hpp
	scene_object_flags.cpp
	scene_provider.hpp
	scene_provider.cpp
	scene_type_system.cpp
	scene_type_system.hpp
	simple_scene_provider.hpp

	tick_scene_view.hpp
	tick_scene_view.cpp

	collision_operation.hpp
	collision_operation.cpp
	collision_scene_view.hpp
	collision_scene_view.cpp

	draw_operation.hpp
	draw_operation.cpp

	intersection_set.cpp
	intersection_set.hpp

	intersect_scene_view.hpp
	intersect_scene_view.cpp
)
IF( NOT BW_IS_SERVER )
	LIST( APPEND ALL_SRCS
		light_scene_view.hpp
		light_scene_view.cpp
		draw_helpers.hpp
		draw_helpers.cpp
		scene_draw_context.hpp
		scene_draw_context.ipp
		scene_draw_context.cpp
		scene_intersect_context.hpp
		scene_intersect_context.cpp
		scene_event_view.hpp
		scene_event_view.cpp
		change_scene_view.hpp
		change_scene_view.cpp
		spatial_query_operation.hpp
		spatial_query_operation.cpp
		object_change_scene_view.hpp
		object_change_scene_view.cpp
	)
ELSE()
	LIST( APPEND ALL_SRCS
		draw_object_operation.hpp
		draw_object_operation.cpp
	)
ENDIF()

SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( scene ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( scene INTERFACE
	cstdmf
	math
	moo
	physics2
	)

BW_PRECOMPILED_HEADER( scene pch.hpp )
BW_PROJECT_CATEGORY( scene "BigWorld Libs" )
