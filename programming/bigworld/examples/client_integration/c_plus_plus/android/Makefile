# This Makefile generates .hpp and .cpp files for
# entities off entity definitions then runs ndk-build.
# Run 'make generated' to build generated sources only.

.PHONY : all generated clean

ifndef BW_ROOT
export BW_ROOT := $(subst programming/bigworld/examples/client_integration/c_plus_plus/android,,$(CURDIR))
endif


# set NDK_ROOT environment variable if you want to point to a 
# non-default NDK installation
NDK_BIN := ndk-build
NDK_BUILD := $(if $(NDK_ROOT),$(NDK_ROOT)/$(NDK_BIN),$(NDK_BIN))

export PROJECT_RES := $(BW_ROOT)/game/res/fantasydemo

ENTITY_DEFS = $(PROJECT_RES)/scripts/entities.xml \
	$(PROJECT_RES)/scripts/entity_defs/alias.xml \
	$(wildcard $(PROJECT_RES)/scripts/entity_defs/*.def) \
	$(wildcard $(PROJECT_RES)/scripts/entity_defs/interfaces/*.def)


# ---------------------------------------------------------------------
# Here we reporduce what happens in programming/bigworld/build/make/Makefile
# to gain proper path to compiled process_defs
# TODO: extract these variables and calculations from
# programming/bigworld/build/make/Makefile so that they can be reused here

BW_CONFIG_HYBRID        := hybrid
BW_CONFIG_DEBUG         := debug

BW_RES_BIGWORLD := $(BW_ROOT)/game/res/bigworld
BW_SRC := $(BW_ROOT)/programming/bigworld

# We make use of BW_ROOT and BW_CONFIG_* within the Makefile.user
-include $(BW_SRC)/Makefile.user

BW_BLDDIR := $(BW_SRC)/build/make

ifdef BW_INSTALL_DIR
BW_INSTALL_DIR := $(BW_INSTALL_DIR)/game
else
BW_INSTALL_DIR := $(BW_ROOT)/game
endif

BW_PLATFORM_PY := $(BW_BLDDIR)/platform_info.py
ifndef BW_HOST_PLATFORM
BW_HOST_PLATFORM := $(shell $(BW_PLATFORM_PY))
endif

# If BW_HOST_PLATFORM is empty, the script has possibly failed to run.
ifeq ($(BW_HOST_PLATFORM),)
$(error Unable to determine BW_HOST_PLATFORM using $(BW_PLATFORM_PY))
endif

BW_HOST_PLATFORM_CONFIG := $(BW_HOST_PLATFORM)

# Paths that are required to be different when building with a different
# host platform
BW_HOST_SERVER_TOOLS_BIN_DIR := bin/server/$(BW_HOST_PLATFORM_CONFIG)/tools


# This comes from common_footer_config.mak
BW_BINDIR := $(BW_INSTALL_DIR)
# ---------------------------------------------------------------------


PROCESS_DEFS_BLD_DIR = $(BW_BINDIR)/$(BW_HOST_SERVER_TOOLS_BIN_DIR)
BW_RES_BIGWORLD_BLD = $(BW_INSTALL_DIR)/res/bigworld
PROCESS_DEFS_SRC_DIR = $(BW_SRC)/tools/process_defs
PROCESS_DEFS_RES_DIR = $(BW_ROOT)/game/bin/tools/process_defs/resources/scripts
PROCESS_DEFS = $(abspath $(PROCESS_DEFS_BLD_DIR)/process_defs)

GENERATED_DIR = jni/GeneratedSource

# Sets $(generatedSource)
-include $(GENERATED_DIR)/Makefile.rules
GENERATED_SOURCES = $(addsuffix .cpp, $(addprefix $(GENERATED_DIR)/, $(generatedSource)))

all : generated 
	+$(NDK_BUILD)

generated : $(GENERATED_DIR)/Makefile.rules $(GENERATED_SOURCES)

clean :
	-rm -rf $(GENERATED_DIR)
	-rm -rf obj
	-rm -rf libs

	
$(PROCESS_DEFS) : 
	$(MAKE) -C $(PROCESS_DEFS_SRC_DIR)
	$(MAKE) -C $(PROCESS_DEFS_SRC_DIR) python_library

# Res paths for process_defs:
# Project
# Generated/installed BigWorld resources
# Static BigWorld resources
$(GENERATED_DIR)/Makefile.rules : $(ENTITY_DEFS) | $(PROCESS_DEFS)
	$(PROCESS_DEFS)									\
	-r $(PROJECT_RES)	 							\
	-r $(BW_RES_BIGWORLD_BLD)						\
	-r $(BW_RES_BIGWORLD)							\
	-p $(PROCESS_DEFS_RES_DIR)						\
	-m GenerateCPlusPlus							\
	--output "$(CURDIR)/$(GENERATED_DIR)"
