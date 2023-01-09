# Top level Makefile for the BigWorld Technology Server components

# Supported top level targets & descriptions
# all         - build all components of the source code
# everything  - build absolutely everything, even non-source stuff (eg: rpms)
# clean       - remove all the intermediate files

# Explanation of some non-obvious things:
# * Variables prefixed with 'delayed_' are intended to be evaluated at a later
#   stage of make execution. This is intended to help avoid confusion about
#   'why isn't this change working'.

# Known issues:
# - initial third party library builds may not like parallel jobs

BW_ROOT := $(abspath $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))/../../../..)

BW_CONFIG_HYBRID        := hybrid
BW_CONFIG_DEBUG         := debug

BW_SUFFIX_RES_BIGWORLD := res/bigworld
BW_SUFFIX_RES_FANTASYDEMO := res/fantasydemo

# Absolute path variables are used to avoid dependencies not matching when
# building from different levels in the src tree heirarchy.
BW_ABS_ROOT := $(BW_ROOT)
BW_RES_BIGWORLD := $(BW_ROOT)/game/$(BW_SUFFIX_RES_BIGWORLD)
BW_RES_FANTASYDEMO := $(BW_ROOT)/game/$(BW_SUFFIX_RES_FANTASYDEMO)

BW_SRC := $(BW_ROOT)/programming/bigworld
BW_ABS_SRC := $(abspath $(BW_SRC))
BW_ABS_SRC_LIB := $(BW_ABS_SRC)/lib
BW_ABS_SRC_THIRDPARTY := $(BW_ABS_SRC)/third_party
BW_ABS_SRC_SERVER := $(BW_ABS_SRC)/server

# We make use of BW_ROOT and BW_CONFIG_* within the Makefile.user
-include $(BW_SRC)/Makefile.user

BW_CONFIG_DEFAULT    := $(BW_CONFIG_HYBRID)
BW_STANDARD_CONFIGS  := $(BW_CONFIG_HYBRID) $(BW_CONFIG_DEBUG)

# MF_CONFIG is a fallback convenience and will supercede BW_CONFIG
ifdef MF_CONFIG
BW_CONFIG ?= $(MF_CONFIG)
endif

BW_CONFIG ?= $(BW_CONFIG_DEFAULT)

BW_HOST_CONFIG ?= $(BW_CONFIG)

# TODO: Add test for BW_CONFIG actually being a valid config

BW_BLDDIR := $(BW_SRC)/build/make
BW_INTERMEDIATE_DIR ?= $(BW_SRC)/build

# If BW_INSTALL_DIR is specified, we append /game to the end of it for the time
# being due to bwmachined assuming that binaries will live under 'game'. This
# should be removed in the future when bwmachined specifies a bin path directly.
ifdef BW_INSTALL_DIR
BW_INSTALL_DIR := $(BW_INSTALL_DIR)/game
else
BW_INSTALL_DIR := $(BW_ROOT)/game
endif

# The install destinations for the res tree specific files
BW_INSTALL_RES_BIGWORLD := $(BW_INSTALL_DIR)/$(BW_SUFFIX_RES_BIGWORLD)
BW_INSTALL_RES_FANTASYDEMO := $(BW_INSTALL_DIR)/$(BW_SUFFIX_RES_FANTASYDEMO)


# Platform/Distribution information used as part of the directory name for
# objects and binary files.
BW_PLATFORM_PY := $(BW_BLDDIR)/platform_info.py
ifndef BW_HOST_PLATFORM
BW_HOST_PLATFORM := $(shell $(BW_PLATFORM_PY))
endif

ifndef BW_BUILD_PLATFORM
BW_BUILD_PLATFORM := $(BW_HOST_PLATFORM)
endif

# If BW_BUILD_PLATFORM is empty, the script has possibly failed to run.
ifeq ($(BW_HOST_PLATFORM),)
$(error Unable to determine BW_HOST_PLATFORM using $(BW_PLATFORM_PY))
endif

# For 'hybrid' build config we don't want trailing _hybrid
ifeq ($(BW_CONFIG),$(BW_CONFIG_HYBRID))
BW_PLATFORM_CONFIG := $(BW_BUILD_PLATFORM)
else
BW_PLATFORM_CONFIG := $(BW_BUILD_PLATFORM)_$(BW_CONFIG)
endif

ifeq ($(BW_HOST_CONFIG),$(BW_CONFIG_HYBRID))
BW_HOST_PLATFORM_CONFIG := $(BW_HOST_PLATFORM)
else
BW_HOST_PLATFORM_CONFIG := $(BW_HOST_PLATFORM)_$(BW_HOST_CONFIG)
endif

BW_STD_CPP_UNIT_TEST_DIR := unit_test

# BW_THIS_DIR is a bit of magic to help us determine the directory of the
# currently included Makefile. It is used to assist with bulk include
# operations.
BW_THIS_DIR = $(subst $(BW_ABS_ROOT)/,,$(abspath $(dir $(lastword $(MAKEFILE_LIST)))))

BW_INVOKE_WITHOUT_JOBSERVER_PY := $(BW_BLDDIR)/invoke_without_jobserver.py
MAKE_WITHOUT_JOBSERVER := $(BW_INVOKE_WITHOUT_JOBSERVER_PY) --make $(MAKE) --

# TODO: This should be potentially better defined either as a chain through the
#       libraries, or by the processes
# Standard libraries that all binaries link to
BW_DEPENDS_ON := network resmgr math cstdmf zip build

#bwt: bw-server bw-server-tools bw-server

# This will eventually contain all the libraries
BW_LIBS :=
# This will eventually contain all the binary names
BW_BINS :=
# This will eventually contain all the third party library names
BW_THIRD_PARTY :=
# This will eventually contain any misc programs or BigWorld components
BW_MISC :=
# This will eventually contain all the unit test rules
BW_UNIT_TESTS :=
# This will eventually contain all the unit test files to run
BW_RUN_UNIT_TESTS :=
# This will eventually contain all the rules to run unit test
BW_RUN_UNIT_TEST_RULES :=
# This will eventually contain all the unit test binary path and source
# directories in the form of binary_path:source_dirctory
BW_UNIT_TEST_BIN_SRC :=
# This will eventually contain any 'extra' rules to be called when cleaning
BW_CLEAN_HOOKS :=
# This will eventually contain any 'extra' rules to be called when doing veryclean
BW_VERY_CLEAN_HOOKS :=
# This will eventually contain all the rules to build baseapp extensions
BW_BASEAPP_EXTENSIONS :=
# This will eventually contain all the rules to build cellapp extensions
BW_CELLAPP_EXTENSIONS :=

# These tests determine whether the user has specified to always build silently
# either via the 'make -s' command, or with the 'user_alwaysSilent' variable
hasSilentFlag := $(findstring s, $(MAKEFLAGS))
func_trueOrEmpty = $(findstring 1, $(1))
BW_IS_QUIET_BUILD := $(if $(or $(call func_trueOrEmpty, $(user_alwaysSilent)), $(hasSilentFlag)),1,0)



# This assignment must remain before the first rule is defined. We check to see
# if a .DEFAULT_GOAL has been supplied, which will only occur if we are
# building from a subdirectory.
#
# If that is the case, we store this .DEFAULT_GOAL name as our default
# component, so if a make command is supplied (eg: make clean) we can perform
# that operation directly on the component.
BW_IN_COMPONENT := $(.DEFAULT_GOAL)


depsForRuleAll :=			\
	bw-libraries			\
	bw-binaries				\
	third-party-libraries	\
	bw-miscellaneous

# Unit tests can be excluded from the regular build cycle
ifneq ($(user_shouldBuildUnitTests),0)
depsForRuleAll += bw-unit-tests

 # By default don't run the unit tests.
 ifeq ($(user_shouldRunUnitTests),1)
depsForRuleAll += bw-run-unit-tests
 endif

endif

.PHONY: all
all: $(depsForRuleAll)

# general flags
BW_INCLUDES := -I$(BW_ABS_SRC_LIB) -I$(BW_ABS_SRC_THIRDPARTY)

# ar flags
ifneq ($(BW_IS_QUIET_BUILD),1)
ARFLAGS = rcsuv
else
ARFLAGS = rcsu
endif

#Note: BW_THIRD_PARTY_INSTALL_DIR is used by platform_xxx.mak, it must be defined
#before include platform specific .mak files.
BW_THIRD_PARTY_INSTALL_DIR := bin/server/$(BW_HOST_PLATFORM_CONFIG)/third_party

include $(BW_BLDDIR)/platform_common.mak
# Import platform-group-specific (e.g. RedHat) or platform-specific .mak to
# override/augment any of the above options.
-include $(BW_BLDDIR)/platform_groups_config.mak
-include $(BW_BLDDIR)/platform_$(BW_BUILD_PLATFORM).mak

# c pre-processor
CPPFLAGS += $(DEPFLAGS)

# This is used to prevent MF_SERVER from being defined. This is used by the
# example clients when building under Linux.
shouldDefineMFServer ?= 1
ifneq ($(shouldDefineMFServer),0)
BW_CPPFLAGS += -DMF_SERVER
endif

BW_CPPFLAGS += -DMF_CONFIG=\"$(BW_CONFIG)\"
BW_CPPFLAGS += -DBW_BUILD_PLATFORM=\"$(BW_BUILD_PLATFORM)\"

ifeq ($(ENABLE_MEMTRACKER),1)
BW_CPPFLAGS += -DENABLE_MEMTRACKER -DUSE_MEMHOOK
endif

ifeq ($(ENABLE_PROTECTED_ALLOCATOR),1)
BW_CPPFLAGS += -DENABLE_PROTECTED_ALLOCATOR -DUSE_MEMHOOK
endif

BW_CXXFLAGS := 

BW_CSTDMF_LINKAGE_EXPORT := CSTDMF_EXPORT
BW_CSTDMF_LINKAGE_IMPORT := CSTDMF_IMPORT
BW_BWENTITY_LINKAGE_EXPORT := BWENTITY_DLL_EXPORT
BW_BWENTITY_LINKAGE_IMPORT := BWENTITY_DLL_IMPORT
 
CXXFLAGS := $(ARCHFLAGS) $(CXXFLAGS)

# gcc flags
CFLAGS   := $(ARCHFLAGS) $(CFLAGS)

# ld flags
LDFLAGS += -L$(BW_LIBDIR)

# Commonly used installation paths
BW_UNIT_TEST_DIR := bin/server/$(BW_PLATFORM_CONFIG)/unit_tests
BW_SERVER_BIN_DIR  := bin/server/$(BW_PLATFORM_CONFIG)/server
BW_SERVER_COMPONENT_DIR = $(BW_SERVER_BIN_DIR)/$(component)-extensions
BW_COMMAND_BIN_DIR := $(BW_SERVER_BIN_DIR)/commands
BW_COMMAND_HELPER_BIN_DIR := $(BW_COMMAND_BIN_DIR)/_helpers
BW_SERVER_TOOLS_BIN_DIR := bin/server/$(BW_PLATFORM_CONFIG)/tools
BW_SERVER_EXAMPLES_DIR := bin/server/$(BW_PLATFORM_CONFIG)/examples

# Paths that are required to be different when building with a different
# host platform
BW_HOST_SERVER_TOOLS_BIN_DIR := bin/server/$(BW_HOST_PLATFORM_CONFIG)/tools

# Commonly included BigWorld makefiles
BW_COMPONENT_HEADER := $(BW_BLDDIR)/common_header.mak
BW_COMPONENT_FOOTER := $(BW_BLDDIR)/common_footer.mak
BW_COMPONENT_CONFIG_FOOTER := $(BW_BLDDIR)/common_footer_config.mak


# Functions to be used throughout the Makefiles
isTypeFunction = $(if $(filter $(1),$(2)),1,)
isBinary = $(call isTypeFunction,binary,$(1))
isCopyOfBinary = $(or $(call isTypeFunction,copy_of_binary,$(1)), $(call isTypeFunction,copy_of_shared_library,$(1)))
isUnitTest = $(call isTypeFunction,unit_test,$(1))
isLibrary = $(call isTypeFunction,library,$(1))
isSharedLibrary = $(call isTypeFunction,shared_library,$(1))
isBinaryOrCopy = $(or $(call isBinary,$(1)),$(call isCopyOfBinary,$(1)))
isBinaryOrUnitTest = $(or $(call isBinary,$(1)), $(call isUnitTest,$(1)))

shouldLinkToDependencies = $(if $(or $(call isBinaryOrUnitTest,$(type)), $(call isSharedLibrary,$(type))),1,0)

setBwConfigFunc = $(eval delayed_BW_CONFIG := $(1))
includeFooterForConfig = $(call setBwConfigFunc,$(1))$(eval include $(BW_COMPONENT_CONFIG_FOOTER))
setBwComponentFunc = $(eval delayed_Component := $(1))
includeFooterForComponent = $(call setBwComponentFunc,$(1))$(eval include $(BW_COMPONENT_CONFIG_FOOTER))

getConfigsToBuild = $(error called old 'getConfigsToBuild')

includeMakefilesFrom = $(eval include $(addprefix $(dir $(lastword $(MAKEFILE_LIST))),$(1:=/Makefile.rules)))

define unitTestIncludeWorkAround
bw_workAroundIncludeFromDir := $$(patsubst $$(BW_ABS_ROOT)/%,%,$$(abspath $$(patsubst %/,%,$$(dir $$(lastword $$(MAKEFILE_LIST))))))
endef

includeUnitTestDirectory = $(eval $(unitTestIncludeWorkAround)) $(eval include $(BW_ROOT)/$(bw_workAroundIncludeFromDir)/$(BW_STD_CPP_UNIT_TEST_DIR)/Makefile.rules)

removeDuplicates = $(sort $(1))

hostSubCommand = BW_INSTALL_DIR=$(abspath $(BW_INSTALL_DIR)/..) BW_CONFIG=$(BW_HOST_CONFIG) $(1)
targetSubCommand = BW_INSTALL_DIR=$(abspath $(BW_INSTALL_DIR)/..) BW_CONFIG=$(BW_CONFIG) $(1)

hostSubBuild = $(call hostSubCommand, $(MAKE) -$(MAKEFLAGS) -C $(1) $(2))
targetSubBuild = $(call targetSubCommand, $(MAKE) -$(MAKEFLAGS) -C $(1) $(2))

BW_INTERMEDIATE_DIRS :=

# Rules for how to create the top level build directories for all configs
bwIntermediateDirs  = $(BW_INTERMEDIATE_DIR)/$(BW_PLATFORM_CONFIG)/obj
bwIntermediateDirs += $(BW_INTERMEDIATE_DIR)/$(BW_PLATFORM_CONFIG)/lib
bwIntermediateDirs += $(BW_INTERMEDIATE_DIR)/$(BW_PLATFORM_CONFIG)/dep
bwIntermediateDirs += $(BW_INSTALL_DIR)


BW_SYSTEM_PYTHON_COMPONENTS =	\
	bwlog						\
	network						\
	pyscript					\
	resmgr						\
	server						\
	math						\
	zip							\
	cstdmf

# Commands for how to compile and link. This has been separated into its own
# file as there is wrapper code to handle the BW_IS_QUIET_BUILD mode.
include $(BW_BLDDIR)/commands.mak


####
# Third Party: START
####

# Prepare the third party build dir, using a local (symlink and exec-friendly)
# dir if specified.
BW_THIRD_PARTY_BUILD_DIR := $(BW_INTERMEDIATE_DIR)/$(BW_PLATFORM_CONFIG)/third_party

ifdef BW_LOCAL_DIR
	BW_THIRD_PARTY_BUILD_DIR := $(BW_LOCAL_DIR)/$(BW_PLATFORM_CONFIG)/third_party
endif

# As a bit of a quirk, because we need to depend on the third party libraries
# from within other components, and the target names are defined inside their
# individual third_party_*.mak files, we will include them first.

# TODO: move these into a rule that is included through
#  src/lib -> src/lib/third_party
include $(BW_BLDDIR)/third_party_openssl.mak
include $(BW_BLDDIR)/third_party_curl.mak
include $(BW_BLDDIR)/third_party_json.mak
include $(BW_BLDDIR)/third_party_mysql.mak
include $(BW_BLDDIR)/third_party_python.mak
include $(BW_BLDDIR)/third_party_python_modules.mak
include $(BW_BLDDIR)/third_party_sdl.mak
include $(BW_BLDDIR)/third_party_mongodb.mak

####
# Third Party: END
####

# Plugins
include $(wildcard $(BW_ROOT)/programming/*/build/make/plugin_setup.mak)

include $(BW_ROOT)/programming/Makefile.rules

# Targets used to consolidate components
.PHONY: bw-libraries bw-binaries third-party-libraries bw-miscellaneous
.PHONY: clean_bw-libraries clean_bw-binaries clean_bw-unit-tests
bw-libraries: $(call removeDuplicates,$(BW_LIBS))
clean_bw-libraries: $(addprefix clean_,$(call removeDuplicates,$(BW_LIBS)))
bw-binaries: $(call removeDuplicates,$(BW_BINS))
clean_bw-binaries: $(addprefix clean_,$(call removeDuplicates,$(BW_BINS)))
third-party-libraries: $(call removeDuplicates,$(BW_THIRD_PARTY))
bw-miscellaneous: $(call removeDuplicates,$(BW_MISC))
bw-unit-tests: $(call removeDuplicates,$(BW_UNIT_TESTS))
clean_bw-unit-tests: $(addprefix clean_,$(call removeDuplicates,$(BW_UNIT_TESTS)))
clean_bw-hooks: $(BW_CLEAN_HOOKS)
veryclean_bw-hooks: $(BW_VERY_CLEAN_HOOKS)

# Unit Tests: run all of the unit tests and don't stop at failures but instead 
# ouput failed modules upon completion 
.PHONY: bw-run-all-unit-tests
bw-run-all-unit-tests: $(call removeDuplicates,$(BW_RUN_UNIT_TESTS))
	@STATUS=0; \
	FAILED_TESTS=""; \
	for bin_path_src_dir in $(call removeDuplicates,$(BW_UNIT_TEST_BIN_SRC)); \
	do \
		bin_path=$${bin_path_src_dir%:*}; \
		src_dir=$${bin_path_src_dir$##*:}; \
		cd $$src_dir; \
		$(TIMEOUT_BIN) 1800 $$bin_path --root $(BW_ROOT) || FAILED_TESTS="$$FAILED_TESTS $$bin_path" \
		STATUS=`expr $$STATUS + $$?`; \
		echo $$bin_path result $$STATUS; \
		if [ "$(UNIT_TEST_STOP_AT_FAILURE)" == "1" -a $$STATUS -gt 0 ]; then \
			exit $$STATUS; \
		fi; \
	done; \
	if [ $$STATUS -gt 0 ]; then \
		echo "Unit test failed. Failed modules: $$FAILED_TESTS"; \
	fi; \
	test $$STATUS -eq 0


# Unit Tests: run all of the unit test and stop at the first failure
.PHONY: bw-run-unit-tests
bw-run-unit-tests: UNIT_TEST_STOP_AT_FAILURE=1
bw-run-unit-tests: bw-run-all-unit-tests
	


# Intermediate output file directories
bwIntermediateDirs += $(call removeDuplicates,$(BW_INTERMEDIATE_DIRS))
$(bwIntermediateDirs):
	@mkdir -p $@


# List all the rules that may be of interest to build
define newline


endef
empty :=
space := $(empty) $(empty)
displayRuleset = $(info $(1) $(subst $(space),$(newline) ,$(sort $(2))))
displayRules = $(call displayRuleset,Unit Tests$(newline),$(BW_UNIT_TESTS))$(info $(newline))$(call displayRuleset,Third Party Libraries$(newline),$(BW_THIRD_PARTY))$(info $(newline))$(call displayRuleset,Libraries$(newline),$(BW_LIBS))$(info $(newline))$(call displayRuleset,Binaries$(newline),$(BW_BINS))

#.PHONY: listrules
#listrules: $(call displayRules)
#	@/bin/true



#
# Clean
#

bwActualClean := clean_allcomponents

ifneq ($(BW_IN_COMPONENT),)
bwActualClean := clean_$(BW_IN_COMPONENT)
endif

.PHONY: clean
clean: $(bwActualClean)
	@echo "All object files and binaries removed"

.PHONY: clean_allcomponents
clean_allcomponents: bwConfigPaths := $(BW_INTERMEDIATE_DIR)/$(BW_CONFIG)
clean_allcomponents: clean_bw-binaries clean_bw-libraries clean_bw-unit-tests clean_bw-hooks
	@rm -rf $(bwConfigPaths)

ifneq ($(BW_IN_COMPONENT),)
.PHONY: cleandeps
cleandeps: cleandeps_$(BW_IN_COMPONENT)
endif


#
# Very Clean - (third party libraries are cleared as well)
#
#
veryCleanTargets := $(addprefix clean_,$(BW_THIRD_PARTY))

.PHONY: veryclean
veryclean: clean_allcomponents veryclean_bw-hooks $(veryCleanTargets)
	@echo "Additionally cleaned all third-party libraries"



#
# Old targets - backwards compatibility
#
.PHONY: all_configs all_configs_clean
.PHONY: cleanall

all_configs: all
all_configs_clean: clean
cleanall: veryclean

# TODO: old rules to support
# unit_tests unit_tests_clean unit_tests_run:
# all_configs_test:
# coverage:
# coverage_clean:
# cleanall:


# Makefile
