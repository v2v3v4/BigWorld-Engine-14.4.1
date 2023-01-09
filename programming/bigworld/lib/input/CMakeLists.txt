CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( input )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	ImeUi.cpp
	ImeUi.h
	ime.cpp
	ime.hpp
	ime_event.hpp
	input.cpp
	input.hpp
	input.ipp
	input_common.cpp
	input_cursor.cpp
	input_cursor.hpp
	input_lib.hpp
	key_code.cpp
	key_code.hpp
	pch.cpp
	pch.hpp
	py_input.cpp
	py_input.hpp
	vk_map.cpp
	vk_map.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( input
	${BLOB_SRCS}
)

BW_TARGET_LINK_LIBRARIES( input INTERFACE
	cstdmf
	math
	pyscript
	# external libraries
	dinput8
	)

BW_PRECOMPILED_HEADER( input pch.hpp )
BW_PROJECT_CATEGORY( input "BigWorld Libs" )
