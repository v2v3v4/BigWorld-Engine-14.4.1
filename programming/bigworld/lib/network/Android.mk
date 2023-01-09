LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := network

#MY_FILES := $(wildcard $(LOCAL_PATH)/*.cpp \)
#MY_FILES := $(MY_FILES:$(LOCAL_PATH)/%=%)
#LOCAL_SRC_FILES := $(MY_FILES)
LOCAL_SRC_FILES :=	address_resolver.cpp \
					basictypes.cpp \
					blocking_reply_handler.cpp \
					bsd_snprintf.cpp \
					bundle.cpp \
					bundle_sending_map.cpp \
					bw_message_forwarder.cpp \
					channel.cpp \
					channel_map.cpp \
					channel_owner.cpp \
					compression_stream.cpp \
					condemned_channels.cpp \
					deferred_bundle.cpp \
					delayed_channels.cpp \
					elliptic_curve_checksum_scheme.cpp \
					encryption_filter.cpp \
					encryption_stream_filter.cpp \
					endpoint.cpp \
					error_reporter.cpp \
					event_dispatcher.cpp \
					event_poller.cpp \
					exposed_message_range.cpp \
					file_stream.cpp \
					forwarding_reply_handler.cpp \
					forwarding_string_handler.cpp \
					fragmented_bundle.cpp \
					frequent_tasks.cpp \
					http_messages.cpp \
					interface_element.cpp \
					interface_minder.cpp \
					interface_table.cpp \
					irregular_channels.cpp \
					keepalive_channels.cpp \
					logger_message_forwarder.cpp \
					machined_utils.cpp \
					machine_guard.cpp \
					machine_guard_response_checker.cpp \
					monitored_channels.cpp \
					netmask.cpp \
					network_interface.cpp \
					network_stats.cpp \
					network_utils.cpp \
					once_off_packet.cpp \
					packet.cpp \
					packet_filter.cpp \
					packet_receiver.cpp \
					packet_receiver_stats.cpp \
					packet_loss_parameters.cpp \
					packet_sender.cpp \
					pch.cpp \
					process_socket_stats_helper.cpp \
					public_key_cipher.cpp \
					recently_dead_channels.cpp \
					request.cpp \
					request_manager.cpp \
					rescheduled_sender.cpp \
					sending_stats.cpp \
					sha_checksum_scheme.cpp \
					space_data_mapping.cpp \
					space_data_mappings.cpp \
					symmetric_block_cipher.cpp \
					tcp_bundle_processor.cpp \
					tcp_bundle.cpp \
					tcp_channel.cpp \
					tcp_connection_opener.cpp \
					udp_bundle_processor.cpp \
					udp_bundle.cpp \
					udp_channel.cpp \
					unacked_packet.cpp \
					unpacked_message_header.cpp \
					watcher_connection.cpp \
					watcher_nub.cpp \
					watcher_packet_handler.cpp \
					websocket_stream_filter.cpp \
					zip_stream.cpp
					
LOCAL_C_INCLUDES := $(MY_INCLUDES)

LOCAL_CFLAGS += -Wno-deprecated -Wno-psabi -Wno-write-strings

LOCAL_STATIC_LIBRARIES := cstdmf
LOCAL_STATIC_LIBRARIES += math
LOCAL_STATIC_LIBRARIES += zip
LOCAL_STATIC_LIBRARIES += libcrypto-static libssl-static

include $(BUILD_STATIC_LIBRARY)

$(call import-module,cstdmf)
$(call import-module,math)
$(call import-module,zip)
$(call import-module,openssl_android)

