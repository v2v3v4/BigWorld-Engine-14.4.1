CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( connection )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	avatar_filter_helper.cpp
	avatar_filter_helper.hpp
	baseapp_ext_interface.hpp
	baseapp_login_request_protocol.cpp
	baseapp_login_request_protocol.hpp
	client_interface.hpp
	client_server_protocol_version.hpp
	common_baseapp_interface.hpp
	common_client_interface.hpp
	condemned_interfaces.cpp
	condemned_interfaces.hpp
	connection_transport.hpp
	cuckoo_cycle_login_challenge_factory.cpp
	cuckoo_cycle_login_challenge_factory.hpp
	data_download.cpp
	data_download.hpp
	download_segment.hpp
	filter_environment.hpp
	filter_helper.cpp
	filter_helper.hpp
	log_on_params.cpp
	log_on_params.hpp
	log_on_status.hpp
	login_challenge.hpp
	login_challenge_factory.cpp
	login_challenge_factory.hpp
	login_challenge_task.cpp
	login_challenge_task.hpp
	login_handler.cpp
	login_handler.hpp
	login_reply_record.hpp
	login_request.cpp
	login_request.hpp
	loginapp_login_request_protocol.cpp
	loginapp_login_request_protocol.hpp
	loginapp_public_key.hpp
	message_handlers.hpp
	movement_filter.cpp
	movement_filter.hpp
	replay_checksum_scheme.cpp
	replay_checksum_scheme.hpp
	replay_controller.cpp
	replay_controller.hpp
	replay_data_file_reader.cpp
	replay_data_file_reader.hpp
	replay_header.cpp
	replay_header.hpp
	replay_metadata.cpp
	replay_metadata.hpp
	replay_tick_loader.cpp
	replay_tick_loader.hpp
	rsa_stream_encoder.hpp
	server_connection_handler.hpp
	server_finder.cpp
	server_finder.hpp
	server_message_handler.hpp
	smart_server_connection.cpp
	smart_server_connection.hpp
	stream_encoder.hpp
)

IF ( NOT BW_IS_CLIENT )
	SET (ALL_SRCS
		${ALL_SRCS}
		avatar_filter.cpp
		avatar_filter.hpp
	)
ENDIF()

# Don't include these sources in tbe blob due to macro complexity
SET( NO_BLOB_SRCS
	login_interface.cpp
	login_interface.hpp
	login_request_protocol.cpp
	login_request_protocol.hpp
	login_request_transport.cpp
	login_request_transport.hpp
	pch.cpp
	pch.hpp
	server_connection.cpp
	server_connection.hpp
	server_connection.ipp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} ${NO_BLOB_SRCS} )

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

BW_ADD_LIBRARY( connection
	${BLOB_SRCS}
	${NO_BLOB_SRCS}
)

FIND_PACKAGE( BWOpenSSL REQUIRED )
IF( BWOPENSSL_FOUND )
	ADD_DEFINITIONS( -DUSE_OPENSSL )
	INCLUDE_DIRECTORIES( ${BWOPENSSL_INCLUDE_DIRS} )
	TARGET_LINK_LIBRARIES( connection INTERFACE ${BWOPENSSL_LIBRARIES} )
ENDIF()

BW_TARGET_LINK_LIBRARIES( connection INTERFACE
	cstdmf
	math
	network
	)

IF( NOT BW_BLOB_CONFIG )
	# Having some issues with pdb creation race condition when using pch and blobs
	BW_PRECOMPILED_HEADER( connection pch.hpp )
ENDIF()

BW_PROJECT_CATEGORY( connection "BigWorld Libs" )
