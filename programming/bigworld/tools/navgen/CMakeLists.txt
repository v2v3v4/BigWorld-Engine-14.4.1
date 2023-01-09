CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( navgen )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardMFCProject )


INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR} )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/lib )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/common )


BW_SET_OPTIONAL_FILES( LANGUAGES_SRCS
	${BW_RESOURCE_DIR}/helpers/languages/files_en.xml
	${BW_RESOURCE_DIR}/helpers/languages/navgen_rc_en.xml
)
SOURCE_GROUP( "Languages" FILES ${LANGUAGES_SRCS} )


SET( RESOURCE_FILES_SRCS
	navgen.ico
)
SOURCE_GROUP( "Resource_Files" FILES ${RESOURCE_FILES_SRCS} )


SET( ALL_SRCS
	../../lib/controls/show_cursor_helper.cpp
	../../lib/controls/show_cursor_helper.hpp
	../common/space_mgr.cpp
	../common/space_mgr.hpp
	asyn_msg.cpp
	asyn_msg.hpp
	chunk_waypoint_generator.cpp
	chunk_waypoint_generator.hpp
	dlg_modeless_info.cpp
	dlg_modeless_info.hpp
	navgen.cpp
	navgen.rc
	navgen_udo.cpp
	navgen_udo.hpp
	pch.cpp
	pch.hpp
	resource.h
	wpentity.cpp
	wpentity.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( COMMON_SRCS
	../common/bwlockd_connection.cpp
	../common/bwlockd_connection.hpp
	../common/chop_poly.cpp
	../common/chop_poly.hpp
	../common/chunk_flooder.cpp
	../common/chunk_flooder.hpp
	../common/collision_advance.cpp
	../common/collision_advance.hpp
	../common/format.cpp
	../common/format.hpp
	../common/girth.cpp
	../common/girth.hpp
	../common/grid_coord.cpp
	../common/grid_coord.hpp
	../common/physics_handler.cpp
	../common/physics_handler.hpp
	../common/utilities.cpp
	../common/utilities.hpp
	../common/waypoint_annotator.cpp
	../common/waypoint_annotator.hpp
)
SOURCE_GROUP( "common" FILES ${COMMON_SRCS} )


BW_ADD_EXECUTABLE( navgen WIN32
	${LANGUAGES_SRCS}
	${RESOURCE_FILES_SRCS}
	${ALL_SRCS}
	${COMMON_SRCS}
)

BW_TARGET_LINK_LIBRARIES( navgen
	chunk
	cstdmf
	chunk_scene_adapter
	duplo
	input
	math
	particle
	physics2
	pyscript
	resmgr
	romp
	terrain
	waypoint_generator
	# external libraries
	opengl32
	)

IF( BW_SPEEDTREE_SUPPORT )
	BW_TARGET_LINK_LIBRARIES( navgen
		speedtree
	)
ENDIF()

SET_TARGET_PROPERTIES( navgen PROPERTIES
		DEBUG_OUTPUT_NAME
		"navgen_d" )

SET_TARGET_PROPERTIES( navgen PROPERTIES
		HYBRID_OUTPUT_NAME
		"navgen" )

BW_USE_MICROSOFT_COMMON_CONTROLS( navgen )
BW_SET_BINARY_DIR( navgen "${BW_GAME_DIR}/tools/navgen/${BW_PLATFORM}" )

BW_PROJECT_CATEGORY( navgen "Executables" )

