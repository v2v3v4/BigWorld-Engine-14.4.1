CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( speedtree )

IF( NOT BW_SPEEDTREE_SUPPORT )
	MESSAGE( FATAL_ERROR "Speedtree CMakeLists.txt was added but BW_SPEEDTREE_SUPPORT is OFF" )
ENDIF()

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/speedtree/include )

# TODO: Work out how to build eval again!
ADD_DEFINITIONS( -DFULL_SPEEDTREE_LIB )


SET( ALL_SRCS
	pch.cpp
	pch.hpp
	speedtree_collision.cpp
	speedtree_collision.hpp
)

IF( NOT BW_IS_SERVER )
	LIST( APPEND ALL_SRCS
		billboard_optimiser.cpp
		billboard_optimiser.hpp
		speedtree_config.cpp
		speedtree_config.hpp
		speedtree_config_lite.hpp
		speedtree_lib.hpp
		speedtree_renderer.cpp
		speedtree_renderer.hpp
		speedtree_renderer_off.cpp
		speedtree_renderer_util.cpp
		speedtree_renderer_util.hpp
		speedtree_thumb_prov.hpp
		speedtree_tree_type.cpp
		speedtree_tree_type.hpp
		speedtree_util.cpp
		speedtree_util.hpp
		speedtree_vertex_types.hpp
		speedtree_vertex_types.cpp
		speedtree_renderer_common.cpp
		speedtree_renderer_common.hpp
		speedtree_renderer_methods.cpp
	)
ENDIF()

IF( BW_IS_TOOLS )
	SET( ALL_SRCS ${ALL_SRCS}
		speedtree_thumb_prov.cpp )
ENDIF()

SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( speedtree
	${BLOB_SRCS}
)

IF ( NOT BW_IS_SERVER AND BW_ARCH_32 )
	BW_TARGET_LINK_LIBRARIES( speedtree INTERFACE
		debug "${BW_SOURCE_DIR}/third_party/speedtree/lib/${BW_COMPILER_TOKEN}/Debug/SpeedTreeRT_Static_d.lib"
		optimized "${BW_SOURCE_DIR}/third_party/speedtree/lib/${BW_COMPILER_TOKEN}/Release/SpeedTreeRT_Static.lib"
	)
ELSEIF( NOT BW_IS_SERVER AND BW_ARCH_64 )
	BW_TARGET_LINK_LIBRARIES( speedtree INTERFACE
		debug "${BW_SOURCE_DIR}/third_party/speedtree/lib/${BW_COMPILER_TOKEN}/Debug (x64)/SpeedTreeRT_Static_d.lib"
		optimized "${BW_SOURCE_DIR}/third_party/speedtree/lib/${BW_COMPILER_TOKEN}/Release (x64)/SpeedTreeRT_Static.lib"
	)
ENDIF()

BW_TARGET_LINK_LIBRARIES( speedtree INTERFACE
	cstdmf
	math
	physics2
	resmgr
	)

IF( NOT BW_IS_SERVER )
	BW_TARGET_LINK_LIBRARIES( speedtree INTERFACE
		moo
		romp
	)
ENDIF()

IF( BW_IS_TOOLS )
	BW_TARGET_LINK_LIBRARIES( speedtree INTERFACE ual )
ENDIF()

BW_PRECOMPILED_HEADER( speedtree pch.hpp )
BW_PROJECT_CATEGORY( speedtree "BigWorld Libs" )
