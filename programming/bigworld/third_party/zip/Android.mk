LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := zip

LOCAL_SRC_FILES :=	adler32.c \
					compress.c \
					crc32.c \
					deflate.c \
					gzread.c \
					gzwrite.c \
					infback.c \
					inffast.c \
					inflate.c \
					inftrees.c \
					trees.c \
					uncompr.c \
					zutil.c \
					bw_zlib_mem.cpp 
					
LOCAL_CFLAGS += -Wno-deprecated -Wno-psabi -DMY_ZCALLOC

include $(BUILD_STATIC_LIBRARY)
