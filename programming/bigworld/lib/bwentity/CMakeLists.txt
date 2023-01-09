CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( bwentity )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	bw_warnings_on.hpp
	bw_warnings_off.hpp
	bwentity_api.hpp
	bwentity_namespace.hpp
	bwentity_main.cpp
	bwentity_types.hpp
	bwresource_helper.cpp
	bwresource_helper.hpp
	entitydef_interface.hpp
	entity_description_map.cpp
	entity_description_map_factory.cpp
	entity_description_map_factory.hpp
	entity_description_map.hpp
	object_deleters.cpp
	object_deleters.hpp
	pch.cpp
	pch.hpp
	reference_count.hpp
	udo_ref_data_type.cpp
	udo_ref_data_type.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

IF ( BW_USE_BWENTITY_IN_DLL )
	REMOVE_DEFINITIONS( 
		-DBWENTITY_DLL_IMPORT
		-DCSTDMF_IMPORT
	)

	ADD_DEFINITIONS( 
		-DBWENTITY_DLL_EXPORT
		-DCSTDMF_EXPORT
	)
ENDIF()

IF ( NOT BW_USE_BWENTITY_DLL )
	MESSAGE( FATAL_ERROR "BW_USE_BWENTITY_DLL is not set when building bwentity library" )
ENDIF()

BW_ADD_LIBRARY( bwentity SHARED ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( bwentity PRIVATE
	connection
	connection_model
	cstdmf
	#delegate_interface
	entitydef
	math
	network
	resmgr
	script
)

IF( ${BW_PLATFORM} MATCHES "win64" )
	SET( BWENTITY_OUTPUT_NAME "bwentity64" )
ELSE()
	SET( BWENTITY_OUTPUT_NAME "bwentity" )
ENDIF()

# Set output binary name, PDB binary name and PDB output directory
# The PDB settings should match the defaults, but we need the values
# available for the POST_BUILD below
SET_TARGET_PROPERTIES( bwentity PROPERTIES
	OUTPUT_NAME_DEBUG ${BWENTITY_OUTPUT_NAME}_d
	PDB_NAME_DEBUG ${BWENTITY_OUTPUT_NAME}_d
)

SET_TARGET_PROPERTIES( bwentity PROPERTIES
	OUTPUT_NAME_HYBRID ${BWENTITY_OUTPUT_NAME}_h
	PDB_NAME_HYBRID ${BWENTITY_OUTPUT_NAME}_h
)

SET_TARGET_PROPERTIES( bwentity PROPERTIES
	OUTPUT_NAME_CONSUMER_RELEASE ${BWENTITY_OUTPUT_NAME}
	PDB_NAME_CONSUMER_RELEASE ${BWENTITY_OUTPUT_NAME}
)

BW_SET_BINARY_DIR( bwentity "${BW_GAME_DIR}/dll/${BW_PLATFORM}" )

BW_PRECOMPILED_HEADER( bwentity pch.hpp )	
BW_PROJECT_CATEGORY( bwentity "BigWorld Entity Dll" )
