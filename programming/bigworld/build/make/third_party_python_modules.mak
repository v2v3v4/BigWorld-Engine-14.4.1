# This Makefile describes how to build the various Python modules we
# bundle with the BigWorld res tree
#

# Module source and patches are kept here
sourcePythonModulesDir := $(BW_ABS_SRC)/third_party/python_modules

#
# Generic setup for all third-party Python modules
#

bwSitePackages := site-packages

stampsForPatches = $(join $(dir $(1)),$(addprefix .,$(notdir $(1:=.applied))))

# Intermediate tree directories
bwCommonSiteDir := $(BW_SUFFIX_RES_BIGWORLD)/scripts/common/$(bwSitePackages)
bwServerCommonSiteDir := $(BW_SUFFIX_RES_BIGWORLD)/scripts/server_common/$(bwSitePackages)
bwIntermediateCommonSiteDir := $(BW_INSTALL_DIR)/$(bwCommonSiteDir)
bwIntermediateServerCommonSiteDir := $(BW_INSTALL_DIR)/$(bwServerCommonSiteDir)

$(bwIntermediateCommonSiteDir) $(bwIntermediateServerCommonSiteDir):
	@mkdir -p $@

.PHONY: clean_python_sitePackages
clean_python_sitePackages:
	-@$(RM) -rf $(bwIntermediateCommonSiteDir) $(bwIntermediateServerCommonSiteDir)

BW_CLEAN_HOOKS += clean_python_sitePackages


#
# Pympler
#

pymplerVersion := 0.3.0
# Proxy files for the two trees in Pympler
pymplerPySentinel := pympler/__init__.py
pymplerDataSentinel := templates/index.tpl

# Source tree (third-party) build targets
sourcePymplerDir := $(sourcePythonModulesDir)/Pympler-$(pymplerVersion)
sourcePymplerTGZ := $(sourcePymplerDir).tar.gz
sourcePymplerPatch := $(sourcePymplerDir)-bwchanges.diff
sourcePymplerPatchStamp := $(call stampsForPatches,$(sourcePymplerPatch))
sourcePymplerPySentinel := $(sourcePymplerDir)/$(pymplerPySentinel)
sourcePymplerDataSentinel := $(sourcePymplerDir)/$(pymplerDataSentinel)

$(sourcePymplerDir): cdDirectory := $(sourcePythonModulesDir)
$(sourcePymplerDir): | $(sourcePymplerTGZ)
	$(bwCommand_extract)

$(sourcePymplerPatchStamp): cdDirectory := $(sourcePymplerDir)
$(sourcePymplerPatchStamp): patchDepth := 1
$(sourcePymplerPatchStamp): $(sourcePymplerPatch) | $(sourcePymplerDir)
	$(bwCommand_patch)

.PHONY: prepare_python_pympler
prepare_python_pympler: $(sourcePymplerPatchStamp) | $(sourcePymplerDir)

$(sourcePymplerPySentinel) $(sourcePymplerDataSentinel): | prepare_python_pympler

# Intermediate tree build targets
bwIntermediatePymplerDir := $(bwIntermediateCommonSiteDir)/Pympler-$(pymplerVersion)

$(bwIntermediatePymplerDir): | $(bwIntermediateCommonSiteDir)
	@mkdir -p $@

# .pth file
bwIntermediatePymplerPth := $(bwIntermediatePymplerDir).pth

$(bwIntermediatePymplerPth): | $(bwIntermediatePymplerDir)
	@echo Pympler-$(pymplerVersion) > $@


# Python module
bwIntermediatePymplerPySentinel := $(bwIntermediatePymplerDir)/$(pymplerPySentinel)

$(bwIntermediatePymplerPySentinel): srcTree := $(dir $(sourcePymplerPySentinel))
$(bwIntermediatePymplerPySentinel): treeName := $(subst $(sourcePythonModulesDir)/,,$(srcTree:/=))
$(bwIntermediatePymplerPySentinel): $(sourcePymplerPySentinel) | $(bwIntermediatePymplerDir)
	$(bwCommand_replaceTree)


# Data for python module
bwIntermediatePymplerDataSentinel := $(bwIntermediatePymplerDir)/$(pymplerDataSentinel)

$(bwIntermediatePymplerDataSentinel): srcTree := $(dir $(sourcePymplerDataSentinel))
$(bwIntermediatePymplerDataSentinel): treeName := $(subst $(sourcePythonModulesDir)/,,$(srcTree:/=))
$(bwIntermediatePymplerDataSentinel): $(sourcePymplerDataSentinel) | $(bwIntermediatePymplerDir)
	$(bwCommand_replaceTree)


.PHONY: python_pympler
python_pympler: $(bwIntermediatePymplerPth) $(bwIntermediatePymplerPySentinel) $(bwIntermediatePymplerDataSentinel)

BW_MISC += python_pympler

# clean pympler
.PHONY: clean_python_pympler
clean_python_pympler:
	-@$(RM) -rf $(bwIntermediatePymplerPth) $(bwIntermediatePymplerDir)

BW_CLEAN_HOOKS += clean_python_pympler

# veryclean pympler
.PHONY: veryclean_python_pympler
veryclean_python_pympler:
	-@$(RM) -rf $(sourcePymplerDir) $(sourcePymplerPatchStamp)

BW_VERY_CLEAN_HOOKS += veryclean_python_pympler


#
# Pika
#

pikaName := pika
pikaVersion := 0.9.13
pikaVersionName := $(pikaName)-$(pikaVersion)
pikaDir := $(sourcePythonModulesDir)/$(pikaVersionName)
pikaTGZ := $(pikaDir).tar.gz
pikaSrc := $(pikaDir)/$(pikaName)
pikaIntermediate := $(bwIntermediateServerCommonSiteDir)/$(pikaVersionName)
pikaIntermediateSrc := $(pikaIntermediate)/$(pikaName)

$(pikaDir): cdDirectory := $(sourcePythonModulesDir)
$(pikaDir): | $(pikaTGZ)
	$(bwCommand_extract)

$(pikaSrc): $(pikaDir)

# Copy pika, we only want pika-x.y.z/pika not everything else
$(pikaIntermediate): | $(bwIntermediateServerCommonSiteDir)
	@mkdir -p $@

$(pikaIntermediateSrc): srcTree := $(dir $(pikaSrc))
$(pikaIntermediateSrc): treeName := $(subst $(sourcePythonModulesDir)/,,$(srcTree:/=))
$(pikaIntermediateSrc): $(pikaSrc) | $(pikaIntermediate)
	$(bwCommand_replaceTree)

# .pth file
pikaPthIntermediate := $(pikaIntermediate).pth

$(pikaPthIntermediate): | $(pikaIntermediate)
	@echo $(pikaVersionName) > $@

.PHONY: python_pika
python_pika: $(pikaPthIntermediate) $(pikaIntermediateSrc)

BW_MISC += python_pika


# clean pika
.PHONY: clean_python_pika
clean_python_pika:
	-@$(RM) -rf $(pikaPthIntermediate) $(pikaIntermediate)

BW_CLEAN_HOOKS += clean_python_pika

# veryclean pika
.PHONY: veryclean_python_pika
veryclean_python_pika:
	-@$(RM) -rf $(pikaDir)

BW_VERY_CLEAN_HOOKS += veryclean_python_pika


#
# Oursql
#

oursqlName := oursql
oursqlVersion := 0.9.2
oursqlSrcBaseVersionName := $(oursqlName)-$(oursqlVersion)
oursqlSrcBase := $(sourcePythonModulesDir)/$(oursqlSrcBaseVersionName)
oursqlExtractedDir := $(BW_THIRD_PARTY_BUILD_DIR)/$(oursqlSrcBaseVersionName)
oursqlZipPath := $(oursqlSrcBase).zip
oursqlBuild := $(oursqlExtractedDir)/build/lib.linux-x86_64-$(BW_PYTHON_VERSION)/$(oursqlName).so
oursqlIntermediate := $(bwIntermediateDynloadDir)/$(oursqlName).so

oursqlPatch := $(oursqlSrcBase)-bwchanges.diff
oursqlPatchStamp := $(call stampsForPatches, $(oursqlExtractedDir)-bwchanges.diff)

$(oursqlExtractedDir): cdDirectory := $(BW_THIRD_PARTY_BUILD_DIR)
$(oursqlExtractedDir): | $(oursqlZipPath) $(BW_THIRD_PARTY_BUILD_DIR)
	$(bwCommand_unzip)


$(oursqlPatchStamp): cdDirectory := $(oursqlExtractedDir)
$(oursqlPatchStamp): patchDepth := 1
$(oursqlPatchStamp): $(oursqlPatch) | $(oursqlExtractedDir)
	$(bwCommand_patch)


$(oursqlBuild): cdDirectory := $(oursqlExtractedDir)
$(oursqlBuild): cflags := '-I$(PYTHON_DIR)/Include -I$(PYTHON_BUILD_DIR)'
$(oursqlBuild): ldflags := '-L$(BW_INTERMEDIATE_DIR)/$(BW_PLATFORM_CONFIG)/lib -l$(BW_PYTHON_TARGET) -l$(BW_OPENSSL_CRYPTO_TARGET)'
$(oursqlBuild): opt := build_ext
$(oursqlBuild): pythonBin := $(pythonBuildBinaryTarget)
$(oursqlBuild): $(PYTHON_BUILD_DIR)/pyconfig.h | $(BW_SSL_LIB) $(BW_CRYPTO_LIB) $(BW_PYTHONLIB) $(oursqlExtractedDir) $(pythonBuildBinaryTarget) $(sourcePythonSharedModSentinel) $(oursqlPatchStamp) 
	@rm -f $(oursqlBuild)
	$(bwCommand_pySetup)

$(oursqlIntermediate): bwConfig := lib-dynload-$(BW_PLATFORM_CONFIG)
$(oursqlIntermediate): $(oursqlBuild) | $(bwIntermediateDynloadDir)
	$(bwCommand_createCopy)


.PHONY: python_oursql
python_oursql: $(oursqlIntermediate)

ifeq ($(SYSTEM_HAS_MYSQL),0)
BW_MISC += python_oursql
endif

# clean oursql
.PHONY: clean_python_oursql
clean_python_oursql:
	-@$(RM) -rf $(oursqlIntermediate)

BW_CLEAN_HOOKS += clean_python_oursql

# veryclean oursql
.PHONY: veryclean_python_oursql
veryclean_python_oursql:
	-@$(RM) -rf $(oursqlExtractedDir) $(oursqlPatchStamp)

BW_VERY_CLEAN_HOOKS += veryclean_python_oursql


#
#
# SQLAlchemy
#

sqlalchemy := sqlalchemy
sqlalchemyAsync := AsyncSQLAlchemy.py
sqlalchemyVersion := 0.6.6
sqlalchemyDir := $(sourcePythonModulesDir)/SQLAlchemy-$(sqlalchemyVersion)
sqlalchemyTar := $(sqlalchemyDir).tar.gz
sqlalchemySrc := $(sqlalchemyDir)/lib/$(sqlalchemy)
sqlalchemyAsyncSrc := $(sourcePythonModulesDir)/$(sqlalchemyAsync)
sqlalchemyBase := $(bwIntermediateServerCommonSiteDir)/$(sqlalchemy)-$(sqlalchemyVersion)
sqlalchemyIntermediate := $(sqlalchemyBase)/$(sqlalchemy)
sqlalchemyAsyncIntermediate := $(sqlalchemyBase)/$(sqlalchemyAsync)

$(sqlalchemySrc): cdDirectory := $(sourcePythonModulesDir)
$(sqlalchemySrc): targets := $(subst $(sourcePythonModulesDir)/,,$(sqlalchemySrc))
$(sqlalchemySrc): | $(sqlalchemyTar)
	$(bwCommand_extract)

$(sqlalchemyBase): | $(bwIntermediateServerCommonSiteDir)
	@mkdir -p $@

# .pth file
sqlalchemyIntermediatePth := $(sqlalchemyBase).pth

$(sqlalchemyIntermediatePth): | $(sqlalchemyBase)
	@echo $(sqlalchemy)-$(sqlalchemyVersion) > $@

# Copy sqlalchemy
$(sqlalchemyIntermediate): bwConfig := site-packages
$(sqlalchemyIntermediate): $(sqlalchemySrc) | $(sqlalchemyBase)
	$(bwCommand_dirCopy)

$(sqlalchemyAsyncIntermediate): bwConfig := site-packages
$(sqlalchemyAsyncIntermediate): $(sqlalchemyAsyncSrc) | $(sqlalchemyBase)
	$(bwCommand_createCopy)


# clean SQLAlchemy
.PHONY: clean_python_sqlalchemy
clean_python_sqlalchemy:
	-@$(RM) -rf $(sqlalchemyIntermediate) $(sqlalchemyAsyncIntermediate)

BW_CLEAN_HOOKS += clean_python_sqlalchemy


# veryclean SQLAlchemy
.PHONY: veryclean_python_sqlalchemy
veryclean_python_sqlalchemy:
	-@$(RM) -rf $(sqlalchemyDir)

BW_VERY_CLEAN_HOOKS += veryclean_python_sqlalchemy


# Hook to build sqlalchemy
.PHONY: python_sqlalchemy
ifneq ($(SYSTEM_HAS_MYSQL),0)
python_sqlalchemy: $(sqlalchemyIntermediatePth) $(sqlalchemyIntermediate) $(sqlalchemyAsyncIntermediate)
else
python_sqlalchemy: $(sqlalchemyIntermediatePth) $(sqlalchemyIntermediate) $(sqlalchemyAsyncIntermediate) $(oursqlIntermediate)
endif

BW_MISC += python_sqlalchemy

#
#
# Zope
#

Zope := zope
ZopeVersion := 2.11.4
ZopeDir := $(sourcePythonModulesDir)/Zope-$(ZopeVersion)-final
ZopeTar := $(ZopeDir).tgz
ZopeSrc := $(ZopeDir)/lib/python/$(Zope)
ZopeInit := __init__.py
ZopeInitSrc :=  $(ZopeSrc)/$(ZopeInit)
ZopeInterface := interface
ZopeInterfaceSrc := $(ZopeSrc)/$(ZopeInterface)

ZopeBase := $(bwIntermediateServerCommonSiteDir)/Zope-$(ZopeVersion)
ZopeDestDir := $(abspath $(ZopeBase)/$(Zope))
ZopeIntermediate := $(ZopeDestDir)/interface
ZopeIntermediateInit := $(ZopeDestDir)/$(ZopeInit)

$(ZopeSrc): cdDirectory := $(sourcePythonModulesDir)
$(ZopeSrc): targets := $(subst $(sourcePythonModulesDir)/,,$(ZopeTarSrc))
$(ZopeSrc): | $(ZopeTar)
	$(bwCommand_extract)

$(ZopeInterfaceSrc) $(ZopeInitSrc): | $(ZopeSrc)

# .pth file
ZopeIntermediatePth := $(ZopeBase).pth

$(ZopeIntermediatePth): | $(ZopeIntermediate)
	@echo Zope-$(ZopeVersion) > $@

$(ZopeDestDir): | $(bwIntermediateServerCommonSiteDir)
	@mkdir -p $@

# Copy Zope
$(ZopeIntermediate): bwConfig := site-packages
$(ZopeIntermediate): $(ZopeInterfaceSrc) | $(ZopeDestDir)
	$(bwCommand_dirCopy)

$(ZopeIntermediateInit): bwConfig := site-packages
$(ZopeIntermediateInit): $(ZopeInitSrc) $(ZopeSrc) | $(ZopeIntermediate)
	$(bwCommand_createCopy)

# clean Zope
.PHONY: clean_python_Zope
clean_python_Zope:
	-@$(RM) -rf $(ZopeIntermediate)

BW_CLEAN_HOOKS += clean_python_Zope


# veryclean Zope
.PHONY: veryclean_python_Zope
veryclean_python_Zope:
	-@$(RM) -rf $(ZopeDir)

BW_VERY_CLEAN_HOOKS += veryclean_python_Zope


# Hook to build Zope
.PHONY: python_zope
python_zope: $(ZopeIntermediatePth) $(ZopeIntermediate) $(ZopeIntermediateInit)

BW_MISC += python_zope



#
#
# twisted
#

twisted := twisted
twistedVersion := 11.0.0
twistedDir := $(sourcePythonModulesDir)/Twisted-$(twistedVersion)
twistedBz2 := $(twistedDir).tar.bz2
twistedSrc := $(twistedDir)/$(twisted)
twistedAsyncSrc := $(sourcePythonModulesDir)/$(twistedAsync)
twistedBase := $(bwIntermediateServerCommonSiteDir)/$(twisted)-$(twistedVersion)
twistedIntermediate := $(twistedBase)/$(twisted)

$(twistedSrc): cdDirectory := $(sourcePythonModulesDir)
$(twistedSrc): targets := $(subst $(sourcePythonModulesDir)/,,$(twistedSrc))
$(twistedSrc): | $(twistedBz2)
	$(bwCommand_extract_bz2)

# .pth file
twistedIntermediatePth := $(twistedBase).pth

$(twistedIntermediatePth): | $(twistedIntermediate)
	@echo $(twisted)-$(twistedVersion) > $@

$(twistedBase): | $(bwIntermediateServerCommonSiteDir)
	@mkdir -p $@

# Copy twisted
$(twistedIntermediate): bwConfig := site-packages
$(twistedIntermediate): $(twistedSrc) | $(twistedBase)
	$(bwCommand_dirCopy)


# clean twisted
.PHONY: clean_python_twisted
clean_python_twisted:
	-@$(RM) -rf $(twistedIntermediate)

BW_CLEAN_HOOKS += clean_python_twisted


# veryclean twisted
.PHONY: veryclean_python_twisted
veryclean_python_twisted:
	-@$(RM) -rf $(twistedDir)

BW_VERY_CLEAN_HOOKS += veryclean_python_twisted


# Hook to build twisted
.PHONY: python_twisted
python_twisted: $(twistedIntermediatePth) $(twistedIntermediate)

BW_MISC += python_twisted

#
# msgpack
#

msgpackName := msgpack
msgpackVersion := 0.4.2
msgpackVersionName := $(msgpackName)-python-$(msgpackVersion)
msgpackDir := $(sourcePythonModulesDir)/$(msgpackVersionName)
msgpackTGZ := $(msgpackDir).tar.gz
msgpackSrc := $(msgpackDir)/$(msgpackName)
msgpackIntermediate := $(bwIntermediateServerCommonSiteDir)/$(msgpackVersionName)
msgpackIntermediateSrc := $(msgpackIntermediate)/$(msgpackName)

$(msgpackDir): cdDirectory := $(sourcePythonModulesDir)
$(msgpackDir): | $(msgpackTGZ)
	$(bwCommand_extract)

$(msgpackSrc): $(msgpackDir)

# Copy msgpack, we only want msgpack-x.y.z/msgpack not everything else
$(msgpackIntermediate): | $(bwIntermediateServerCommonSiteDir)
	@mkdir -p $@

$(msgpackIntermediateSrc): srcTree := $(dir $(msgpackSrc))
$(msgpackIntermediateSrc): treeName := $(subst $(sourcePythonModulesDir)/,,$(srcTree:/=))
$(msgpackIntermediateSrc): $(msgpackSrc) | $(msgpackIntermediate)
	$(bwCommand_replaceTree)

# .pth file
msgpackPthIntermediate := $(msgpackIntermediate).pth

$(msgpackPthIntermediate): | $(msgpackIntermediate)
	@echo $(msgpackVersionName) > $@

.PHONY: python_msgpack
python_msgpack: $(msgpackPthIntermediate) $(msgpackIntermediateSrc)

BW_MISC += python_msgpack


# clean msgpack
.PHONY: clean_python_msgpack
clean_python_msgpack:
	-@$(RM) -rf $(msgpackPthIntermediate) $(msgpackIntermediate)

BW_CLEAN_HOOKS += clean_python_msgpack

# veryclean msgpack
.PHONY: veryclean_python_msgpack
veryclean_python_msgpack:
	-@$(RM) -rf $(msgpackDir)

BW_VERY_CLEAN_HOOKS += veryclean_python_msgpack
# third_party_python_modules.mak
