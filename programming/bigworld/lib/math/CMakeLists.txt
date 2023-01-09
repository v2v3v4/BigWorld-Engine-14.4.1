CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( math )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	boundbox.cpp
	boundbox.hpp
	boundbox.ipp
	ema.cpp
	ema.hpp
	forward_declarations.hpp
	integrat.hpp
	linear_animation.hpp
	math_extra.cpp
	math_extra.hpp
	math_lib.hpp
	mathdef.hpp
	matrix.cpp
	matrix.hpp
	matrix.ipp
	matrix_liason.hpp
	pch.cpp
	pch.hpp
	planeeq.cpp
	planeeq.hpp
	planeeq.ipp
	portal2d.cpp
	portal2d.hpp
	portal2d.ipp
	quat.cpp
	quat.hpp
	quat.ipp
	range1dt.hpp
	range1dt.ipp
	rectt.hpp
	rectt.ipp
	sma.hpp
	stat_watcher_creator.cpp
	stat_watcher_creator.hpp
	stat_with_rates_of_change.hpp
	stat_with_rates_of_change.ipp
	vector2.cpp
	vector2.hpp
	vector2.ipp
	vector3.cpp
	vector3.hpp
	vector3.ipp
	vector4.cpp
	vector4.hpp
	vector4.ipp
	xp_math.hpp
)
IF( NOT BW_IS_SERVER )
	LIST( APPEND ALL_SRCS
		angle.hpp
		angle.ipp
		blend_transform.cpp
		blend_transform.hpp
		colour.hpp
		convex_hull.cpp
		convex_hull.hpp
		graham_scan.hpp
		frustum_hull.hpp
		frustum_hull.cpp
		linear_lut.cpp
		linear_lut.hpp
		lineeq.hpp
		lineeq.ipp
		lineeq3.hpp
		lineeq3.ipp
		loose_octree.cpp
		loose_octree.ipp
		loose_octree.hpp
		loose_octree_logging.cpp
		loose_octree_logging.hpp
		oriented_bbox.cpp
		oriented_bbox.hpp
		perlin_noise.cpp
		perlin_noise.hpp
		polygon.cpp
		polygon.hpp
		polyhedron.cpp
		polyhedron.hpp
		simplex_noise.cpp
		simplex_noise.hpp
		sphere.cpp
		sphere.hpp
		sphere.ipp
	)
ENDIF()
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

IF ( BW_USE_BWENTITY_IN_DLL )
	REMOVE_DEFINITIONS( 
		-DBWENTITY_DLL_IMPORT
		-DCSTDMF_IMPORT
	)

	ADD_DEFINITIONS( 
		-DBWENTITY_DLL_EXPORT
		-DCSTDMF_EXPORT
	)
ENDIF()

BW_ADD_LIBRARY( math
	${BLOB_SRCS}
)

IF( BW_PLATFORM_WINDOWS )
	BW_TARGET_LINK_LIBRARIES( math INTERFACE
		# external libraries
		d3dx9
	)
ENDIF()

BW_PRECOMPILED_HEADER( math pch.hpp )
BW_PROJECT_CATEGORY( math "BigWorld Libs" )
