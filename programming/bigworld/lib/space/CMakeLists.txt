CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( space )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	pch.hpp
	pch.cpp
	forward_declarations.hpp
	space_dll.hpp
	space_lib.hpp

	space_manager.hpp
	space_manager.cpp
	client_space.hpp
	client_space.cpp
	debug_draw.hpp
	debug_draw.cpp
	dynamic_scene_provider.hpp
	dynamic_scene_provider.cpp
	dynamic_scene_provider_types.hpp
	dynamic_light_scene_provider.hpp
	dynamic_light_scene_provider.cpp

	deprecated_space_helpers.hpp
	deprecated_space_helpers.cpp

	enviro_minder_scene_provider.hpp
	enviro_minder_scene_provider.cpp

	space_interfaces.hpp
	space_interfaces.cpp
	client_space_factory.hpp
	client_space_factory.cpp
	entity_embodiment.hpp
	entity_embodiment.cpp
	global_embodiments.hpp
	global_embodiments.cpp
	light_embodiment.hpp
	light_embodiment.cpp

	script_object_query_operation.hpp
	script_object_query_operation.cpp

	space_romp_collider.hpp
	space_romp_collider.cpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( space ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( space INTERFACE
	chunk
	cstdmf
	math
	moo
	network
	physics2
	resmgr
	romp
	scene
	)

BW_PRECOMPILED_HEADER( space pch.hpp )
BW_PROJECT_CATEGORY( space "BigWorld Libs" )
