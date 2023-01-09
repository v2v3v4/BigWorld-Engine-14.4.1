#
# Flags for platforms built on centos5 or rhel5 (i.e. el5-based systems)
# Using this instead of platform_el5.mak to handle platforms that could
# be compiled on el5 but not be the el5 platform, such as emscriptem
#
ifneq (,$(wildcard /usr/share/doc/bash-3.2/scripts/timeout))
	TIMEOUT_BIN :=  $(SHELL) /usr/share/doc/bash-3.2/scripts/timeout
endif

# build/make/platform_group_el5_host.mak
