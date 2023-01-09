LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := connection_model

LOCAL_SRC_FILES :=	active_entities.cpp \
					bw_connection.cpp \
					bw_connection_helper.cpp \
					bw_entities.cpp \
					bw_entity.cpp \
					bw_entity_factory.cpp \
					bw_null_connection.cpp \
					bw_replay_connection.cpp \
					bw_server_connection.cpp \
					bw_server_message_handler.cpp \
					deferred_passengers.cpp \
					entity_entry_blocker.cpp \
					entity_entry_blocking_condition_handler.cpp \
					entity_entry_blocking_condition_impl.cpp \
					entity_extension.cpp \
					entity_extension_factory_base.cpp \
					entity_extension_factory_manager.cpp \
					entity_extensions.cpp \
					nested_property_change.cpp \
					pch.cpp \
					pending_entities_blocking_condition_handler.cpp \
					pending_entities.cpp \
					server_entity_mail_box.cpp \
					simple_space_data_storage.cpp \

LOCAL_CFLAGS += -Wno-deprecated -Wno-psabi

LOCAL_C_INCLUDES := $(MY_INCLUDES)
LOCAL_C_INCLUDES += $(MY_LIB_PATH)/..

LOCAL_STATIC_LIBRARIES := connection

include $(BUILD_STATIC_LIBRARY)

$(call import-module,connection)
