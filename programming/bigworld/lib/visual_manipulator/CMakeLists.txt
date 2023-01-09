CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( visual_manipulator )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/nvmeshmender )

SET( ALL_SRCS
	animation.cpp
	animation.hpp
	bloat_vertex.cpp
	bloat_vertex.hpp
	hull.cpp
	hull.hpp
	mesh_piece.cpp
	mesh_piece.hpp
	node.cpp
	node.hpp
	pch.cpp
	pch.hpp
	portal.cpp
	portal.hpp
	skin_splitter.cpp
	skin_splitter.hpp
	triangle.cpp
	triangle.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( visual_manipulator
	${ALL_SRCS}
)

BW_PROJECT_CATEGORY( visual_manipulator "BigWorld Libs" )
