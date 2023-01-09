#
# Platform group configuration.
#
# Platform groups are configuration options that affect a family of platforms,
# for example Red-Hat derived distributions.
#

BW_REDHAT_PLATFORMS := el5 el6 el7 fedora18 fedora19

ifeq ($(findstring $(BW_BUILD_PLATFORM),$(BW_REDHAT_PLATFORMS)),$(BW_BUILD_PLATFORM))
BW_BUILD_PLATFORM_GROUP := redhat
endif

ifneq ($(BW_BUILD_PLATFORM_GROUP),)
-include $(BW_BLDDIR)/platform_group_$(BW_BUILD_PLATFORM_GROUP).mak
endif

-include $(BW_BLDDIR)/platform_group_$(BW_HOST_PLATFORM)_host.mak
# build/make/platform_groups_config.mak
