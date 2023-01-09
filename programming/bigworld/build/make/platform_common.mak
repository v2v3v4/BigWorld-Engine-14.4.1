#
# Common flags for all platforms
#

# Enable dependency file generation
DEPFLAGS := -MMD -MP

CXXFLAGS += -pipe
CXXFLAGS += -Wall -Wno-deprecated
CXXFLAGS += -Wno-uninitialized -Wno-char-subscripts
CXXFLAGS += -fno-strict-aliasing -Wno-non-virtual-dtor
CXXFLAGS += -Wno-write-strings
CXXFLAGS += -g
CXXFLAGS += -Wfloat-equal

OPENSSL_CONFIG_OPTIONS := no-hw no-gost -no-asm no-shared

TIMEOUT_BIN := timeout

# build/platform_common.mak
