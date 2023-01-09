LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := math

LOCAL_SRC_FILES :=	boundbox.cpp \
					ema.cpp \
					linear_lut.cpp \
					math_extra.cpp \
					matrix.cpp \
					oriented_bbox.cpp \
					pch.cpp \
					perlin_noise.cpp \
					planeeq.cpp \
					polygon.cpp \
					polyhedron.cpp \
					portal2d.cpp \
					quat.cpp \
					simplex_noise.cpp \
					stat_watcher_creator.cpp \
					vector2.cpp \
					vector3.cpp \
					vector4.cpp

LOCAL_CFLAGS += -Wno-deprecated -Wno-psabi

LOCAL_C_INCLUDES := $(MY_INCLUDES)


include $(BUILD_STATIC_LIBRARY)
