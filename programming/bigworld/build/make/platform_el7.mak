#
# Configuration for EL 7-based platforms
#
CXX11_CXXFLAGS := -std=c++11

# Test if the system installed Mongo DB library, 0 is installed. Now as we are
# building and linking to mongodb cxx driver locally, the testing is not
# appropriate anymore. We only support it on el7 at the moment, so we just
# assume system has mongodb for el7. In future, if we go with system installed
# mongodb cxx driver again, we can re-enable this testing. 
#SYSTEM_TEST_MONGODB_RESULT := $(shell $(BW_BLDDIR)/test_mongodb.sh && echo $$?)
#ifeq ($(SYSTEM_TEST_MONGODB_RESULT),0)
SYSTEM_HAS_MONGODB := 1
#endif


# Enable C++11 as a standard compiler for EL 7
CXXFLAGS   += $(CXX11_CXXFLAGS)

# build/make/platform_el7.mak
