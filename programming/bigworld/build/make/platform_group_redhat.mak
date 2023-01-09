#
# This file contains platform-specific options for RedHat-based platforms,
# both Fedora and Enterprise Linux
#

ARCHFLAGS 	= -m64 -fPIC -msse3

CC 			:= gcc
CXX 		:= g++
AR 			:= ar
RANLIB 		:= ranlib

CXXFLAGS 	+= -Werror

# add all symbols (including unused) to the dynamic symbol table
LDFLAGS 	+= -Wl,-export-dynamic

# default libraries used by all programs
LDLIBS 		+= -lm
LDLIBS 		+= -lpthread


ifeq ($(ENABLE_MEMTRACKER),1)
LDLIBS 		+= -lbfd -liberty
endif

# OpenSSL configure options
OPENSSL_CONFIG_TARGET  := linux-x86_64 
OPENSSL_CONFIG_FLAGS   := -fPIC -DOPENSSL_PIC

# build/make/platform_group_redhat.mak
