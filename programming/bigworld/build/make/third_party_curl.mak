# This Makefile describes how to build the cURL library

# useCurl

# Enforce that the OpenSSL makefile has already been included as we depend
# on it's directory having been defined.
ifeq ($(origin OPENSSL_BUILD_DIR),undefined)
$(error third_party_openssl.mak must be included before curl)
endif

CURL_DIR = $(BW_ABS_SRC)/third_party/curl

# Note: We don't want a build config specific build directory for cURL as it
#       is just doing the same ./configure step in each config. If anything we
#       should consider having a per-platform directory (eg: libcurl-el5)
CURL_BUILD_DIR = $(BW_THIRD_PARTY_BUILD_DIR)/libcurl-$(BW_PLATFORM_CONFIG)

# Note: the library has been prefixed with 'bw' to ensure that we don't
#       attempt to link against any system versions of Curl.
BW_CURL_TARGET := bwcurl

sourceCurlFile   := libcurl.a
sourceCurlTarget := $(CURL_BUILD_DIR)/$(sourceCurlFile)
sourceCurlDependencies := $(CURL_BUILD_DIR)/include/curl/curlbuild.h $(CURL_BUILD_DIR)/lib/Makefile

curlConfigureOpts := \
	--without-libidn \
	--disable-ldap \
	--disable-shared \
	--disable-manual \
	--enable-silent-rules \
	--with-ssl=$(abspath $(OPENSSL_BUILD_DIR))

ifeq ($(BW_IS_QUIET_BUILD),1)
curlConfigureOpts += --silent
endif

curlLibDir := $(BW_INTERMEDIATE_DIR)/$(BW_PLATFORM_CONFIG)/lib

BW_CURLLIB := $(curlLibDir)/lib$(BW_CURL_TARGET).a

#
# Curl static library
#

$(BW_CURLLIB): bwConfig := curl
$(BW_CURLLIB): $(sourceCurlTarget) | $(curlLibDir)
	$(bwCommand_createCopy)

sourceCurlLib := $(CURL_BUILD_DIR)/lib/.libs/libcurl.a
$(sourceCurlTarget): $(sourceCurlLib)
	$(bwCommand_createCopy)

# The actual rule for kicking off the 'make'
# TODO: encapsulate the $(MAKE) inside a $(bwCommand...) that better handles
#       silent mode and echos what it's doing
ifeq ($(BW_IS_QUIET_BUILD),1)
extraMakeVariables := 
$(sourceCurlLib): extraMakeVariables := LIBTOOLFLAGS=--silent
endif
$(sourceCurlLib): $(sourceCurlDependencies)
	@$(MAKE_WITHOUT_JOBSERVER) $(extraMakeVariables) -C $(CURL_BUILD_DIR)/lib


$(CURL_BUILD_DIR)/include/curl/curlbuild.h: $(CURL_BUILD_DIR)/lib/Makefile

$(CURL_BUILD_DIR):
	@mkdir -p $@

$(CURL_BUILD_DIR)/lib/Makefile: configureName := $(sourceCurlFile)
$(CURL_BUILD_DIR)/lib/Makefile: cdDirectory := $(CURL_BUILD_DIR)
$(CURL_BUILD_DIR)/lib/Makefile: configureCmd := $(CURL_DIR)/configure
$(CURL_BUILD_DIR)/lib/Makefile: configureOpts := $(curlConfigureOpts)
$(CURL_BUILD_DIR)/lib/Makefile: | $(CURL_BUILD_DIR) $(BW_SSL_LIB) $(BW_CRYPTO_LIB)
	$(bwCommand_configure)


#
# Build
#
.PHONY: lib$(BW_CURL_TARGET)
lib$(BW_CURL_TARGET): $(BW_LIBDIR) $(BW_CURLLIB)

BW_THIRD_PARTY += lib$(BW_CURL_TARGET)


#
# Clean
#
.PHONY: clean_lib$(BW_CURL_TARGET)
clean_lib$(BW_CURL_TARGET):
	-@$(RM) -rf $(CURL_BUILD_DIR)

# third_party_curl.mak
