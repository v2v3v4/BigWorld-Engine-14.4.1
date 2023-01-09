CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( entitydef_script )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	delegate_entity_method.cpp
	delegate_entity_method.hpp
	py_component.cpp
	py_component.hpp
	pch.cpp
	pch.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( entitydef_script
	${ALL_SRCS}
)

BW_PRECOMPILED_HEADER( entitydef_script pch.hpp )
BW_PROJECT_CATEGORY( entitydef_script "BigWorld Libs" )
