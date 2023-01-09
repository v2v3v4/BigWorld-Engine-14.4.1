CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

MACRO( BW_ANIMATION_EXPORTER_PROJECT _LIBRARY_NAME _MAX_VERSION )
	INCLUDE( BWStandardProject )

	# TODO: Remove this once virtual hiding warnings fixed
	BW_REMOVE_COMPILE_FLAGS( /w34263 )

	IF( ${BW_PLATFORM} STREQUAL "win32" )
		SET( BW_BINARY_DIR ${BW_GAME_DIR}/tools/exporter/3dsmax${_MAX_VERSION} )
		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/3dsmax/${_MAX_VERSION}/lib/Win32 )
	ELSEIF( ${BW_PLATFORM} STREQUAL "win64" )
		SET( BW_BINARY_DIR ${BW_GAME_DIR}/tools/exporter/3dsmax${_MAX_VERSION}x64 )		
		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/3dsmax/${_MAX_VERSION}/lib/x64 )
	ENDIF()
	INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/3dsmax/${_MAX_VERSION}/include )
	INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/3dsmax/${_MAX_VERSION}/include/CS )
	INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/3dsmax/${_MAX_VERSION}/samples/modifiers/morpher )
	INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/lib )
	INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/animationexporter )

	SET( ABOUT_BOX_SRCS
		${BW_SOURCE_DIR}/tools/animationexporter/aboutbox.cpp
		${BW_SOURCE_DIR}/tools/animationexporter/aboutbox.hpp
		${BW_SOURCE_DIR}/tools/animationexporter/aboutbox.ipp
	)
	SOURCE_GROUP( "About_box" FILES ${ABOUT_BOX_SRCS} )

	SET( ALL_SRCS
		${BW_SOURCE_DIR}/tools/animationexporter/max.h
		${BW_SOURCE_DIR}/tools/animationexporter/resource.h
	)
	SOURCE_GROUP( "" FILES ${ALL_SRCS} )

	SET( SETTINGS_SRCS
		${BW_SOURCE_DIR}/tools/animationexporter/expsets.cpp
		${BW_SOURCE_DIR}/tools/animationexporter/expsets.hpp
		${BW_SOURCE_DIR}/tools/animationexporter/expsetsio.cpp
	)
	SOURCE_GROUP( "Settings" FILES ${SETTINGS_SRCS} )

	SET( RESOURCE_FILES_SRCS
		${BW_SOURCE_DIR}/tools/animationexporter/animationexporter.rc
	)
	SOURCE_GROUP( "Resource_Files" FILES ${RESOURCE_FILES_SRCS} )

	SET( COMMON_SRCS
		${BW_SOURCE_DIR}/tools/exporter_common/data_section_cache_purger.cpp
		${BW_SOURCE_DIR}/tools/exporter_common/data_section_cache_purger.hpp
	)
	SOURCE_GROUP( "Common" FILES ${COMMON_SRCS} )

	SET( SOURCE_FILES_SRCS
		${BW_SOURCE_DIR}/tools/animationexporter/animationexporter.def
		${BW_SOURCE_DIR}/tools/animationexporter/chunkids.hpp
		${BW_SOURCE_DIR}/tools/animationexporter/cuetrack.cpp
		${BW_SOURCE_DIR}/tools/animationexporter/cuetrack.hpp
		${BW_SOURCE_DIR}/tools/animationexporter/expmain.cpp
		${BW_SOURCE_DIR}/tools/animationexporter/mfxexp.cpp
		${BW_SOURCE_DIR}/tools/animationexporter/mfxexp.hpp
		${BW_SOURCE_DIR}/tools/animationexporter/mfxfile.cpp
		${BW_SOURCE_DIR}/tools/animationexporter/mfxfile.hpp
		${BW_SOURCE_DIR}/tools/animationexporter/mfxfile.ipp
		${BW_SOURCE_DIR}/tools/animationexporter/mfxnode.cpp
		${BW_SOURCE_DIR}/tools/animationexporter/mfxnode.hpp
		${BW_SOURCE_DIR}/tools/animationexporter/mfxnode.ipp
		${BW_SOURCE_DIR}/tools/animationexporter/pch.cpp
		${BW_SOURCE_DIR}/tools/animationexporter/pch.hpp
		${BW_SOURCE_DIR}/tools/animationexporter/polyCont.cpp
		${BW_SOURCE_DIR}/tools/animationexporter/polyCont.hpp
		${BW_SOURCE_DIR}/tools/animationexporter/utility.cpp
		${BW_SOURCE_DIR}/tools/animationexporter/utility.hpp
		${BW_SOURCE_DIR}/tools/animationexporter/vertcont.cpp
		${BW_SOURCE_DIR}/tools/animationexporter/vertcont.hpp
		${BW_SOURCE_DIR}/tools/animationexporter/visual_envelope.cpp
		${BW_SOURCE_DIR}/tools/animationexporter/visual_envelope.hpp
		${BW_SOURCE_DIR}/tools/animationexporter/visual_envelope.ipp
		${BW_SOURCE_DIR}/tools/animationexporter/visual_mesh.cpp
		${BW_SOURCE_DIR}/tools/animationexporter/visual_mesh.hpp
		${BW_SOURCE_DIR}/tools/animationexporter/visual_mesh.ipp
	)
	SOURCE_GROUP( "Source_Files" FILES ${SOURCE_FILES_SRCS} )

	ADD_LIBRARY( ${_LIBRARY_NAME} SHARED 
		${ABOUT_BOX_SRCS}
		${ALL_SRCS}
		${SETTINGS_SRCS}
		${RESOURCE_FILES_SRCS}
		${COMMON_SRCS}
		${SOURCE_FILES_SRCS}
	)

	TARGET_LINK_LIBRARIES( ${_LIBRARY_NAME}
		cstdmf
		math
		resmgr
		zip
		maxscrpt
		core
		d3d9
		d3dx9
		geom
		maxutil
		mesh
		imagehlp
		comctl32
	)

	SET_TARGET_PROPERTIES( ${_LIBRARY_NAME} PROPERTIES
			DEBUG_OUTPUT_NAME
			"animationexporter_d" )

	SET_TARGET_PROPERTIES( ${_LIBRARY_NAME} PROPERTIES
			HYBRID_OUTPUT_NAME
			"animationexporter" )
			
	SET_TARGET_PROPERTIES( ${_LIBRARY_NAME}
			PROPERTIES
			SUFFIX ".dle"
			PREFIX "" )
			
	BW_PROJECT_CATEGORY( ${_LIBRARY_NAME} "3dsMax Plugin" )
	BW_SET_BINARY_DIR( ${_LIBRARY_NAME} ${BW_BINARY_DIR} )

ENDMACRO( BW_ANIMATION_EXPORTER_PROJECT )

