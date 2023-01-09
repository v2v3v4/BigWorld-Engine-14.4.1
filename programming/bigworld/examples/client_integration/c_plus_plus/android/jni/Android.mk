# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

MY_LIB_PATH := $(LOCAL_PATH)/../../../../../lib
MY_THIRD_PARTY_PATH := $(LOCAL_PATH)/../../../../../third_party
# You may also use absolute path if BigWorld libs are in another tree
#MY_LIB_PATH := /home/user/2/current/programming/bigworld/lib
#MY_THIRD_PARTY_PATH := /home/user/2/current/programming/bigworld/third_party

MY_GENERATED_DIR = GeneratedSource

include jni/$(MY_GENERATED_DIR)/Makefile.rules
MY_GENERATED_SOURCES = $(addsuffix .cpp, $(addprefix $(MY_GENERATED_DIR)/, $(generatedSource)))

MY_OPENSSL_PATH := $(MY_THIRD_PARTY_PATH)/openssl_android

MY_INCLUDES := $(MY_LIB_PATH)
MY_INCLUDES += $(MY_THIRD_PARTY_PATH)
MY_INCLUDES += $(MY_OPENSSL_PATH)/include
MY_INCLUDES += $(LOCAL_PATH)/$(MY_GENERATED_DIR) 

MY_SRC_FILES := $(MY_GENERATED_SOURCES)

include jni/GameLogic/Makefile.rules

MY_SRC_FILES += $(addprefix GameLogic/, $(MY_LOGIC_SRC_FILES))

MY_SRC_FILES += app.cpp \
				game_logic_factory.cpp \
				main.cpp

include $(CLEAR_VARS)

LOCAL_MODULE    := native-activity

LOCAL_SRC_FILES := $(MY_SRC_FILES)

LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv1_CM
# Work around link issue with APP_STL in Android NDK r8b
LOCAL_LDLIBS	+= $(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/$(TOOLCHAIN_VERSION)/libs/$(TARGET_ARCH_ABI)/libgnustl_static.a

LOCAL_LDFLAGS := -Wl,-z,muldefs

LOCAL_C_INCLUDES := $(MY_INCLUDES)

LOCAL_CFLAGS += -Wno-deprecated -Wno-psabi

LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_STATIC_LIBRARIES += connection_model

#NDK_MODULE_PATH += /home/michaelb/2/current/bigworld/src/lib
#$(call __ndk_info,$(NDK_MODULE_PATH))

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path,$(MY_LIB_PATH))
$(call import-add-path,$(MY_THIRD_PARTY_PATH))
$(call import-module,connection_model)
$(call import-module,android/native_app_glue)
