# Makefile added by BigWorld

CXXFLAGS += -Iinclude

LIB = jsoncpp

SRCS = 	src/lib_json/json_reader \
		src/lib_json/json_value	\
		src/lib_json/json_writer

ifndef MF_ROOT
export MF_ROOT := $(subst /bigworld/src/lib/third_party/jsoncpp,,$(CURDIR))
endif

all::

clean::
	rm -rf $(MF_CONFIG)

include $(MF_ROOT)/bigworld/src/build/common.mak


