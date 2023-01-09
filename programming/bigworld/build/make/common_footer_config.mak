# This file contains rules that are used for defining different targets for
# each build configuration (ie: Hybrid64, Debug64, ...)
#
# This file should not be included by components, it should only be used by
# common_footer.mak which is the driver for which configurations to build.


###
# Stage 1
#
# Define the commonly used variables based off delayed_BW_CONFIG changes.
#

BW_OBJDIR := $(BW_INTERMEDIATE_DIR)/$(BW_PLATFORM_CONFIG)/obj
BW_LIBDIR := $(BW_INTERMEDIATE_DIR)/$(BW_PLATFORM_CONFIG)/lib
BW_META_DEPDIR := $(BW_INTERMEDIATE_DIR)/$(BW_PLATFORM_CONFIG)/dep
BW_BINDIR := $(BW_INSTALL_DIR)

ifeq ($(delayed_Component),)
componentSuffix :=
else
componentSuffix := _$(delayed_Component)
endif

bwConfBinName := $(BW_CONFIG)_$(binName)$(componentSuffix)

ccFlags_$(bwConfBinName)  :=
cxxFlags_$(bwConfBinName) :=
cppFlags_$(bwConfBinName) :=
incFlags_$(bwConfBinName) :=
ldFlags_$(bwConfBinName)  :=
ldLibs_$(bwConfBinName)   :=

# --- Binary setup
ifeq ($(type),binary)

destDir      := $(BW_BINDIR)/$(bindir)
destFile     := $(abspath $(destDir)/$(tmpDestFile))
outputObjDir := $(BW_OBJDIR)/$(tmpObjDir)

BW_INTERMEDIATE_DIRS += $(destDir)

# --- Copy of a Binary setup
else ifeq ($(type),copy_of_binary)

destDir      := $(BW_BINDIR)/$(bindir)
destFile     := $(abspath $(destDir)/$(tmpDestFile))
sourceDir    := $(BW_BINDIR)/$(sourceBinaryBinDir)
sourceFile   := $(abspath $(sourceDir)/$(sourceBinaryName))

# --- Unit test setup
else ifeq ($(type),unit_test)

noDefaultDependencies := 1

# unlike binaries which can specify where they are built to, we
# will always put unit tests in the same location.
destDir      := $(BW_BINDIR)/$(BW_UNIT_TEST_DIR)
destFile     := $(abspath $(destDir)/$(tmpDestFile)$(componentSuffix))
outputObjDir := $(BW_OBJDIR)/$(dirOfSource)$(componentSuffix)

BW_INTERMEDIATE_DIRS += $(destDir)

# --- Library setup
else ifeq ($(type),library)

destFile     := $(BW_LIBDIR)/$(binName)$(componentSuffix).a
outputObjDir := $(BW_OBJDIR)/$(dirOfSource)$(componentSuffix)

# --- Shared library setup
else ifeq ($(type),shared_library)

noDefaultDependencies := 1

destDir      := $(BW_BINDIR)/$(bindir)
destFile     := $(abspath $(destDir)/$(tmpDestFile))
outputObjDir := $(BW_OBJDIR)/$(tmpObjDir)

BW_INTERMEDIATE_DIRS += $(destDir)

# --- Copy of a Shared library setup
else ifeq ($(type),copy_of_shared_library)

destDir      := $(BW_BINDIR)/$(bindir)
destFile     := $(abspath $(destDir)/$(tmpDestFile))
sourceDir    := $(BW_BINDIR)/$(sourceBinaryBinDir)
sourceFile   := $(abspath $(sourceDir)/$(sourceBinaryName))

BW_INTERMEDIATE_DIRS += $(destDir)

# --- Unknown types
else

$(error Not sure how to build a $(type))

endif

# TODO: everything contained within this conditional could potentially be moved
#       into a compilation specific makefile (eg: common_footer_compile.mak)
ifneq ($(call isCopyOfBinary,$(type)),1)

###
# Stage 2
#
# Turn all the source code definitions from the component into .o and .d
# files to be used for rule definitions later.
#

objectDirectories := $(outputObjDir)

allComponentSource := $(cSource) $(cxxSource)

ifneq ($(delayed_Component),)
allComponentSource += $(cxxSource_$(delayed_Component))
cxxSource_$(delayed_Component) :=
endif

# This rule adds the directory of any required source file that is outside the
# directory of the currently specified component.
#
# From the inner-most out we read this as:
# * The list of all files required for this component  - $(allComponentSource)
# * Get the directory component of all files           - $(dir ...)
# * Remove any 'current directory' references          - $(patsubst ./,, ...)
# * Remove any surrounding whitespace                  - $(strip ...)
# * Prepend all remaining directories with the objdir  - $(addprefix ...)
# * Sort the list to remove any duplicate directories  - $(sort ...)
objectDirectories += $(sort $(addprefix $(outputObjDir)/,$(strip $(patsubst ./,,$(dir $(allComponentSource))))))

#componentBaseFiles   := $(abspath $(addprefix $(outputObjDir)/,$(allComponentSource)))
#componentObjects      = $(abspath $(componentBaseFiles:=.o))
#componentDependencies = $(abspath $(componentBaseFiles:=.d))
componentBaseFiles   := $(addprefix $(outputObjDir)/,$(allComponentSource))
componentObjects      = $(componentBaseFiles:=.o)
componentDependencies = $(componentBaseFiles:=.d)


###
# Stage 3
#
# If the component has specified that it uses another component, configure
# any compilation / linking flags now.
#

# Deal with the specific third party components
ifeq ($(useCurl),1)
incFlags_$(bwConfBinName) += -I$(CURL_BUILD_DIR)/include/curl
incFlags_$(bwConfBinName) += -I$(CURL_DIR)/include

thirdPartyDependsOn += $(BW_CURL_TARGET)

useRT = 1
useOpenSSL = 1
endif

ifeq ($(useRT),1)
ldLibs_$(bwConfBinName)   += -lrt
endif

ifeq ($(useJSON),1)
incFlags_$(bwConfBinName) += -I$(JSON_DIR)/include
cxxFlags_$(bwConfBinName) += -Wno-float-equal

thirdPartyDependsOn += $(BW_JSON_TARGET)
endif

ifeq ($(usePython), 1)

# All usage of Python requires the SCRIPT_PYTHON define as it controls how
# Python is #included from the BW source code.
cppFlags_$(bwConfBinName) += -DSCRIPT_PYTHON

 ifeq ($(useSystemPython), 1)
incFlags_$(bwConfBinName) += $(SYSTEM_PYTHON_INCLUDES)
ldFlags_$(bwConfBinName)  += $(SYSTEM_PYTHON_LDFLAGS)
 else
incFlags_$(bwConfBinName) += -I$(PYTHON_DIR)/Include
incFlags_$(bwConfBinName) += -I$(PYTHON_BUILD_DIR)
ldFlags_$(bwConfBinName)  += $(BW_PYTHON_LDFLAGS)
ldLibs_$(bwConfBinName)   += -pthread -lutil

thirdPartyDependsOn += $(BW_PYTHON_TARGET)
useOpenSSL = 1
 endif
endif

ifeq ($(useOpenSSL), 1)
incFlags_$(bwConfBinName) += -I$(OPENSSL_BUILD_DIR)/include
cppFlags_$(bwConfBinName) += -DUSE_OPENSSL
ldLibs_$(bwConfBinName)   += -ldl

thirdPartyDependsOn += $(BW_OPENSSL_SSL_TARGET) $(BW_OPENSSL_CRYPTO_TARGET)
endif

ifeq ($(useMySQL),1)
incFlags_$(bwConfBinName) += $(SYSTEM_MYSQL_INCLUDES)
ldLibs_$(bwConfBinName)   += $(SYSTEM_MYSQL_LDLIBS)
endif

ifeq ($(useSDL),1)
cxxFlags_$(bwConfBinName) += $(SYSTEM_SDL_CFLAGS)
ldLibs_$(bwConfBinName)   += $(SYSTEM_SDL_LDLIBS) -lSDL_image 
endif

ifeq ($(useBigWorld),1)
incFlags_$(bwConfBinName) += $(BW_INCLUDES)
cppFlags_$(bwConfBinName) += $(BW_CPPFLAGS)
cxxFlags_$(bwConfBinName) += $(BW_CXXFLAGS)
endif

ifeq ($(useMongoDB),1)
incFlags_$(bwConfBinName) += $(MONGO_CXX_DRIVER_INCLUDE)

# Flags to work around warnings in MongoDB/Boost header files.
cppFlags_$(bwConfBinName) += -Wno-ignored-qualifiers -Wno-unused-variable -Wno-unused-local-typedefs -Wno-float-equal
ldLibs_$(bwConfBinName)   += $(MONGO_CXX_DRIVER_LIBS)
endif

# Set up the appropriate import/export preprocessor macros
ifeq (cstdmf,$(findstring cstdmf,$(exportFor)))
cppFlags_$(bwConfBinName) += -D$(BW_CSTDMF_LINKAGE_EXPORT)
else
cppFlags_$(bwConfBinName) += -D$(BW_CSTDMF_LINKAGE_IMPORT)
endif

ifeq (bwentity,$(findstring bwentity,$(exportFor)))
cppFlags_$(bwConfBinName) += -D$(BW_BWENTITY_LINKAGE_EXPORT)
else
cppFlags_$(bwConfBinName) += -D$(BW_BWENTITY_LINKAGE_IMPORT)
endif


###
# Stage 4
#
# Generate the list of libraries that this component should depend on. This
# list should only represent BigWorld distributed libraries, i.e.: anything
# with source code in src/lib (including src/lib/third_party).
#
# Libraries that we link against but are provided by the system should not be
# part of this list.
#

# TODO: The use of bwLibraryDepsAsName is a bit clumsy, we do an
#    $(addprefix lib,..) just below and then an $(addprefix -l,..) later
#    can this be improved?
bwLibraryDepsAsName :=

# For binaries we explicitly add any libraries they may depend on
ifeq ($(call shouldLinkToDependencies,$(type)),1)
bwLibraryDepsAsName += $(dependsOn)

# Some binaries don't want to pull in the huge chain of library dependencies
 ifneq ($(noDefaultDependencies),1)
  bwLibraryDepsAsName += $(BW_DEPENDS_ON)
 endif
 
bwLibraryDepsAsCleanTargets := $(addprefix clean_lib,$(bwLibraryDepsAsName))

# Third party library dependencies are appended last as they will be required
# to resolve dependencies in virtually all libraries.
#
# Don't add third party library dependencies for shared libraries, as these will
# be resolved by the program loading the shared library.
 ifneq ($(call isSharedLibrary,$(type)),1)
bwLibraryDepsAsName += $(thirdPartyDependsOn)
 endif

 
# Link against memhook if we memtracker enabled
ifeq ($(ENABLE_MEMTRACKER),1)
bwLibraryDepsAsName += memhook
endif

ifeq ($(ENABLE_PROTECTED_ALLOCATOR),1)
bwLibraryDepsAsName += memhook
endif

endif


# Pre-requisites should represent an actual file, so we turn our library
# dependancy list into a set of actual paths to local libraries.
bwLibraryDepsAsFile := $(addsuffix .a,$(addprefix $(BW_LIBDIR)/lib,$(bwLibraryDepsAsName)))

# Create a set of third-party pre-requisites as above which will be needed
# by object files due to the buildTimeFile reverse dependency chain.
bwThirdPartyLibraryDepsAsFile := $(addsuffix .a,$(addprefix $(BW_LIBDIR)/lib,$(thirdPartyDependsOn)))

###
# Stage 5
#
# Define compiler and linker flags based on the current delayed_BW_CONFIG
# 

ccFlags_$(bwConfBinName)  += $(ccFlags_$(binName))
cxxFlags_$(bwConfBinName) += $(cxxFlags_$(binName))
cppFlags_$(bwConfBinName) += $(cppFlags_$(binName))
incFlags_$(bwConfBinName) += $(incFlags_$(binName))
ldFlags_$(bwConfBinName)  += $(ldFlags_$(binName))
ldLibs_$(bwConfBinName)   += $(ldLibs_$(binName))

# Compiler flags that are specific to the currently defined delayed_BW_CONFIG
bwConf_CxxFlags :=
bwConf_CppFlags :=
bwConf_LdFlags :=

# Specify the build configuration specific compiler and linker flags
ifeq ($(BW_CONFIG), $(BW_CONFIG_HYBRID))

bwConf_CxxFlags += -O3

bwConf_CppFlags += -DCODE_INLINE
bwConf_CppFlags += -DMF_USE_ASSERTS
bwConf_CppFlags += -D_HYBRID

else ifeq ($(BW_CONFIG), $(BW_CONFIG_DEBUG))

bwConf_CppFlags += -DMF_USE_ASSERTS
bwConf_CppFlags += -D_DEBUG

else

$(error Unknown build config $(BW_CONFIG), unable to set flags)

endif

ifeq ($(user_shouldBuildCodeCoverage), 1)

bwConf_CppFlags += -fprofile-arcs
bwConf_CppFlags += -ftest-coverage
bwConf_LdFlags += -lgcov
bwConf_LdFlags += -coverage

endif

###
# Stage 6
#
# Define rules for how to compile the object files.
# 
ifneq ($(precompiledHeaderFile),)
pchFile := $(outputObjDir)/$(precompiledHeaderFile).gch
 
$(pchFile): bwConfig := $(BW_CONFIG)
$(pchFile): cFlags   := $(ccFlags_$(bwConfBinName)) $(CFLAGS)
$(pchFile): cxxFlags := -x c++-header $(CXXFLAGS) $(cxxFlags_$(bwConfBinName)) $(bwConf_CxxFlags)
$(pchFile): cppFlags := $(cppFlags_$(bwConfBinName)) $(bwConf_CppFlags) $(CPPFLAGS)
$(pchFile): incFlags := $(incFlags_$(bwConfBinName))
#TODO: .h -> .cpp what about .hpp -> .cpp
$(pchFile): $(pathToSource)/$(precompiledHeaderFile:.h=.cpp) $(bwThirdPartyLibraryDepsAsFile) | $(objectDirectories)
	$(bwCommand_createCxxObjects)

-include $(pchFile:.gch=.d)
 
# We use the cxxFlags so the include can be before all others
cxxFlags_$(bwConfBinName) += -include $(outputObjDir)/$(precompiledHeaderFile)
else
pchFile :=
endif


# Target specific variables
$(outputObjDir)/%.o: bwConfig := $(BW_CONFIG)
$(outputObjDir)/%.o: cFlags   := $(ccFlags_$(bwConfBinName)) $(CFLAGS)
$(outputObjDir)/%.o: cxxFlags := $(CXXFLAGS) $(cxxFlags_$(bwConfBinName)) $(bwConf_CxxFlags)
$(outputObjDir)/%.o: cppFlags := $(cppFlags_$(bwConfBinName)) $(bwConf_CppFlags) $(CPPFLAGS)
$(outputObjDir)/%.o: incFlags := $(incFlags_$(bwConfBinName))


$(outputObjDir)/%.o: $(pathToSource)/%.cpp $(pchFile) $(bwThirdPartyLibraryDepsAsFile) | $(objectDirectories)
	$(bwCommand_createCxxObjects)


$(outputObjDir)/%.o: $(pathToSource)/%.c | $(objectDirectories)
	$(bwCommand_createCObjects)

# Pull in all the .d files now
depFile := $(BW_META_DEPDIR)/$(binName)$(componentSuffix).d
$(depFile): outputObjDir := $(outputObjDir) 
$(depFile): depFiles := $(allComponentSource:=.d)
$(depFile): $(componentDependencies) | $(BW_META_DEPDIR)
	@(cd $(outputObjDir); \
		for depfile in $(depFiles); do \
			cat $$depfile; \
			echo ; \
		done) > $@

-include $(depFile)


# End Stage 6
###


endif # ifneq ($(call isCopyOfBinary,$(type)),1)


###
# Stage 7
#
# Define rules for how to compile each type of component:
#  binary, library, shared_library
# 

bwBuildPlatform ?= $(BW_BUILD_PLATFORM)
ifneq ($(BW_BUILD_PLATFORM),$(bwBuildPlatform))
originalBuildPlatform := $(BW_BUILD_PLATFORM)
BW_BUILD_PLATFORM := $(bwBuildPlatform)
endif

#
# Binary
#
ifeq ($(call isBinaryOrUnitTest,$(type)),1)

#special treatment for Havok
ifeq ($(useHavok),1)
ldLibs_$(bwConfBinName) += -Wl,--start-group
endif

ldLibs_$(bwConfBinName) += $(addprefix -l, $(bwLibraryDepsAsName))

#special treatment for Havok
ifeq ($(useHavok),1)
ldLibs_$(bwConfBinName) += -Wl,--end-group
endif

$(destFile): bwConfig   := $(BW_CONFIG)
$(destFile): cxxFlags   := $(bwConf_CxxFlags) $(CXXFLAGS)
$(destFile): ldFlags    := $(ldFlags_$(bwConfBinName)) $(bwConf_LdFlags) $(LDFLAGS)
$(destFile): ldLibs     := $(ldLibs_$(bwConfBinName)) $(LDLIBS)

# We can't use $^ because of the pre-requisite list, so we need to pass
# the required objects through
$(destFile): componentObjects := $(componentObjects)

$(destFile): $(componentObjects) $(bwLibraryDepsAsFile) | $(destDir)
	$(bwCommand_createBinary)


 # If a build time file was specified, add all the dependencies for building
 # the binary _except_ for the build time file itself.
 ifneq ($(buildTimeFile),)

componentBuildTimeDirDependencies += $(objectDirectories)
componentBuildTimeFileDependencies += $(filter-out $(outputObjDir)/$(buildTimeFile).o,$(componentObjects))
componentBuildTimeFileDependencies += $(bwLibraryDepsAsFile)

 endif

 ifeq ($(call isUnitTest,$(type)),1)

.PHONY: bw-run-$(binName)$(componentSuffix)
bw-run-$(binName)$(componentSuffix): $(destFile)
	$<

ifeq ($(usePython), 1)
unitTestRuntimeDependsOn += $(BW_PYTHON_MODULES)
endif

.PHONY: bw-run-test-$(name)$(componentSuffix)
bw-run-test-$(name)$(componentSuffix): workingDir := $(pathToSource)
bw-run-test-$(name)$(componentSuffix): $(destFile) $(unitTestRuntimeDependsOn)
	(cd $(workingDir); $< --root $(BW_ROOT))

shouldAddRunRule := 1
  ifdef user_delayed_onlyRunUnitTestsForConfig
   ifneq ($(user_delayed_onlyRunUnitTestsForConfig),$(BW_CONFIG))
shouldAddRunRule := 0
   endif
  endif

  ifeq ($(shouldAddRunRule),1)
BW_RUN_UNIT_TESTS += $(destFile)
BW_RUN_UNIT_TEST_RULES += bw-run-test-$(name)$(componentSuffix)
BW_UNIT_TEST_BIN_SRC += $(destFile):$(pathToSource)
  endif
 endif

#
# Library
#
else ifeq ($(call isLibrary,$(type)),1)

$(destFile): bwConfig         := $(BW_CONFIG)
$(destFile): componentObjects := $(componentObjects)

$(destFile): $(componentObjects) | $(BW_LIBDIR) $(objectDirectories)
	$(bwCommand_createLibrary)

#
# Shared Library
#
else ifeq ($(call isSharedLibrary,$(type)),1)

#special treatment for Havok
ifeq ($(linkHavok),1)
ldLibs_$(bwConfBinName) += -Wl,--start-group
endif

ldLibs_$(bwConfBinName) += $(addprefix -l, $(bwLibraryDepsAsName))

#special treatment for Havok
ifeq ($(linkHavok),1)
ldLibs_$(bwConfBinName) += -Wl,--end-group
endif

$(destFile): bwConfig := $(BW_CONFIG)
$(destFile): cxxFlags := $(bwConf_CxxFlags) $(CXXFLAGS)
$(destFile): ldFlags  := $(ldFlags_$(bwConfBinName)) $(bwConf_LdFlags) $(LDFLAGS)
$(destFile): ldLibs   := $(ldLibs_$(bwConfBinName)) $(LDLIBS)
$(destFile): componentObjects  := $(componentObjects)

$(destFile): $(componentObjects) $(bwLibraryDepsAsFile) | $(destDir) 
	$(bwCommand_createSharedLibrary)

#
# Copy of Binary
#
else ifeq ($(call isCopyOfBinary,$(type)),1)

$(destFile): bwConfig := $(BW_CONFIG)
$(destFile): $(sourceFile) | $(destDir) 
	$(bwCommand_createCopy)

endif

ifneq ($(BW_BUILD_PLATFORM),$(bwBuildPlatform))
	$(BW_BUILD_PLATFORM) := $(originalBuildPlatform)
endif


###
# Stage 8
#
# Define the remaining rules for this component that aren't directly generated
# from the source code.
#

.PHONY: $(binName)$(componentSuffix) $(preBuildTargets)

# Check preBuildTargets are properly namespaced.
ifneq ($(strip $(filter-out $(binName)_%,$(preBuildTargets))),)
$(info All targets in preBuildTargets for $(binName) must be prefixed with "$$(binName)_")
$(error Offending targets in component $(binName): $(filter-out $(binName)_%,$(preBuildTargets)))
endif

# Check postBuildTargets are properly namespaced.
ifneq ($(strip $(filter-out $(binName)_%,$(postBuildTargets))),)
$(info All targets in postBuildTargets for $(binName) must be prefixed with "$$(binName)_")
$(error Offending targets in component $(binName): $(filter-out $(binName)_%,$(postBuildTargets)))
endif

$(binName)$(componentSuffix): $(objectDirectories) $(bwLibraryDepsAsFile) $(preBuildTargets) $(destFile) $(postBuildTargets)

# Check cleanTargets are properly namespaced.
ifneq ($(strip $(filter-out $(binName)_%,$(cleanTargets))),)
$(info All cleanTargets targets for $(binName) must be prefixed with "$$(binName)_")
$(error Offending targets in component $(binName): $(filter-out $(binName)_%,$(cleanTargets)))
endif

.PHONY: clean_$(binName)$(componentSuffix) cleanTargets_$(binName)$(componentSuffix) $(cleanTargets)
cleanTargets_$(binName)$(componentSuffix): $(cleanTargets)
clean_$(binName)$(componentSuffix): cleanTargets_$(binName)$(componentSuffix) clean_$(bwConfBinName) 
.PHONY: clean_$(bwConfBinName)
clean_$(bwConfBinName): pathsToClean := $(abspath $(objectDirectories)) $(destFile) $(depFile)
clean_$(bwConfBinName): $(addprefix clean_,$(postBuildTargets))
	$(bwCommand_cleanTrees)
	
.PHONY: cleandeps_$(binName)$(componentSuffix)
cleandeps_$(binName)$(componentSuffix): cleandeps_$(bwConfBinName)
.PHONY: cleandeps_$(bwConfBinName)
cleandeps_$(bwConfBinName): $(bwLibraryDepsAsCleanTargets)
	


# This rule is primarily for bwlog.so which lives within the message_logger
# directory. Without this rule we generate a warning because of the second
# include in Makefile.rules wanting to create the same directory.
ifeq ($(noObjectDirectoryRule),0)

$(objectDirectories):
	@mkdir -p $@

endif


###
# Stage 9
#
# Add the component to the primary build list variables.
#

ifneq ($(words $(buildConfigs)),0)

 # --- Binary setup
 ifeq ($(type),binary)

BW_BINS += $(binName)

 # --- Copy of Binary setup
 else ifeq ($(type),copy_of_binary)

BW_BINS += $(binName)


 # --- Unit test setup
 else ifeq ($(type),unit_test)

BW_UNIT_TESTS += $(binName)$(componentSuffix)

 # --- Library setup
 else ifeq ($(type),library)

BW_LIBS += $(binName)$(componentSuffix)

 # --- Shared library setup
 else ifeq ($(type),shared_library)

BW_LIBS += $(binName)

 # --- Copy of Shared library setup
 else ifeq ($(type),copy_of_shared_library)

BW_LIBS += $(binName)

 endif
 
endif


###
# Stage 10
#
# Cleanup some of the configuration specific variables.
#
# While not technically necessary, these will help us with debugging
# incorrect variable usage by not having left over values on the next
# iteration.
# 
# These are candidates for using the 'undefine' functionality in make 3.28
#

delayed_BW_CONFIG :=
outputObjDir :=

componentBaseFiles :=
componentObjects :=
componentDependencies :=

bwLibraryDepsAsFile :=
preBuildTargets :=
postBuildTargets :=
cleanTargets :=

sourceDir :=
sourceFile :=

# common_footer_config.mak
