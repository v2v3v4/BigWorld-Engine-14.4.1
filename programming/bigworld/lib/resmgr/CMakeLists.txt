CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( resmgr )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	access_monitor.cpp
	access_monitor.hpp
	access_monitor.ipp
	auto_config.cpp
	auto_config.hpp
	bundiff.cpp
	bundiff.hpp
	bwresource.cpp
	bwresource.hpp
	bwresource.ipp
	file_handle_streamer.cpp
	file_handle_streamer.hpp
	file_streamer.hpp
	forward_declarations.hpp
	memory_exchange.hpp
	pch.cpp
	pch.hpp
	primitive_file.cpp
	primitive_file.hpp
	primitive_file.ipp
	quick_file_writer.hpp
	resource_cache.hpp
	resource_file_path.cpp
	resource_file_path.hpp
	resmgr_lib.hpp
	resmgr_test_utils.cpp
	resmgr_test_utils.hpp
	string_provider.cpp
	string_provider.hpp
	xml_special_chars.cpp
	xml_special_chars.hpp
)

IF( NOT BW_IS_SERVER )
	LIST( APPEND ALL_SRCS
		hierarchical_config.cpp
		hierarchical_config.hpp
	)
ELSEIF( BW_IS_SERVER )
	LIST( APPEND ALL_SRCS
		bdiff.cpp
		bdiff.hpp
	)
ENDIF()

SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( DATASECTION_SRCS
	bin_section.cpp
	bin_section.hpp
	binary_block.cpp
	binary_block.hpp
	data_section_cache.cpp
	data_section_cache.hpp
	data_section_census.cpp
	data_section_census.hpp
	dataresource.cpp
	dataresource.hpp
	datasection.cpp
	datasection.hpp
	dir_section.cpp
	dir_section.hpp
	packed_section.cpp
	packed_section.hpp
	sanitise_helper.cpp
	sanitise_helper.hpp
	xml_section.cpp
	xml_section.hpp
	zip_section.cpp
	zip_section.hpp
)
SOURCE_GROUP( "DataSection" FILES ${DATASECTION_SRCS} )


SET( FILESYSTEM_SRCS
	file_system.hpp
	multi_file_system.cpp
	multi_file_system.hpp
	zip_file_system.cpp
	zip_file_system.hpp
)
IF( NOT BW_IS_SERVER )
	LIST( APPEND FILESYSTEM_SRCS
		filename_case_checker.cpp
		filename_case_checker.hpp
		resource_modification_listener.cpp
		resource_modification_listener.hpp
	)
ENDIF()
IF ( BW_PLATFORM_WINDOWS )
	LIST( APPEND FILESYSTEM_SRCS
		win_file_system.cpp
		win_file_system.hpp
		win_file_streamer.cpp
		win_file_streamer.hpp
	)
ELSEIF( BW_PLATFORM_LINUX )
	LIST( APPEND FILESYSTEM_SRCS
		unix_file_system.cpp
		unix_file_system.hpp
	)
ENDIF()
SOURCE_GROUP( "FileSystem" FILES ${FILESYSTEM_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${ALL_SRCS}
	${DATASECTION_SRCS}
	${FILESYSTEM_SRCS}
)

ADD_DEFINITIONS(
	-DHAVE_ZIP
)

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

BW_ADD_LIBRARY( resmgr ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( resmgr
	INTERFACE
	cstdmf
	math
	# third party
	zip
	)

IF( BW_PLATFORM_WINDOWS )
	BW_TARGET_LINK_LIBRARIES( resmgr
		INTERFACE
		# third party
		re2
		# external libraries
		shlwapi
		version
		)

	TARGET_INCLUDE_DIRECTORIES( resmgr PRIVATE
		"${BW_SOURCE_DIR}/third_party/re2" )
ENDIF()

BW_PRECOMPILED_HEADER( resmgr pch.hpp )
BW_PROJECT_CATEGORY( resmgr "BigWorld Libs" )
