LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := cstdmf

LOCAL_SRC_FILES :=	allocator.cpp \
					ansi_allocator.cpp \
					background_file_writer.cpp \
					base64.cpp \
					bgtask_manager.cpp \
					binary_stream.cpp \
					bit_reader.cpp \
					bit_writer.cpp \
					blob_or_null.cpp \
					bwrandom.cpp \
					bw_hash.cpp \
					bw_memory.cpp \
					bw_safe_allocatable.cpp \
					bw_string_ref.cpp \
					bw_util.cpp \
					bw_util_linux.cpp \
					bwversion.cpp \
					checksum_stream.cpp \
					concurrency.cpp \
					critical_error_handler.cpp \
					cstdmf_init.cpp \
					debug.cpp \
					debug_filter.cpp \
					dogwatch.cpp \
					dprintf.cpp \
					fini_job.cpp \
					intrusive_object.cpp \
					locale.cpp \
					log_meta_data.cpp \
					log_msg.cpp \
					md5.cpp \
					memory_stream.cpp \
					pch.cpp \
					profile.cpp \
					profiler.cpp \
					stack_tracker.cpp \
					string_builder.cpp \
					task_watcher.cpp \
					time_queue.cpp \
					timestamp.cpp \
					unique_id.cpp \
					watcher.cpp \
					watcher_path_request.cpp


LOCAL_C_INCLUDES := $(LOCAL_PATH)/..
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../third_party

LOCAL_CFLAGS += -Wno-deprecated -Wno-psabi

# Instantiate templates when building this library
LOCAL_CPPFLAGS := -UCSTDMF_IMPORT -DCSTDMF_EXPORT
# but library users should not instantiate the templates
LOCAL_EXPORT_CPPFLAGS := -UCSTDMF_EXPORT -DCSTDMF_IMPORT

include $(BUILD_STATIC_LIBRARY)
