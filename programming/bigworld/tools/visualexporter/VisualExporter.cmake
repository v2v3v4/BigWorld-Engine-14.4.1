MACRO( BW_VISUAL_EXPORTER_PROJECT _LIBRARY_NAME _MAX_VERSION )
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
	INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/nvmeshmender )
	INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/visualexporter )
	INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/lib )

	SET( ABOUT_BOX_SRCS
		${BW_SOURCE_DIR}/tools/visualexporter/aboutbox.cpp
		${BW_SOURCE_DIR}/tools/visualexporter/aboutbox.hpp
	)
	SOURCE_GROUP( "About_box" FILES ${ABOUT_BOX_SRCS} )


	SET( ALL_SRCS
		${BW_SOURCE_DIR}/tools/visualexporter/max.h
		${BW_SOURCE_DIR}/tools/visualexporter/resource.h
	)
	SOURCE_GROUP( "" FILES ${ALL_SRCS} )


	SET( SETTINGS_SRCS
		${BW_SOURCE_DIR}/tools/visualexporter/expsets.cpp
		${BW_SOURCE_DIR}/tools/visualexporter/expsets.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/expsetsio.cpp
	)
	SOURCE_GROUP( "Settings" FILES ${SETTINGS_SRCS} )


	SET( RESOURCE_FILES_SRCS
		${BW_SOURCE_DIR}/tools/visualexporter/visualexporter.rc
	)
	SOURCE_GROUP( "Resource_Files" FILES ${RESOURCE_FILES_SRCS} )


	SET( COMMON_SRCS
		${BW_SOURCE_DIR}/tools/exporter_common/bsp_generator.cpp
		${BW_SOURCE_DIR}/tools/exporter_common/bsp_generator.hpp
		${BW_SOURCE_DIR}/tools/exporter_common/data_section_cache_purger.cpp
		${BW_SOURCE_DIR}/tools/exporter_common/data_section_cache_purger.hpp
		${BW_SOURCE_DIR}/tools/exporter_common/node_catalogue_holder.cpp
		${BW_SOURCE_DIR}/tools/exporter_common/node_catalogue_holder.hpp
		${BW_SOURCE_DIR}/tools/exporter_common/skin_splitter.cpp
		${BW_SOURCE_DIR}/tools/exporter_common/skin_splitter.hpp
		${BW_SOURCE_DIR}/tools/exporter_common/skin_splitter.ipp
	)
	SOURCE_GROUP( "Common" FILES ${COMMON_SRCS} )


	SET( SOURCE_FILES_SRCS
		${BW_SOURCE_DIR}/tools/resourcechecker/visual_checker.cpp
		${BW_SOURCE_DIR}/tools/resourcechecker/visual_checker.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/chunkids.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/expmain.cpp
		${BW_SOURCE_DIR}/tools/visualexporter/hull_mesh.cpp
		${BW_SOURCE_DIR}/tools/visualexporter/hull_mesh.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/mfxexp.cpp
		${BW_SOURCE_DIR}/tools/visualexporter/mfxexp.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/mfxexp.ipp
		${BW_SOURCE_DIR}/tools/visualexporter/mfxnode.cpp
		${BW_SOURCE_DIR}/tools/visualexporter/mfxnode.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/mfxnode.ipp
		${BW_SOURCE_DIR}/tools/visualexporter/morpher_holder.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/pch.cpp
		${BW_SOURCE_DIR}/tools/visualexporter/pch.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/polyCont.cpp
		${BW_SOURCE_DIR}/tools/visualexporter/polyCont.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/utility.cpp
		${BW_SOURCE_DIR}/tools/visualexporter/utility.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/vertcont.cpp
		${BW_SOURCE_DIR}/tools/visualexporter/vertcont.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/visual_envelope.cpp
		${BW_SOURCE_DIR}/tools/visualexporter/visual_envelope.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/visual_mesh.cpp
		${BW_SOURCE_DIR}/tools/visualexporter/visual_mesh.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/visual_portal.cpp
		${BW_SOURCE_DIR}/tools/visualexporter/visual_portal.hpp
		${BW_SOURCE_DIR}/tools/visualexporter/visualexporter.def
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
		core
		d3d9
		d3dx9
		geom
		maxutil
		mesh
		imagehlp
		comctl32
		bmm
		dxguid
		cstdmf
		math
		moo
		physics2
		resmgr
		re2
		libpng
		zip
		nvmeshmender
	)

	TARGET_LINK_LIBRARIES( ${_LIBRARY_NAME} optimized maxscrpt )

	SET_PROPERTY( DIRECTORY APPEND PROPERTY
		COMPILE_DEFINITIONS_DEBUG
		BW_EXPORTER_DEBUG )

	SET_TARGET_PROPERTIES( ${_LIBRARY_NAME} PROPERTIES
			DEBUG_OUTPUT_NAME
			"visualexporter_d" )

	SET_TARGET_PROPERTIES( ${_LIBRARY_NAME} PROPERTIES
			HYBRID_OUTPUT_NAME
			"visualexporter" )
			
	SET_TARGET_PROPERTIES( ${_LIBRARY_NAME}
			PROPERTIES
			SUFFIX ".dle"
			PREFIX "" )
			
	BW_PROJECT_CATEGORY( ${_LIBRARY_NAME} "3dsMax Plugin" )
	BW_SET_BINARY_DIR( ${_LIBRARY_NAME} ${BW_BINARY_DIR} )

ENDMACRO( BW_VISUAL_EXPORTER_PROJECT )

