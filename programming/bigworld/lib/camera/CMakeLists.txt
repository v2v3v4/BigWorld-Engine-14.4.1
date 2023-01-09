CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( camera )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	annal.cpp
	annal.hpp
	base_camera.cpp
	base_camera.hpp
	base_camera.ipp
	camera_blender.cpp
	camera_blender.hpp
	camera_control.cpp
	camera_control.hpp
	collision_advance.cpp
	collision_advance.hpp
	cursor_camera.cpp
	cursor_camera.hpp
	cursor_camera.ipp
	direction_cursor.cpp
	direction_cursor.hpp
	direction_cursor.ipp
	flexicam.cpp
	flexicam.hpp
	flexicam.ipp
	free_camera.cpp
	free_camera.hpp
	pch.cpp
	pch.hpp
	projection_access.cpp
	projection_access.hpp
	projection_access.ipp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( camera
	${BLOB_SRCS}
)

BW_TARGET_LINK_LIBRARIES( camera INTERFACE
	chunk
	cstdmf
	input
	math
	moo
	physics2
	romp
	space
	)

BW_PRECOMPILED_HEADER( camera pch.hpp )
BW_PROJECT_CATEGORY( camera "BigWorld Libs" )
