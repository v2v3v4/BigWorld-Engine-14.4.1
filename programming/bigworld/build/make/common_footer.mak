# This file drives the definition of rules for a component for each required
# build configuration.
#
# This file should be included at the tail end of a component Makefile.rules
# using:
#
#  include $(BW_COMPONENT_FOOTER)


# The following segment simply helps us determine which build configurations
# are required to be built for the current component. The logic is fairly
# straight forward.
# - if we have been told to only build certain configs, follow that command
# - otherwise, if we are only to build the default rule, follow that command
# - otherwise, build all the standard configurations
buildConfigs := 

ifdef onlyBuildConfig
 ifeq ($(onlyBuildConfig),$(BW_CONFIG))
	buildConfigs += $(onlyBuildConfig)
 endif
else
 	buildConfigs += $(BW_CONFIG)
endif


# Don't bother doing anything else if we don't have to build this component
ifneq ($(words $(buildConfigs)),0)

#
# Build Time Files
#
# A build time file is a single file specified per component which will be
# rebuilt, if any other dependency of the component has been modified. This is
# used so that the build time displayed within a process acurately represents
# the last build time of the binary.
#
componentBuildTimeFile :=
componentBuildTimeFileDependencies :=
componentBuildTimeDirDependencies :=
 ifneq ($(buildTimeFile),)

componentBuildTimeFile := $(pathToSource)/$(buildTimeFile).cpp

 endif

 ifdef components

# component we have determined should be built.
$(foreach componentName,$(components),$(call includeFooterForComponent,$(componentName)))

 else

include $(BW_COMPONENT_CONFIG_FOOTER)

 endif

 # If a build time file was specified, consolidate all the dependencies and
 # specify a reverse mapping from the buildTimeFile's .cpp to those dependencies
 ifneq ($(componentBuildTimeFile),)

componentBuildTimeFileDependencies := $(call removeDuplicates,$(componentBuildTimeFileDependencies))
componentBuildTimeDirDependencies := $(call removeDuplicates,$(componentBuildTimeDirDependencies))

.SECONDARY: $(componentBuildTimeFile)
$(componentBuildTimeFile): $(componentBuildTimeFileDependencies) | $(componentBuildTimeDirDependencies)
	$(bwCommand_touchTarget)

 endif # ifneq ($(componentBuildTimeFile),)


endif # ifneq ($(words $(buildConfigs)),0)

# As a final step we reset the name and type so that the next thing to come
# along will fail if it isn't configured correctly
#
# These are candidates for using the 'undefine' functionality in make 3.82

name :=
type :=
bindir :=
component :=
components :=
componentName :=

precompiledHeaderFile :=

cSource :=
cxxSource :=
pathToSource :=

exportFor :=
absIncFromDir :=

# common_footer.mak
