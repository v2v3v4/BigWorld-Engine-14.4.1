CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( gizmo )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/common )

SET( ALL_SRCS
	coord_mode_provider.cpp
	coord_mode_provider.hpp
	gizmo_lib.hpp
	pch.cpp
	pch.hpp
	snap_provider.cpp
	snap_provider.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( TOOL_SRCS
	item_view.cpp
	item_view.hpp
	tool.cpp
	tool.hpp
	tool_locator.cpp
	tool_locator.hpp
	tool_manager.cpp
	tool_manager.hpp
	tool_view.cpp
	tool_view.hpp
)
SOURCE_GROUP( "Tool" FILES ${TOOL_SRCS} )


SET( UNDO_SRCS
	undoredo.cpp
	undoredo.hpp
)
SOURCE_GROUP( "Undo" FILES ${UNDO_SRCS} )


SET( FUNCTOR_SRCS
	always_applying_functor.cpp
	always_applying_functor.hpp
	dynamic_float_device.cpp
	dynamic_float_device.hpp
	matrix_mover.cpp
	matrix_mover.hpp
	matrix_positioner.cpp
	matrix_positioner.hpp
	matrix_rotator.cpp
	matrix_rotator.hpp
	matrix_scaler.cpp
	matrix_scaler.hpp
	property_scaler.cpp
	property_scaler.hpp
	scripted_tool_functor.cpp
	scripted_tool_functor.hpp
	tool_functor.cpp
	tool_functor.hpp
	wheel_rotator.cpp
	wheel_rotator.hpp
)
SOURCE_GROUP( "Functor" FILES ${FUNCTOR_SRCS} )


SET( PROPERTIES_SRCS
	current_general_properties.cpp
	current_general_properties.hpp
	formatter.cpp
	formatter.hpp
	general_editor.cpp
	general_editor.hpp
	general_properties.cpp
	general_properties.hpp
	meta_data.cpp
	meta_data.hpp
	meta_data_comments_property.cpp
	meta_data_creation_utility.hpp
	meta_data_datetime_property.cpp
	meta_data_define.hpp
	meta_data_general_property.cpp
	meta_data_helper.cpp
	meta_data_helper.hpp
	value_type.cpp
	value_type.hpp
)
SOURCE_GROUP( "Properties" FILES ${PROPERTIES_SRCS} )


SET( GIZMO_SRCS
	angle_gizmo.cpp
	angle_gizmo.hpp
	combination_gizmos.cpp
	combination_gizmos.hpp
	genprop_gizmoviews.cpp
	genprop_gizmoviews.hpp
	gizmo_manager.cpp
	gizmo_manager.hpp
	position_gizmo.cpp
	position_gizmo.hpp
	property_rotater_helper.cpp
	property_rotater_helper.hpp
	property_scaler_helper.cpp
	property_scaler_helper.hpp
	radius_gizmo.cpp
	radius_gizmo.hpp
	rotation_gizmo.cpp
	rotation_gizmo.hpp
	scale_gizmo.cpp
	scale_gizmo.hpp
	solid_shape_mesh.cpp
	solid_shape_mesh.hpp
	tile_gizmo.cpp
	tile_gizmo.hpp
)
SOURCE_GROUP( "Gizmo" FILES ${GIZMO_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${ALL_SRCS}
	${TOOL_SRCS}
	${UNDO_SRCS}
	${FUNCTOR_SRCS}
	${PROPERTIES_SRCS}
	${GIZMO_SRCS}
)
BW_ADD_LIBRARY( gizmo ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( gizmo INTERFACE
	appmgr
	chunk
	cstdmf
	input
	moo
	pyscript
	resmgr
	romp
	)

BW_PRECOMPILED_HEADER( gizmo pch.hpp )
BW_PROJECT_CATEGORY( gizmo "BigWorld Libs" )
