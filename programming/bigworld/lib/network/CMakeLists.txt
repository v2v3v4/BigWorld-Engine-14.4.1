CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( network )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )


SET( ALL_SRCS
	basictypes.cpp
	basictypes.hpp
	basictypes.ipp
	block_cipher.hpp
	block_cipher.ipp
	bsd_snprintf.cpp
	bsd_snprintf.h
	buffered_tcp_endpoint.cpp
	buffered_tcp_endpoint.hpp
	bundle.cpp
	bundle.hpp
	bundle.ipp
	channel.cpp
	channel.hpp
	channel.ipp
	channel_finder.hpp
	channel_listener.hpp
	channel_map.cpp
	channel_map.hpp
	channel_owner.cpp
	channel_owner.hpp
	channel_sender.hpp
	circular_array.hpp
	common_interface_macros.hpp
	common_message_handlers.hpp
	compression_stream.cpp
	compression_stream.hpp
	condemned_channels.cpp
	condemned_channels.hpp
	deferred_bundle.cpp
	deferred_bundle.hpp
	delayed_channels.cpp
	delayed_channels.hpp
	dispatcher_coupling.hpp
	elliptic_curve_checksum_scheme.cpp
	elliptic_curve_checksum_scheme.hpp
	encryption_filter.cpp
	encryption_filter.hpp
	encryption_stream_filter.cpp
	encryption_stream_filter.hpp
	endpoint.cpp
	endpoint.hpp
	endpoint.ipp
	error_reporter.cpp
	error_reporter.hpp
	event_dispatcher.cpp
	event_dispatcher.hpp
	event_dispatcher.ipp
	event_poller.cpp
	event_poller.hpp
	exposed_message_range.cpp
	exposed_message_range.hpp
	format_string_handler.hpp
	forwarding_reply_handler.cpp
	forwarding_reply_handler.hpp
	forwarding_string_handler.cpp
	forwarding_string_handler.hpp
	fragmented_bundle.cpp
	fragmented_bundle.hpp
	frequent_tasks.cpp
	frequent_tasks.hpp
	http_messages.cpp
	http_messages.hpp
	interface_element.cpp
	interface_element.hpp
	interface_macros.hpp
	interface_minder.cpp
	interface_minder.hpp
	interface_minder.ipp
	interface_table.cpp
	interface_table.hpp
	interfaces.hpp
	irregular_channels.cpp
	irregular_channels.hpp
	keepalive_channels.cpp
	keepalive_channels.hpp
	logger_endpoint.cpp
	logger_endpoint.hpp
	logger_message_forwarder.cpp
	logger_message_forwarder.hpp
	machine_guard.cpp
	machine_guard.hpp
	machine_guard_response_checker.cpp
	machine_guard_response_checker.hpp
	machined_utils.cpp
	machined_utils.hpp
	message_filter.hpp
	misc.hpp
	monitored_channels.cpp
	monitored_channels.hpp
	msgtypes.hpp
	msgtypes.ipp
	network_interface.cpp
	network_interface.hpp
	network_interface.ipp
	network_lib.hpp
	network_stats.hpp
	network_stats.cpp
	network_utils.cpp
	network_utils.hpp
	nub_exception.hpp
	nub_exception.ipp
	once_off_packet.cpp
	once_off_packet.hpp
	packet.cpp
	packet.hpp
	packet_filter.cpp
	packet_filter.hpp
	packet_filter.ipp
	packet_loss_parameters.hpp
	packet_loss_parameters.cpp
	packet_monitor.hpp
	packet_receiver.cpp
	packet_receiver.hpp
	packet_receiver.ipp
	packet_receiver_stats.cpp
	packet_receiver_stats.hpp
	packet_receiver_stats.ipp
	packet_sender.cpp
	packet_sender.hpp
	pch.cpp
	pch.hpp
	portmap.hpp
	process_socket_stats_helper.cpp
	process_socket_stats_helper.hpp
	public_key_cipher.cpp
	public_key_cipher.hpp
	recently_dead_channels.cpp
	recently_dead_channels.hpp
	reliable_order.hpp
	reply_order.hpp
	request.cpp
	request.hpp
	request_manager.cpp
	request_manager.hpp
	rescheduled_sender.cpp
	rescheduled_sender.hpp
	sending_stats.cpp
	sending_stats.hpp
	sha_checksum_scheme.cpp
	sha_checksum_scheme.hpp
	signed_checksum_scheme.cpp
	signed_checksum_scheme.hpp
	space_data_mapping.cpp
	space_data_mapping.hpp
	space_data_mappings.cpp
	space_data_mappings.hpp
	stream_filter.hpp
	stream_filter_factory.hpp
	symmetric_block_cipher.cpp
	symmetric_block_cipher.hpp
	tcp_bundle.cpp
	tcp_bundle.hpp
	tcp_bundle_processor.cpp
	tcp_bundle_processor.hpp
	tcp_channel.cpp
	tcp_channel.hpp
	tcp_channel_stream_adaptor.hpp
	tcp_connection_opener.cpp
	tcp_connection_opener.hpp
	tcp_server.cpp
	tcp_server.hpp
	udp_bundle.cpp
	udp_bundle.hpp
	udp_bundle.ipp
	udp_bundle_processor.cpp
	udp_bundle_processor.hpp
	udp_channel.cpp
	udp_channel.hpp
	udp_channel.ipp
	unacked_packet.cpp
	unacked_packet.hpp
	unpacked_message_header.cpp
	unpacked_message_header.hpp
	websocket_handshake_handler.hpp
	websocket_stream_filter.cpp
	websocket_stream_filter.hpp
	zip_stream.cpp
	zip_stream.hpp
)

IF( BW_IS_SERVER )
	LIST( APPEND ALL_SRCS
		address_resolver.cpp
		address_resolver.hpp
		blocking_reply_handler.cpp
		blocking_reply_handler.hpp
		bundle_sending_map.cpp
		bundle_sending_map.hpp
		bw_message_forwarder.cpp
		bw_message_forwarder.hpp
		file_stream.cpp
		file_stream.hpp
		file_stream.ipp
		netmask.cpp
		netmask.hpp
		watcher_connection.cpp
		watcher_connection.hpp
		watcher_endpoint.hpp
		watcher_nub.cpp
		watcher_nub.hpp
		watcher_packet_handler.cpp
		watcher_packet_handler.hpp
	)
ENDIF()

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

BW_ADD_LIBRARY( network
	${BLOB_SRCS}
)

FIND_PACKAGE( BWOpenSSL REQUIRED )
IF( BWOPENSSL_FOUND )
	ADD_DEFINITIONS( -DUSE_OPENSSL )
	INCLUDE_DIRECTORIES( ${BWOPENSSL_INCLUDE_DIRS} )
	BW_TARGET_LINK_LIBRARIES( network INTERFACE ${BWOPENSSL_LIBRARIES} )
ENDIF()

BW_TARGET_LINK_LIBRARIES( network INTERFACE
	cstdmf
	math
	# third party
	zip
	)
IF( BW_PLATFORM_WINDOWS )
	BW_TARGET_LINK_LIBRARIES( network INTERFACE
		# external libraries
		ws2_32
		)
ENDIF()

BW_PRECOMPILED_HEADER( network pch.hpp )
BW_PROJECT_CATEGORY( network "BigWorld Libs" )
