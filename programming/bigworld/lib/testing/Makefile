LIB = testing

SRCS =							\
	baseapp_message_filter 		\
	mangle						\
	message_filter				\

ifndef MF_ROOT
export MF_ROOT := $(subst /src/lib/$(LIB),,$(CURDIR))
endif

include $(MF_ROOT)/bigworld/src/server/common/common.mak

all::

