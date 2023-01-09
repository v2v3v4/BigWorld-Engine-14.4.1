# This Makefile describes how to use the MongoDB C++ library.

# useMongoDB


# Currently we just include pre-built MongoDB C++ library and header files and pre-built libraries MongoDB C++ driver depends on.
# There is README file under these two libraries which describes how to build them as we need.


MONGO_CXX_DRIVER_ROOT_DIR := $(BW_ABS_SRC)/third_party/mongodb


# Boost related definition
MONGO_CXX_DRIVER_BOOST_DIR := ${MONGO_CXX_DRIVER_ROOT_DIR}/boost
MONGO_CXX_DRIVER_BOOST_BUILD_DIR := ${MONGO_CXX_DRIVER_BOOST_DIR}/build
MONGO_CXX_DRIVER_BOOST_LIB_DIR := ${MONGO_CXX_DRIVER_BOOST_BUILD_DIR}/lib
MONGO_CXX_DRIVER_BOOST_INCLUDE_DIR := $(MONGO_CXX_DRIVER_BOOST_BUILD_DIR)/include

MONGO_CXX_DRIVER_BOOST_INCLUDE := -I${MONGO_CXX_DRIVER_BOOST_INCLUDE_DIR}
MONGO_CXX_DRIVER_BOOST_LIBS := -L$(MONGO_CXX_DRIVER_BOOST_LIB_DIR) -lboost_system-mt -lboost_thread-mt -lboost_program_options-mt -lboost_filesystem-mt -lboost_regex-mt


# MongoDB C++ Driver related definition
MONGO_CXX_DRIVER_DIR := $(MONGO_CXX_DRIVER_ROOT_DIR)/mongo_cxx_driver
MONGO_CXX_DRIVER_BUILD_DIR := $(MONGO_CXX_DRIVER_DIR)/build
MONGO_CXX_DRIVER_LIB_DIR := $(MONGO_CXX_DRIVER_BUILD_DIR)/lib
MONGO_CXX_DRIVER_INCLUDE_DIR := $(MONGO_CXX_DRIVER_BUILD_DIR)/include

# As MongoDB depends on boost, so boost headers need come before MongoDB ones but libs come after MongoDB one
MONGO_CXX_DRIVER_INCLUDE := ${MONGO_CXX_DRIVER_BOOST_INCLUDE} -I${MONGO_CXX_DRIVER_INCLUDE_DIR}
MONGO_CXX_DRIVER_LIBS = -L$(MONGO_CXX_DRIVER_LIB_DIR)/ -lmongoclient ${MONGO_CXX_DRIVER_BOOST_LIBS}


# third_party_mongo_cxx_driver