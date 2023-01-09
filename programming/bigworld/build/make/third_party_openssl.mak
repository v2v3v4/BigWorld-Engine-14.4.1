# This Makefile describes how to build the OpenSSL library

# useOpenSSL

# The OpenSSL redist is used for all builds as cstdmf/md5.[ch]pp depends
# on the OpenSSL MD5 implementation.
OPENSSL_DIR = $(BW_ABS_SRC)/third_party/openssl

OPENSSL_BUILD_DIR := $(BW_THIRD_PARTY_BUILD_DIR)/openssl
OPENSSL_BW_LIB_DIR := $(OPENSSL_BUILD_DIR)/lib

# A separate target is used for each library to ensure that the dependency
# chains are well defined.
BW_OPENSSL_CRYPTO_TARGET := bwcrypto
BW_OPENSSL_SSL_TARGET    := bwssl

sslLibDir := $(BW_INTERMEDIATE_DIR)/$(BW_PLATFORM_CONFIG)/lib
sslConfigureFilePath := $(OPENSSL_BUILD_DIR)/Configure
sslMakefileFilePath := $(OPENSSL_BUILD_DIR)/Makefile
sslPlatformIncludeDir := $(OPENSSL_BUILD_DIR)/include/openssl

sslBuildConfHeaderFile := opensslconf.h
sslBuildConfPlatformPath := $(sslPlatformIncludeDir)/$(sslBuildConfHeaderFile)

sslBuildInfHeaderFile := buildinf.h
sslBuildInfPlatformPath := $(sslPlatformIncludeDir)/$(sslBuildInfHeaderFile)

BW_SSL_LIB    := $(sslLibDir)/lib$(BW_OPENSSL_SSL_TARGET).a
BW_CRYPTO_LIB := $(sslLibDir)/lib$(BW_OPENSSL_CRYPTO_TARGET).a

sourceSslFile    = libssl.a
BW_SSL_LIB_SOURCE := $(OPENSSL_BUILD_DIR)/$(sourceSslFile)

sourceCryptoFile    = libcrypto.a
BW_CRYPTO_LIB_SOURCE := $(OPENSSL_BUILD_DIR)/$(sourceCryptoFile)


#####################

ifeq ($(origin OPENSSL_CONFIG_TARGET),undefined)
$(error Platform must define OPENSSL_CONFIG_TARGET for OpenSSL Configure target)
endif

# Configure line
OPENSSL_CONFIGURE := CC="$(CC)" ./Configure $(OPENSSL_CONFIG_OPTIONS) \
	$(OPENSSL_CONFIG_TARGET) $(OPENSSL_CONFIG_FLAGS)


#
# Recipes for creation of the libraries (in order of a clean build)
$(OPENSSL_BUILD_DIR):
	mkdir -p $@


# If the OpenSSL Configure file does not exist then the entire source dir should be
# relinked to the platform dir.
$(sslConfigureFilePath): | $(OPENSSL_BUILD_DIR)
	@$(BW_BLDDIR)/test_symlink.sh $(OPENSSL_BUILD_DIR)
	cp -TR $(OPENSSL_DIR) $(OPENSSL_BUILD_DIR)
	chmod -R +w $(OPENSSL_BUILD_DIR)

# Makefile
$(sslMakefileFilePath): $(sslConfigureFilePath)
	(cd $(OPENSSL_BUILD_DIR) && $(OPENSSL_CONFIGURE))

# Regenerate the build config header by rebuilding the entire library (using
# intermediate as a multi-target recipe that is safe for jobserver builds).
.INTERMEDIATE: $(sslBuildConfPlatformPath).intermediate
$(sslBuildConfPlatformPath).intermediate: $(sslMakefileFilePath)
	rm -f $(BW_SSL_LIB_SOURCE) $(BW_CRYPTO_LIB_SOURCE)
	$(MAKE_WITHOUT_JOBSERVER) -C $(OPENSSL_BUILD_DIR) clean build_ssl build_crypto

$(sslBuildConfPlatformPath): $(sslBuildConfPlatformPath).intermediate
$(BW_SSL_LIB_SOURCE): $(sslBuildConfPlatformPath).intermediate
$(BW_CRYPTO_LIB_SOURCE): $(sslBuildConfPlatformPath).intermediate

# Destination files
$(BW_CRYPTO_LIB): $(BW_CRYPTO_LIB_SOURCE) | $(sslLibDir)
	$(bwCommand_createCopy)

$(BW_SSL_LIB): $(BW_SSL_LIB_SOURCE) | $(sslLibDir)
	$(bwCommand_createCopy)

#
# Build
#
.PHONY: lib$(BW_OPENSSL_CRYPTO_TARGET) lib$(BW_OPENSSL_SSL_TARGET)
lib$(BW_OPENSSL_CRYPTO_TARGET): $(BW_CRYPTO_LIB)
lib$(BW_OPENSSL_SSL_TARGET): $(BW_SSL_LIB)

BW_THIRD_PARTY += lib$(BW_OPENSSL_CRYPTO_TARGET) lib$(BW_OPENSSL_SSL_TARGET)


#
# Clean
#
.PHONY: clean_openssl
clean_openssl:
	-@$(RM) -rf $(sslLibDir) $(OPENSSL_BUILD_DIR) $(BW_SSL_LIB) $(BW_CRYPTO_LIB)

.PHONY: clean_lib$(BW_OPENSSL_CRYPTO_TARGET) clean_lib$(BW_OPENSSL_SSL_TARGET)
clean_lib$(BW_OPENSSL_CRYPTO_TARGET): clean_openssl
clean_lib$(BW_OPENSSL_SSL_TARGET): clean_openssl

# third_party_openssl.mak
