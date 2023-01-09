LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := connection

LOCAL_SRC_FILES :=	avatar_filter.cpp \
					avatar_filter_helper.cpp \
					baseapp_login_request_protocol.cpp \
					client_interface.cpp \
					condemned_interfaces.cpp \
					cuckoo_cycle_login_challenge_factory.cpp \
					data_download.cpp \
					filter_helper.cpp \
					log_on_params.cpp \
					login_challenge_factory.cpp \
					login_challenge_task.cpp \
					login_handler.cpp \
					login_interface.cpp \
					login_request.cpp \
					login_request_protocol.cpp \
					login_request_transport.cpp \
					loginapp_login_request_protocol.cpp \
					movement_filter.cpp \
					pch.cpp \
					replay_checksum_scheme.cpp \
					replay_controller.cpp \
					replay_data_file_reader.cpp \
					replay_header.cpp \
					replay_metadata.cpp \
					replay_tick_loader.cpp \
					server_connection.cpp \
					server_finder.cpp \
					smart_server_connection.cpp \
					

LOCAL_CFLAGS += -Wno-deprecated -Wno-psabi

LOCAL_C_INCLUDES := $(MY_INCLUDES)

LOCAL_STATIC_LIBRARIES := network

include $(BUILD_STATIC_LIBRARY)

$(call import-module,network)
