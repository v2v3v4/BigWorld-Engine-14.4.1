#
# Configuration for EL 6-based platforms
#


# Note: This flag is only to work around an issue in gcc 4.4 see BWT-22459
CXXFLAGS += -fno-tree-sink
CXX11_CXXFLAGS := -std=c++0x

# build/make/platform_el6.mak
