CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( process_defs )

INCLUDE( BWStandardProject )

SET( SOURCE_SRCS
	help_msg_handler.cpp help_msg_handler.hpp
	main.cpp
)
SOURCE_GROUP( "Source" FILES ${SOURCE_SRCS} )


BW_ADD_EXECUTABLE( process_defs
	${SOURCE_SRCS}
)


BW_TARGET_LINK_LIBRARIES( process_defs
	memhook
	chunk
	cstdmf
	entitydef
	math
	network
	pyscript
	resmgr
	script
	)


SET_TARGET_PROPERTIES( process_defs PROPERTIES
		DEBUG_OUTPUT_NAME
		"process_defs_d" )

SET_TARGET_PROPERTIES( process_defs PROPERTIES
		HYBRID_OUTPUT_NAME
		"process_defs" )

BW_SET_BINARY_DIR( process_defs "${BW_GAME_DIR}/tools/process_defs/${BW_PLATFORM}" )

BW_PROJECT_CATEGORY( process_defs "Executables" )
