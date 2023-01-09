# This file performs basic initialisation and sanity checks of a component
# Makefile.rules file. It does some basic setup based on the component type
# which is then used in the $(BW_COMPONENT_FOOTER)
#
# This file should be included at the start of a component Makefile.rules
# after defining the component name / type. For example:
#
#  name := cstdmf
#  type := library
#
#  include $(BW_COMPONENT_HEADER)
#


# Enforce that the basic set of variables have been defined
ifeq ($(origin name),undefined)
$(error Component name has not been defined)
endif
ifeq ($(origin type),undefined)
$(error Component type has not been defined)
endif

ifeq ($(type),binary)
 ifneq ($(words $(bindir)),1)
bindir := unknown
 endif
endif

absIncFromDir ?=
ifeq ($(absIncFromDir),)
# This bit of magic is to find the directory of the Makefile that included us.
# From the inner-most out we read this as:
# * Get the hierachy of Makefile includes leading to here  - $(MAKEFILE_LIST)
# * Get the current Makefile path                          - $(lastword ...)
# * Strip the last Makefile path from the include chain    - $(filter-out ...)
# * Return the second last Makefile from the include chain - $(lastword ...)
# * What is the directory of the second last Makefile      - $(dir ...)
# * Remove the trailing / on the directory                 - $(patsubst %/,%,...)
# * Resolve the absolute path                              - $(abspath ...)
absIncFromDir := $(abspath $(patsubst %/,%,$(dir $(lastword $(filter-out $(lastword $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))))
endif

# * Remove the leading $(BW_ABS_ROOT)                      - $(patsubst $(BW_ABS_ROOT
incFromDir := $(patsubst $(BW_ABS_ROOT)/%,%,$(absIncFromDir))

BW_LAST_COMPONENT_INCLUDE_DIR := $(incFromDir)

# If we're not a unit test and $(bw_workAroundIncludeFromDir) is defined, then
# use that as the incFromDir instead, and reattach $(BW_ABS_ROOT).
ifdef bw_workAroundIncludeFromDir
 ifneq ($(call isUnitTest,$(type)),1)
incFromDir := $(bw_workAroundIncludeFromDir)
absIncFromDir := $(BW_ABS_ROOT)/$(incFromDir)
BW_LAST_COMPONENT_INCLUDE_DIR := $(bw_workAroundIncludeFromDir)
bw_workAroundIncludeFromDir :=
 endif
endif

# --- Binary setup
ifeq ($(type),binary)

binName      := $(name)
dirOfSource  := $(incFromDir)
pathToSource := $(absIncFromDir)

# These are finalised in common_footer_config.mak
tmpDestFile  := $(binName)
tmpObjDir    := $(dirOfSource)


# --- Copy of a Binary setup
else ifeq ($(type),copy_of_binary)

binName      := $(name)

# These are finalised in common_footer_config.mak
tmpDestFile  := $(binName)
tmpObjDir    := $(dirOfSource)


# --- Unit test setup
else ifeq ($(type),unit_test)

# unit tests are a variation of binaries
binName      := unit_test_$(name)
dirOfSource  := $(incFromDir)
pathToSource := $(absIncFromDir)

# These are finalised in common_footer_config.mak
tmpDestFile  := $(name)_test
tmpObjDir    := $(dirOfSource)


# --- Library setup
else ifeq ($(type),library)

binName      := lib$(name)
dirOfSource  := $(incFromDir)
pathToSource := $(absIncFromDir)


# --- Shared library setup
else ifeq ($(type),shared_library)

binName      := $(name)
dirOfSource  := $(incFromDir)
pathToSource := $(absIncFromDir)

# These are finalised in common_footer_config.mak
tmpDestFile  := $(binName).so
tmpObjDir    := $(dirOfSource)


# --- Copy of a Shared library setup
else ifeq ($(type),copy_of_shared_library)

binName      := $(name)
pathToSource := $(absIncFromDir)

# These are finalised in common_footer_config.mak
tmpDestFile  := $(binName).so
tmpObjDir    := $(dirOfSource)


# --- Unknown types
else

$(error Not sure how to build a $(type))

endif


useCurl    := 0
useOpenSSL := 0
usePython  := 0
useSystemPython := 0
usePyURLRequest := 0
useMySQL   := 0
useJSON    := 0
useSDL     := 0
useRT      := 0

useBigWorld := 0

ccFlags_$(binName)  :=
cxxFlags_$(binName) :=
cppFlags_$(binName) :=
incFlags_$(binName) :=
ldFlags_$(binName)  :=
ldLibs_$(binName)   :=

cSource :=
cxxSource :=
objectDirectories :=

buildTimeFile :=
dependsOn =
thirdPartyDependsOn :=
noDefaultDependencies := 0
noObjectDirectoryRule := 0
preBuildTargets :=
postBuildTargets :=
cleanTargets :=
onlyBuildConfig :=
unitTestRuntimeDependsOn :=
delayed_Component :=
exportFor :=

# Perform some basic dependency setup for certain build types, to help
# simplify the components Makefile.rules
ifeq ($(call isUnitTest,$(type)),1)

dependsOn += unit_test_lib CppUnitLite2

ldFlags_$(binName) += -rdynamic

endif

$(eval $(projectSpecificFlags))

# common_header.mak
