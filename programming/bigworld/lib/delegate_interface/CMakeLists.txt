CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( delegate_interface )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	space_physics_delegate.hpp
	entity_delegate.hpp
	game_delegate.cpp
	game_delegate.hpp
	space_world_factory.hpp
	space_world.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( delegate_interface ${ALL_SRCS} )

BW_PROJECT_CATEGORY( delegate_interface "BigWorld Server" )
