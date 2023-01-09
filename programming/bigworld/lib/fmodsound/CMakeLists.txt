CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( fmodsound )

IF( NOT BW_FMOD_SUPPORT )
	MESSAGE( FATAL_ERROR "FMOD CMakeLists.txt was added but BW_FMOD_SUPPORT is OFF" )
ENDIF()

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	fmod_config.hpp
	fmodsound_lib.hpp
	pch.cpp
	pch.hpp
	py_event_category.cpp
	py_event_category.hpp
	py_event_group.cpp
	py_event_group.hpp
	py_event_project.cpp
	py_event_project.hpp
	py_event_reverb.cpp
	py_event_reverb.hpp
	py_music_system.cpp
	py_music_system.hpp
	py_sound.cpp
	py_sound.hpp
	py_sound_list.cpp
	py_sound_list.hpp
	py_sound_parameter.cpp
	py_sound_parameter.hpp
	sound_manager.cpp
	sound_manager.hpp
	sound_occluder.cpp
	sound_occluder.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( fmodsound
	${BLOB_SRCS}
)

BW_TARGET_LINK_LIBRARIES( fmodsound INTERFACE
	cstdmf
	duplo
	math
	model
	moo
	physics2
	pyscript
	resmgr
	romp
	terrain
	)

IF( NOT BW_IS_SERVER AND BW_ARCH_32 ) 
	BW_TARGET_LINK_LIBRARIES( fmodsound INTERFACE
		fmodex_vc
		fmod_event_net
		)
ELSEIF( NOT BW_IS_SERVER AND BW_ARCH_64 )
	BW_TARGET_LINK_LIBRARIES( fmodsound INTERFACE
		fmodex64_vc
		fmod_event_net64
		)
ELSE()
	MESSAGE( FATAL_ERROR "invalid platform '${BW_PLATFORM}'!" )
ENDIF()

TARGET_INCLUDE_DIRECTORIES( fmodsound PUBLIC
	"${BW_SOURCE_DIR}/third_party/fmod/api/inc"
	"${BW_SOURCE_DIR}/third_party/fmod/fmoddesignerapi/api/inc"
	)

BW_PRECOMPILED_HEADER( fmodsound pch.hpp )
BW_PROJECT_CATEGORY( fmodsound "BigWorld Libs" )
