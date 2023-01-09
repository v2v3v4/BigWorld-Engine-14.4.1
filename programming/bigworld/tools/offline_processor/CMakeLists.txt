CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( offline_processor )

ADD_DEFINITIONS( 
	-DCSTDMF_IMPORT
)

INCLUDE( BWStandardProject )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/offline_processor )

SET( COMMANDS_SRCS
	command.cpp
	command.hpp
	command_upgrade_bsp.cpp
	command_upgrade_bsp.hpp
)
SOURCE_GROUP( "Commands" FILES ${COMMANDS_SRCS} )


SET( RESOURCE_FILES_SRCS
	offline_processor.ico
	offline_processor.rc
)
SOURCE_GROUP( "Resource_Files" FILES ${RESOURCE_FILES_SRCS} )


SET( PROCESSCHUNKSCOMMAND_COMMANDS_SRCS
	command_process_chunks.cpp
	command_process_chunks.hpp
	offline_chunk_processor_manager.cpp
	offline_chunk_processor_manager.hpp
)
SOURCE_GROUP( "ProcessChunksCommand\\Commands" FILES ${PROCESSCHUNKSCOMMAND_COMMANDS_SRCS} )

SET( SOURCE_FILES_SRCS
	offline_processor_main.cpp
	command_manager.cpp
	pch.cpp
)
SOURCE_GROUP( "Source_Files" FILES ${SOURCE_FILES_SRCS} )


SET( HEADER_FILES_SRCS
	command_manager.hpp
	pch.hpp
	resource.h
	targetver.h
)
SOURCE_GROUP( "Header_Files" FILES ${HEADER_FILES_SRCS} )


BW_BLOB_SOURCES( BLOB_SRCS
	${COMMANDS_SRCS}
	${PROCESSOR_TOOLS_COMMON_SRCS}
	${RESOURCE_FILES_SRCS}
	${PROCESSCHUNKSCOMMAND_COMMANDS_SRCS}
	${SOURCE_FILES_SRCS}
	${HEADER_FILES_SRCS}
)

BW_ADD_TOOL_EXE( offline_processor offline_processor ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( offline_processor PRIVATE
	chunk
	cstdmf
	moo
	resmgr
	physics2
	terrain
)

BW_PRECOMPILED_HEADER( offline_processor pch.hpp )
