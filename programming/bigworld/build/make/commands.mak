#
# C++ Objects
#
define actual_createCxxObjects
	$(CXX) $(cxxFlags) $(cppFlags) $(incFlags) -c $<  -o $@
endef

define quiet_createCxxObjects
	@echo "[CXX] [$(bwConfig)] $(lastword $(subst /, ,$(@D)))/$(@F)"
	@$(actual_createCxxObjects)
endef


#
# C Objects
#
define actual_createCObjects
	$(CC) $(cFlags) $(cppFlags) $(incFlags) -c $<  -o $@
endef

define quiet_createCObjects
	@echo "[CC] [$(bwConfig)] $(lastword $(subst /, ,$(@D)))/$(@F)"
	@$(actual_createCObjects)
endef


#
# Binary
#
define actual_createBinary
	$(CXX) $(cxxFlags) -o $@ $(componentObjects) $(ldFlags) $(ldLibs)
endef

define quiet_createBinary
	@echo "[BIN] [$(bwConfig)] $(@F)"
	@$(actual_createBinary)
endef


#
# Library
#
define actual_createLibrary
	$(AR) $(ARFLAGS) $@ $(componentObjects)
endef

define quiet_createLibrary
	@echo "[LINK] [$(bwConfig)] $(@F)"
	@$(actual_createLibrary)
endef


#
# Shared Library
#
define actual_createSharedLibrary
	$(CXX) $(cxxFlags) -shared -o $@ $(componentObjects) $(ldFlags) $(ldLibs)
endef

define quiet_createSharedLibrary
	@echo "[SO] [$(bwConfig)] $(@F)"
	@$(actual_createSharedLibrary)
endef


#
# Symbolic Link
#
define actual_createSymbolicLink
	ln -s -f $< $@;	\
	if [ $$? != 0 ]; then		\
		cp $< $@;	\
	fi
endef

define quiet_createSymbolicLink
	@echo "[LN] [$(bwConfig)] $(@F)"
	@$(actual_createSymbolicLink)
endef


#
# Copy of a file
#
define actual_createCopy
	cp --preserve=mode -f $< $@
endef

define quiet_createCopy
	@echo "[CP] [$(bwConfig)] $(@F)"
	@$(actual_createCopy)
endef


#
# Copy of a directory
#
define actual_dirCopy
	mkdir -p $(dir $@);		\
	cp -a -f $< $@
endef

define quiet_dirCopy
	@echo "[CP] [$(bwConfig)] $(@F)"
	@$(actual_dirCopy)
endef


#
# Touch a file or files
#
define actual_touchTarget
	touch $@
endef

define quiet_touchTarget
	@echo "[TOUCH] [$(bwConfig)] $(lastword $(subst /, ,$(@D)))/$(@F)"
	@$(actual_touchTarget)
endef


#
# Replace a directory tree, ignoring SVN 1.6 WC directories
# $(dir $@) must evaluate to the directory to copy $(srcTree)
# into. $(dir $srcTree) should evaluate to $srcTree.
#
define actual_replaceTree
	-[ -d $(dir $@) ] && find $(dir $@) -type d -name .svn -prune -o -type f -print0 | xargs -0 rm -f

	# create a temporary keep-around file in $(dir $@ ), so the directory will
	# not be removed by "rmdir -p".
	-[ -d $(dir $@) ] && touch $(dir $@)/tmp_keep_around

	-[ -d $(dir $@) ] && find $(dir $@) -type d -name .svn -prune -o -type d -print0 | xargs -0 rmdir -p --ignore-fail-on-non-empty
	mkdir -p $(dir $@)
	cd $(dir $@) && find $(srcTree) -type d -name .svn -prune -o -type d -printf %P/\\0 | xargs -0 mkdir -p
	cd $(srcTree) && find $(srcTree) -type d -name .svn -prune -o -type f -printf %P\\0 | xargs -0 cp --parents -t $(dir $@)

	# remove the temporary keep-around file.
	-rm -f $(dir $@)/tmp_keep_around
endef

define quiet_replaceTree
	@echo "[CPTREE] $(treeName)"
	@$(actual_replaceTree)
endef

#
# Delete a list of trees
#
define actual_cleanTrees
	-rm -rf $(pathsToClean)
endef

define quiet_cleanTrees
	@for path in $(patsubst $(BW_ROOT)/%,%,$(pathsToClean)); do echo "[CLEAN] $$path"; done
	@$(actual_cleanTrees)
endef

#
# ./configure
#
define actual_configure
	cd $(cdDirectory) && $(configureCmd) $(configureOpts)
endef

define quiet_configure
	@echo "[AUTOCONF] $(configureName)"
	@$(actual_configure)
endef


#
#
# Extract a zip
#
define actual_unzip
	unzip -qq $(firstword $|) -d $(cdDirectory)
endef

define quiet_unzip
	@echo "[UNZIP] $(notdir $(firstword $|))"
	@$(actual_unzip)
endef

#
#
# Extract a tarball
# Tarball is the first order-only dependancy, because tar
# resets mtime on its output, so there's no point having
# a full dependancy on the tarball
#
define actual_extract
	tar xzf $(firstword $|) -C $(cdDirectory) $(targets)
endef

define quiet_extract
	@echo "[UNTAR] $(notdir $(firstword $|))"
	@$(actual_extract)
endef

#
#
# Extract a tarball (bz2)
# Tarball is the first order-only dependancy, because tar
# resets mtime on its output, so there's no point having
# a full dependancy on the tarball
#
define actual_extract_bz2
	tar xjf $(firstword $|) -C $(cdDirectory) $(targets)
endef

define quiet_extract_bz2
	@echo "[UNTAR] $(notdir $(firstword $|))"
	@$(actual_extract_bz2)
endef

#
# Apply a patch. This rule works in this order:
# 1. Test whether the patch has been applied by option -R --dry-run, if so,
#    return success
# 2. Dry-run applying the patch by option --dry-run, if sucessful, continue
#    to step 3, otherwise return failure
# 3. Apply the patch 
# This is "result = (try-reverse() || (try() && apply()))" in C parlance.
#
define actual_patch
	patch -R --dry-run --force -s -d $(cdDirectory) -p $(patchDepth) < $< \
	|| (patch --dry-run --force -s -d $(cdDirectory) -p $(patchDepth) < $< \
	&& patch --force -s -d $(cdDirectory) -p $(patchDepth) < $<)
	
	touch $@
endef

define quiet_patch
	@echo "[APPLY] $(<F)"
	@$(actual_patch)
endef

#
# Execute Python setup.py
#
define actual_pySetup
	cd $(cdDirectory) && CFLAGS=$(cflags) LDFLAGS=$(ldflags) $(pythonBin) $(cdDirectory)/setup.py $(opt) &> /dev/null
endef

define quiet_pySetup
	@echo "[SETUP.PY] $(@F)"
	@$(actual_pySetup)
endef

#
# Setup the variables for use in building
#
ifeq ($(BW_IS_QUIET_BUILD),1)
 bwCommandPrefix := quiet_
else
 bwCommandPrefix := actual_
endif

bwCommand_createCxxObjects    = $($(bwCommandPrefix)createCxxObjects)
bwCommand_createCObjects      = $($(bwCommandPrefix)createCObjects)

bwCommand_createBinary        = $($(bwCommandPrefix)createBinary)
bwCommand_createLibrary       = $($(bwCommandPrefix)createLibrary)
bwCommand_createSharedLibrary = $($(bwCommandPrefix)createSharedLibrary)

bwCommand_createSymbolicLink  = $($(bwCommandPrefix)createSymbolicLink)
bwCommand_createCopy          = $($(bwCommandPrefix)createCopy)
bwCommand_dirCopy             = $($(bwCommandPrefix)dirCopy)
bwCommand_touchTarget         = $($(bwCommandPrefix)touchTarget)
bwCommand_replaceTree         = $($(bwCommandPrefix)replaceTree)
bwCommand_cleanTrees		  = $($(bwCommandPrefix)cleanTrees)

bwCommand_configure           = $($(bwCommandPrefix)configure)

bwCommand_unzip               = $($(bwCommandPrefix)unzip)
bwCommand_extract             = $($(bwCommandPrefix)extract)
bwCommand_extract_bz2         = $($(bwCommandPrefix)extract_bz2)
bwCommand_patch               = $($(bwCommandPrefix)patch)
bwCommand_pySetup             = $($(bwCommandPrefix)pySetup)

# commands.mak
