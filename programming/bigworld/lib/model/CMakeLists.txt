CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( model )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	dye_prop_setting.hpp
	dye_property.hpp
	dye_selection.hpp
	fashion.cpp
	fashion.hpp
	forward_declarations.hpp
	match_info.cpp
	match_info.hpp
	matter.cpp
	matter.hpp
	model.cpp
	model.ipp
	model.hpp
	model_action.cpp
	model_action.hpp
	model_actions_iterator.cpp
	model_actions_iterator.hpp
	model_animation.cpp
	model_animation.hpp
	model_dye.cpp
	model_dye.hpp
	model_factory.cpp
	model_factory.hpp
	model_lib.hpp
	model_map.cpp
	model_map.hpp
	node_tree.cpp
	node_tree.hpp
	nodefull_model.cpp
	nodefull_model.hpp
	nodefull_model_animation.cpp
	nodefull_model_animation.hpp
	nodefull_model_default_animation.cpp
	nodefull_model_default_animation.hpp
	nodeless_model.cpp
	nodeless_model.hpp
	pch.cpp
	pch.hpp
	super_model.cpp
	super_model.hpp
	super_model.ipp
	super_model_action.cpp
	super_model_action.hpp
	super_model_animation.cpp
	super_model_animation.hpp
	super_model_dye.cpp
	super_model_dye.hpp
	super_model_node_tree.cpp
	super_model_node_tree.hpp
	switched_model.hpp
	switched_model_animation.hpp
	tint.cpp
	tint.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( model
	${BLOB_SRCS}
)

BW_TARGET_LINK_LIBRARIES( model INTERFACE
	cstdmf
	math
	moo
	resmgr
	)

BW_PRECOMPILED_HEADER( model pch.hpp )
BW_PROJECT_CATEGORY( model "BigWorld Libs" )
