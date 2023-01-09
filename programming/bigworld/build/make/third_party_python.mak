# This Makefile describes how to build the Python library

comma := ,
addUndefined = $(addprefix -Wl$(comma)-u,$(1))
# usePython

BW_PYTHON_VERSION := 2.7

# Enforce that the OpenSSL makefile has already been included as we depend
# on it's directory having been defined.
ifeq ($(origin OPENSSL_BUILD_DIR),undefined)
$(error third_party_openssl.mak must be included before curl)
endif

PYTHON_DIR = $(BW_ABS_SRC)/third_party/python

PYTHON_BUILD_DIR = $(BW_THIRD_PARTY_BUILD_DIR)/python
PYTHON_INSTALL_DIR = $(BW_INSTALL_DIR)/$(BW_THIRD_PARTY_INSTALL_DIR)/python

# Script to help discover system python settings if necessary
PYTHON_DISCOVERY := $(BW_BLDDIR)/discover_python.sh

SYSTEM_PYTHON_INCLUDES := `$(PYTHON_DISCOVERY) --includes`
SYSTEM_PYTHON_LDFLAGS  := `$(PYTHON_DISCOVERY) --ldflags`

# Note: the library has been prefixed with 'bw' to ensure that we don't
#       attempt to link against any system versions of Python.
BW_PYTHON_TARGET := bwpython$(BW_PYTHON_VERSION)

# This is the target used in the Python third party Makefile
sourcePythonFile   := libpython$(BW_PYTHON_VERSION).a
sourcePythonTarget := $(PYTHON_BUILD_DIR)/$(sourcePythonFile)
sourcePythonDependencies := $(PYTHON_BUILD_DIR)/pyconfig.h $(PYTHON_BUILD_DIR)/Makefile

# Extra things for LINKFORSHARED, affecting the built Python interpreter's link
# stage


linkForSharedExtra = -L$(BW_INTERMEDIATE_DIR)/$(BW_PLATFORM_CONFIG)/lib
# Add our OpenSSL build to the Python interpreter
sourcePythonDependencies += $(BW_CRYPTO_LIB) $(BW_SSL_LIB)
linkForSharedExtra += -Wl,--whole-archive -lbwssl -lbwcrypto -Wl,--no-whole-archive

pythonLibDir := $(BW_INTERMEDIATE_DIR)/$(BW_PLATFORM_CONFIG)/lib

BW_PYTHONLIB := $(pythonLibDir)/lib$(BW_PYTHON_TARGET).a

#
# Python static library
#


$(BW_PYTHONLIB): bwConfig := python
$(BW_PYTHONLIB): $(sourcePythonTarget) | $(pythonLibDir)
	$(bwCommand_createCopy)


bwPython_flags = -m64 -fPIC

ifeq ($(BW_BUILD_PLATFORM),el7)
#disable excessive warnings for third-party libraries source
bwPython_flags += -Wno-unused-but-set-variable
endif

ifeq ($(user_enablePythonTraceRefs),1)
pyconfig_extra += \n/* Enable heavy reference debugging */\n\#define Py_TRACE_REFS 1
pyplatform_extra := -pydebug
endif

# Also builds $(PYTHON_BUILD_DIR)/python
$(sourcePythonTarget): cFlags_python := EXTRA_CFLAGS="$(bwPython_flags)"
$(sourcePythonTarget): $(sourcePythonDependencies)
	$(MAKE_WITHOUT_JOBSERVER) -C $(PYTHON_BUILD_DIR) $(@F) $(cFlags_python)

$(PYTHON_BUILD_DIR)/pyconfig.h: $(PYTHON_BUILD_DIR)/Makefile

# Because the Python ./configure process generates symlinks, we need to ensure
# that we don't generate the PYTHON_BUILD_DIR on a Windows mount which will
# cause a failure.
$(PYTHON_BUILD_DIR):
	@mkdir -p $@


# This is needed because when BW_INSTALL_DIR changed, the python build
# should regenerate the Makefile, otherwise python will be installed into
# old (previous) PYTHON_INSTALL_DIR as python configure script always put
# --prefix path into generated Makefile now.
# Note: we also can't just use the PYTHON_INSTALL_DIR as normal prerequisite
# nor order only prerequisite because they will either always trigger the
# reconfiguration even if the PYTHON_INSTALL_DIR is the same or never tigger
# it even if the PYTHON_INSTALL_DIR is changed.
PythonPreInstallSentinel := $(PYTHON_INSTALL_DIR)/bwsentinel_preinstall.txt
$(PYTHON_INSTALL_DIR):
	@mkdir -p $@

$(PythonPreInstallSentinel): | $(PYTHON_INSTALL_DIR)
	@touch $@

# We explicitly set the suffix for the generated Python binary to cater for
# compiling on case-insensitive file systems, as the python binary name
# will conflict with the Python source library directory.
PYTHON_EXE_SUFFIX  := .exe

# The Python binary (is used by make sharedmods)
pythonBinaryFile   := python$(PYTHON_EXE_SUFFIX)
pythonBuildBinaryTarget := $(PYTHON_BUILD_DIR)/$(pythonBinaryFile)

$(pythonBuildBinaryTarget): cFlags_python := EXTRA_CFLAGS="$(bwPython_flags)"
$(pythonBuildBinaryTarget): $(sourcePythonTarget)
	$(MAKE_WITHOUT_JOBSERVER) -C $(PYTHON_BUILD_DIR) $(@F) $(cFlags_python)

pythonInstallBinaryTarget := $(PYTHON_INSTALL_DIR)/bin/$(pythonBinaryFile)

#
# How to ./configure Python
#

pythonConfigureOpts := --enable-unicode=ucs4 
pythonConfigureOpts += --with-suffix="$(PYTHON_EXE_SUFFIX)"
pythonConfigureOpts += --prefix="$(PYTHON_INSTALL_DIR)"

ifeq ($(BW_IS_QUIET_BUILD),1)
pythonConfigureOpts += --silent
endif

# Use our local OpenSSL headers
pythonConfigureOpts += CPPFLAGS="-I$(OPENSSL_BUILD_DIR)/include"

# Note: the first sed replacement is to ensure that functions that have been
#       marked as dangerous (see man pages), don't issue warnings at link time.
#       This approach causes Python to use its own implementation.
#       The second sed replacement is to cause the built Python interpreter to
#       link to the same modules as the eventual runtimes will, ensuring that
#       the Python sharedmodule build step doesn't fail otherwise-value modules.
#       The third sed replacement is to add any extra configuration options that
#       aren't exposed via the configure command line
$(PYTHON_BUILD_DIR)/Makefile: configureName := $(BW_PYTHON_TARGET)
$(PYTHON_BUILD_DIR)/Makefile: cdDirectory := $(PYTHON_BUILD_DIR)
$(PYTHON_BUILD_DIR)/Makefile: configureCmd := $(PYTHON_DIR)/configure
$(PYTHON_BUILD_DIR)/Makefile: configureOpts := $(pythonConfigureOpts)
$(PYTHON_BUILD_DIR)/Makefile: $(PythonPreInstallSentinel) | $(PYTHON_BUILD_DIR) 
	@$(BW_BLDDIR)/test_symlink.sh $(PYTHON_BUILD_DIR)
	@$(BW_BLDDIR)/test_exec.sh $(PYTHON_BUILD_DIR)
	$(bwCommand_configure)
	sed -r -i -e 's/#define (HAVE_T(E)*MPNAM(_R)?) 1/\/* #undef \1 *\//' $(PYTHON_BUILD_DIR)/pyconfig.h
	sed -r -i -e 's#LINKFORSHARED=	-Xlinker -export-dynamic#LINKFORSHARED=	-Xlinker -export-dynamic $(linkForSharedExtra)#' $(PYTHON_BUILD_DIR)/Makefile
	sed -r -i -e '/Py_DEBUG/a \$(pyconfig_extra)' $(PYTHON_BUILD_DIR)/pyconfig.h

#
# Build
#
.PHONY: lib$(BW_PYTHON_TARGET)
lib$(BW_PYTHON_TARGET): $(BW_LIBDIR) $(BW_PYTHONLIB)

BW_THIRD_PARTY += lib$(BW_PYTHON_TARGET)


#
# Clean
#
.PHONY: clean_lib$(BW_PYTHON_TARGET)
clean_lib$(BW_PYTHON_TARGET):
	-@$(RM) -rf $(PYTHON_BUILD_DIR)


#
# Python shared modules and module Library
#

# Proxy file for the entire tree, hopefully changes with every update
pythonLibrarySentinel := __future__.py
# The shared modules we expect
pythonSharedMods := array.so audioop.so binascii.so _bisect.so bz2.so cmath.so _codecs_cn.so _codecs_hk.so _codecs_iso2022.so _codecs_jp.so _codecs_kr.so _codecs_tw.so _collections.so cPickle.so crypt.so cStringIO.so _csv.so _ctypes.so _ctypes_test.so _curses_panel.so _curses.so datetime.so _elementtree.so fcntl.so _functools.so future_builtins.so gdbm.so grp.so _hashlib.so _heapq.so _hotshot.so _io.so itertools.so _json.so linuxaudiodev.so _locale.so _lsprof.so math.so mmap.so _multibytecodec.so _multiprocessing.so nis.so operator.so ossaudiodev.so parser.so pyexpat.so _random.so readline.so resource.so select.so _socket.so spwd.so _sqlite3.so _ssl.so strop.so _struct.so syslog.so termios.so _testcapi.so time.so unicodedata.so zlib.so

# Relative paths in our trees
bwCommonDir := $(BW_SUFFIX_RES_BIGWORLD)/scripts/common
bwPythonLibraryDir := Lib
bwServerCommonDir := $(BW_SUFFIX_RES_BIGWORLD)/scripts/server_common
bwPythonArchitectureDir := lib-dynload-$(BW_PLATFORM_CONFIG)

# Source tree (third-party) build targets
sourcePythonSharedModDir := $(PYTHON_BUILD_DIR)/build/lib.linux-x86_64-$(BW_PYTHON_VERSION)$(pyplatform_extra)
sourcePythonSharedMods := $(addprefix $(sourcePythonSharedModDir)/,$(pythonSharedMods))
sourcePythonSharedModSentinel := $(sourcePythonSharedModDir)/bwsentinel.txt

sourcePythonLibraryDir := $(PYTHON_DIR)/Lib
sourcePythonLibrarySentinel := $(sourcePythonLibraryDir)/$(pythonLibrarySentinel)

# libpython.a is used as a pre-requisite of the shared modules here to ensure
# that a parallel make doesn't attempt to build both the shared modules and
# the library at the same time.
$(sourcePythonSharedModSentinel): cFlags_python := EXTRA_CFLAGS="$(bwPython_flags)"
$(sourcePythonSharedModSentinel): $(pythonBuildBinaryTarget)
	$(MAKE_WITHOUT_JOBSERVER) -C $(PYTHON_BUILD_DIR) sharedmods $(cFlags_python)
	@true $(foreach output,$(sourcePythonSharedMods),&& [ -f $(output) ])
	@if [ $$? == 0 ]; then \
		touch $(sourcePythonSharedModSentinel); \
	fi

$(sourcePythonSharedMods): $(sourcePythonSharedModSentinel)


.PHONY: prepare_python_library
prepare_python_library: $(sourcePythonSharedMods)

# Build the shared python modules in the third-party build, but not the clean
lib$(BW_PYTHON_TARGET): prepare_python_library


# Destination directories for Python shared objects and Libraries
# ie: lib-dynload and Libs
bwIntermediateCommonDir := $(BW_INSTALL_DIR)/$(bwCommonDir)
bwIntermediateServerCommonDir := $(BW_INSTALL_DIR)/$(bwServerCommonDir)

$(bwIntermediateCommonDir):
	@mkdir -p $@


# Lib
bwIntermediateLibraryDir := $(bwIntermediateCommonDir)/$(bwPythonLibraryDir)
bwIntermediateLibrarySentinel := $(bwIntermediateLibraryDir)/$(pythonLibrarySentinel)

$(bwIntermediateLibrarySentinel): srcTree := $(sourcePythonLibraryDir)/
$(bwIntermediateLibrarySentinel): treeName := $(subst $(PYTHON_DIR:/python=)/,,$(srcTree:/=))
$(bwIntermediateLibrarySentinel): $(sourcePythonLibrarySentinel) | $(bwIntermediateLibraryDir)
	$(bwCommand_replaceTree)


# It's possible that the destination Lib directory doesn't exist. This can
# happen if BW_INSTALL_DIR != BW_ROOT
$(bwIntermediateLibraryDir): | $(bwIntermediateCommonDir)
	@mkdir -p $@


.PHONY: clean_python_commonLibDir
clean_python_commonLibDir:
	-@$(RM) -rf $(bwIntermediateLibraryDir)

BW_CLEAN_HOOKS += clean_python_commonLibDir


# Dynload
bwIntermediateDynloadDir := $(bwIntermediateServerCommonDir)/$(bwPythonArchitectureDir)
bwIntermediateSharedMods := $(foreach prefix,$(bwIntermediateDynloadDir),$(addprefix $(prefix)/,$(pythonSharedMods)))
bwIntermediateSharedModRule := $(addsuffix /%.so,$(bwIntermediateDynloadDir))

$(bwIntermediateDynloadDir):
	@mkdir -p $@

.PHONY: clean_python_serverCommonDynloadDir
clean_python_serverCommonDynloadDir:
	-@$(RM) -rf $(bwIntermediateDynloadDir)

BW_CLEAN_HOOKS += clean_python_serverCommonDynloadDir

#
# Implement a new command rule _just_ for this shared modules copy
#
ifndef bwCommandPrefix
$(error third_party_python.mak included before commands.mak)
endif

define actual_pythonCopySharedModule
	cp --preserve=mode,ownership -f $(sourcePythonSharedModDir)/$(@F) $@
endef

define quiet_pythonCopySharedModule
	@echo "[CP] [lib-dynload-$(BW_PLATFORM_CONFIG)] $(@F)"
	@$(actual_pythonCopySharedModule)
endef

bwCommand_pythonCopySharedModule = $($(bwCommandPrefix)pythonCopySharedModule)

$(bwIntermediateSharedModRule): infoMessage := lib-dynload-$(BW_PLATFORM_CONFIG)
$(bwIntermediateSharedModRule): $(sourcePythonSharedModSentinel) $(bwIntermediateDynloadDir)
	$(bwCommand_pythonCopySharedModule)


.PHONY: python_library
python_library: $(bwIntermediateLibrarySentinel) $(bwIntermediateSharedMods)

BW_MISC += python_library

# Anything that needs to run during build and links to our internal python build
# should depend on BW_PYTHON_MODULES and have its resource paths pointing to the
# intermediate resource tree
# used by client_integration/c_plus_plus/sdl atm
BW_PYTHON_MODULES += python_library


# Python module symbols that binaries using our Python modules
# need to provide
getUndefined = $(shell nm -u $(1) | grep 'U ' | grep -v -E ' _?Py' | grep -v '@@GLIBC' | cut -b20-)

modulesWhichNeedSymbols := _hashlib.so _ssl.so
sourceModulesWhichNeedSymbols := $(addprefix $(sourcePythonSharedModDir)/,$(modulesWhichNeedSymbols))

bwThirdPartyPythonSymbolsMak := $(BW_BLDDIR)/third_party_python_symbols.mak
bwIntermediateSymbolsMak := $(PYTHON_BUILD_DIR)/bwsymbols.mak


# In order to link, we use the pregenerated symbols, but we also attempt to
# regenerated them in order to compare whether any differences have occurred.
# If differences are detected we terminate with an error to notify the user
# to update the symbols file.
# 
# Attempting to generate the symbols on the fly wasn't working and caused
# BW_PYTHON_LDFLAGS to be empty when evaluating the process_defs/Makefile.rules.
# This then caused process_defs to fail to load Python modules.
#
# The 'correct' way to resolve this would probably be to have process_defs
# depend directly on the symbols.mak file and assign BW_PYTHON_LDFLAGS from
# within that file when it was finally included, however this is a valid fast
# work around.
include $(bwThirdPartyPythonSymbolsMak)

# Anything which references this needs to depend on BW_PYTHON_EXTRA_DEPS and
# cannot reference it immediately (i.e., with :=)
BW_PYTHON_LDFLAGS := $(foreach symbol,$(symbolsNeededToLink),$(call addUndefined,$(symbol)))


# The following rules regenerate and test whether the symbols file has changed
$(bwIntermediateSymbolsMak): $(sourceModulesWhichNeedSymbols)
	echo "symbolsNeededToLink := $(sort $(foreach mod,$(sourceModulesWhichNeedSymbols),$(call getUndefined,$(mod))))" > $@


.PHONY: python_compare_symbols
python_compare_symbols: $(bwIntermediateSymbolsMak)
	@cmp $(bwThirdPartyPythonSymbolsMak) $(bwIntermediateSymbolsMak)

BW_MISC += python_compare_symbols


#
# Install everything for python to PYTHON_INSTALL_DIR, include python.exe
# binary, man docs, python libraries, python shared binary modules, etc.
#

PythonPostInstallSentinel := $(PYTHON_INSTALL_DIR)/bwsentinel_postinstall.txt

.PHONY: python_install
python_install: python_library
	@if [ ! -f $(PythonPostInstallSentinel) ]; then \
		$(MAKE_WITHOUT_JOBSERVER) -C $(PYTHON_BUILD_DIR) install && touch $(PythonPostInstallSentinel); \
	fi

ifeq ($(user_shouldInstallPython), 1)
BW_THIRD_PARTY += python_install
endif #user_shouldInstallPython

.PHONY: clean_python_install
clean_python_install:
	-@$(RM) -rf $(PYTHON_INSTALL_DIR)

BW_CLEAN_HOOKS += clean_python_install

# third_party_python.mak
