CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( scaleform )

IF( NOT BW_SCALEFORM_SUPPORT )
	MESSAGE( FATAL_ERROR "Scaleform CMakeLists.txt was added but BW_SCALEFORM_SUPPORT is OFF" )
ENDIF()

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	config.hpp
	flash_gui_component.cpp
	flash_gui_component.hpp
	flash_text_gui_component.cpp
	flash_text_gui_component.hpp
	ime.cpp
	ime.hpp
	loader.cpp
	loader.hpp
	log.cpp
	log.hpp
	manager.cpp
	manager.hpp
	pch.cpp
	pch.hpp
	py_external_interface.cpp
	py_external_interface.hpp
	py_fs_command_handler.cpp
	py_fs_command_handler.hpp
	py_movie_def.cpp
	py_movie_def.hpp
	py_movie_view.cpp
	py_movie_view.hpp
	util.cpp
	util.hpp
	fx_game_delegate.cpp
	fx_game_delegate.hpp
	py_gfx_display_object.cpp
	py_gfx_display_object.hpp
	py_gfx_script_function.cpp
	py_gfx_script_function.hpp
	py_gfx_value.cpp
	py_gfx_value.hpp
	sys_alloc.cpp
	sys_alloc.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( scaleform
	${BLOB_SRCS}
)

FIND_PACKAGE( ScaleformSDK 4.1.19 EXACT )

IF( SCALEFORMSDK_FOUND )
	ADD_DEFINITIONS( -DSCALEFORM_SUPPORT=1 )

	TARGET_INCLUDE_DIRECTORIES( scaleform PRIVATE ${SCALEFORMSDK_INCLUDE_DIRS} )
	BW_TARGET_LINK_LIBRARIES( scaleform INTERFACE ${SCALEFORMSDK_LIBRARIES} )
ELSE()
	ADD_DEFINITIONS( -DSCALEFORM_SUPPORT=0 )
ENDIF()

BW_TARGET_LINK_LIBRARIES( scaleform INTERFACE
	ashes
	cstdmf
	math
	moo
	pyscript
	resmgr
	romp
	)

IF( BW_FMOD_SUPPORT )
	BW_TARGET_LINK_LIBRARIES( scaleform INTERFACE fmodsound )
	TARGET_INCLUDE_DIRECTORIES( scaleform PRIVATE ${FMOD_INCLUDE_DIRS} )
ENDIF()

BW_PRECOMPILED_HEADER( scaleform pch.hpp )
BW_PROJECT_CATEGORY( scaleform "BigWorld Libs" )
