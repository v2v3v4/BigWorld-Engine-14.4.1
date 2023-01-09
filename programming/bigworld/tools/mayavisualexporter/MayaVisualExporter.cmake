MACRO( BW_MAYA_VISUAL_EXPORTER_PROJECT _LIBRARY_NAME _MAYA_VERSION )
	INCLUDE( BWStandardProject )

	ADD_DEFINITIONS(
		-DWIN32
		-D_WINDOWS
		-D_USRDLL 
		-DVISUALTEST_EXPORTS
	)

	INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/maya/${_MAYA_VERSION}/include )
	INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/nvmeshmender )
	INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/mayavisualexporter )
	IF( ${BW_PLATFORM} STREQUAL "win32" )
		SET( BW_BINARY_DIR ${BW_GAME_DIR}/tools/exporter/maya${_MAYA_VERSION} )
		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/maya/${_MAYA_VERSION}/lib/Win32 )
	ELSEIF( ${BW_PLATFORM} STREQUAL "win64" )
		SET( BW_BINARY_DIR ${BW_GAME_DIR}/tools/exporter/maya${_MAYA_VERSION}x64 )
		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/maya/${_MAYA_VERSION}/lib/x64 )
	ENDIF()

	SET( ALL_SRCS
		${BW_SOURCE_DIR}/tools/resourcechecker/visual_checker.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/ExportIterator.h
		${BW_SOURCE_DIR}/tools/mayavisualexporter/VisualFileExporterScript.mel
		${BW_SOURCE_DIR}/tools/mayavisualexporter/VisualFileTranslator.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/VisualFileTranslator.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/blendshapes.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/blendshapes.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/boneset.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/boneset.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/bonevertex.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/bonevertex.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/constraints.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/constraints.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/exporter_dialog.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/expsets.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/expsets.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/expsets.ipp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/expsetsio.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/face.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/face.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/hierarchy.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/hierarchy.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/hull_mesh.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/hull_mesh.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/material.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/material.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/matrix3.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/matrix4.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/mayavisualexporter.rc
		${BW_SOURCE_DIR}/tools/mayavisualexporter/mesh.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/mesh.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/pch.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/pch.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/portal.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/portal.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/resource.h
		${BW_SOURCE_DIR}/tools/mayavisualexporter/skin.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/skin.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/utility.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/utility.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/vector2.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/vector3.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/vertex.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/vertex.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/visual_envelope.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/visual_envelope.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/visual_envelope.ipp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/visual_mesh.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/visual_mesh.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/visual_mesh.ipp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/visual_portal.cpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/visual_portal.hpp
		${BW_SOURCE_DIR}/tools/mayavisualexporter/visualmain.cpp
	)
	SOURCE_GROUP( "" FILES ${ALL_SRCS} )


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
		${BW_SOURCE_DIR}/tools/exporter_common/vertex_formats.hpp
	)
	SOURCE_GROUP( "Common" FILES ${COMMON_SRCS} )




	ADD_LIBRARY( ${_LIBRARY_NAME} SHARED 
		${ALL_SRCS}
		${COMMON_SRCS}
	)

	TARGET_LINK_LIBRARIES( ${_LIBRARY_NAME}
		d3d9 
		d3dx9 
		dxguid 
		Foundation 
		Image 
		libmocap 
		OpenMaya 
		OpenMayaAnim 
		OpenMayaFX 
		OpenMayaRender 
		OpenMayaUI 
		cstdmf
		math
		moo
		physics2
		resmgr
		re2
		zip
		nvmeshmender
	)

	SET_TARGET_PROPERTIES( ${_LIBRARY_NAME} PROPERTIES
			DEBUG_OUTPUT_NAME
			"visual_d" )

	SET_TARGET_PROPERTIES( ${_LIBRARY_NAME} PROPERTIES
			HYBRID_OUTPUT_NAME
			"visual" )
			
	SET_TARGET_PROPERTIES( ${_LIBRARY_NAME}
			PROPERTIES
			SUFFIX ".mll"
			PREFIX "" )

	SET_TARGET_PROPERTIES( ${_LIBRARY_NAME} PROPERTIES COMPILE_FLAGS "/Yupch.hpp" )
	SET_SOURCE_FILES_PROPERTIES( ${BW_SOURCE_DIR}/tools/mayavisualexporter/pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.hpp" )
			
	BW_PROJECT_CATEGORY( ${_LIBRARY_NAME} "Maya Plugin" )
	BW_SET_BINARY_DIR( ${_LIBRARY_NAME} ${BW_BINARY_DIR} )

ENDMACRO( BW_MAYA_VISUAL_EXPORTER_PROJECT )

