CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( modeleditor )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
)

INCLUDE( BWStandardProject )
INCLUDE( BWStandardMFCProject )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR} )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/modeleditor )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/modeleditor_core )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/modeleditor_core/App )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/modeleditor_core/GUI )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/modeleditor_core/Pages )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/modeleditor_core/Models )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/lib/ual )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/nvmeshmender )

SET( RESOURCE_FILES_SRCS
	modeleditor.rc
)
SOURCE_GROUP( "Resource_Files" FILES ${RESOURCE_FILES_SRCS} )


SET( GUI_SRCS
	GUI/model_editor.cpp
	GUI/model_editor.h
)
SOURCE_GROUP( "GUI" FILES ${GUI_SRCS} )


BW_BLOB_SOURCES( BLOB_SRCS
	${RESOURCE_FILES_SRCS} 
	${GUI_SRCS}
)

BW_ADD_TOOL_EXE( modeleditor modeleditor WIN32 ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( modeleditor PRIVATE
	appmgr
	chunk
	cstdmf
	editor_shared
	gizmo
	guimanager
	modeleditor_core
	pyscript
	terrain
	ual
)

