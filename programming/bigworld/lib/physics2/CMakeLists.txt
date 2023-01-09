CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( physics2 )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( COLLISION_SRCS
	bsp.cpp
	bsp.hpp
	bsp.ipp
	hulltree.cpp
	hulltree.hpp
	hulltree.ipp
	worldpoly.cpp
	worldpoly.hpp
	worldpoly.ipp
	worldtri.cpp
	worldtri.hpp
	worldtri.ipp
	sweep_helpers.cpp
	sweep_helpers.hpp
	sweep_shape.hpp
	collision_interface.hpp
	collision_callback.hpp
	collision_obstacle.hpp
)
SOURCE_GROUP( "collision" FILES ${COLLISION_SRCS} )


SET( ALL_SRCS
	forward_declarations.hpp
	material_kinds.cpp
	material_kinds.hpp
	pch.cpp
	pch.hpp
	physics2_lib.hpp
	quad_tree.cpp
	quad_tree.hpp
	quad_tree.ipp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


BW_BLOB_SOURCES( BLOB_SRCS
	${COLLISION_SRCS}
	${ALL_SRCS}
)
BW_ADD_LIBRARY( physics2 ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( physics2 INTERFACE
	cstdmf
	math
	resmgr
	)

BW_PRECOMPILED_HEADER( physics2 pch.hpp )
BW_PROJECT_CATEGORY( physics2 "BigWorld Libs" )
