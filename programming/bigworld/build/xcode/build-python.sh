#!/bin/bash

function abspath() {
    (cd `dirname $1`; echo `pwd`/`basename $1`)
}

SHARED_MODS_OUTPUT_DIR="${SRCROOT}/../../../../game/res/bigworld/scripts/server_common/lib-dynload-macosx"
STANDARD_LIBRARY_OUTPUT_DIR="${SRCROOT}/../../../../game/res/bigworld/scripts/common/Lib"
PYTHON_BUILD_DIR="${SRCROOT}/pythonbuild-macosx"

# This is required for building for BigWorld, else a compile-time assertion
# fails while building any Python-dependent libraries.
CONFIGURE_FLAGS="--enable-unicode=ucs4"

PYTHON_VERSION=2.7
export LDFLAGS="-lcrypto -lssl"
CONFIGURE_SCRIPT_PATH=`abspath ${SRCROOT}/../../third_party/python/configure`

CONFIGURE="LDFLAGS=\"${LDFLAGS}\" ${CONFIGURE_SCRIPT_PATH}"

BUILD_LOG_PATH="${PYTHON_BUILD_DIR}/build.log"

if [ "${CONFIGURATION}" = "Debug" ]; then
    SHARED_MODS_OUTPUT_DIR="${SHARED_MODS_OUTPUT_DIR}_debug"
fi

set -e

echo "== Building Python ${PYTHON_VERSION} ${CONFIGURATION} static library and shared modules =="
date 
echo
echo "Intermediate directory:" `abspath ${PYTHON_BUILD_DIR}`
echo "Shared modules output directory:" `abspath ${SHARED_MODS_OUTPUT_DIR}`
echo "Configure script path:" `abspath ${CONFIGURE_SCRIPT_PATH}`
echo "Build log:" `abspath ${BUILD_LOG_PATH}`

mkdir -p ${PYTHON_BUILD_DIR}

cd ${PYTHON_BUILD_DIR}

if [ ! -f Makefile ]
then
    if [ ! -x ${CONFIGURE_SCRIPT_PATH} ]
    then
        echo "Could not find configure script"
        exit 1
    fi
    echo " * No Makefile, running configure script..."

    if ! env LDFLAGS="${LDFLAGS}" ${CONFIGURE_SCRIPT_PATH} ${CONFIGURE_FLAGS} >> ${BUILD_LOG_PATH} 2>&1 
    then
        echo "Failed to run Python configure"
        exit 1
    fi

    echo " * Finished running configure script"
else
    echo " * Makefile pre-existing, configure skipped"
fi

echo " * Building Python static library"
if ! make libpython${PYTHON_VERSION}.a >> ${BUILD_LOG_PATH} 2>&1
then
    echo "Failed to make static library"
    exit 1
fi

echo " * Building shared modules"
if ! make sharedmods >> ${BUILD_LOG_PATH} 2>&1

then
    echo "Failed to make shared modules"
    exit 1
fi

if ! [ -d ${TARGET_BUILD_DIR} ]
then
    echo " * Creating build directory for static library output"
    if ! mkdir -p ${TARGET_BUILD_DIR}; then
        echo "Failed to create output directory for static library build"
    fi
fi

echo " * Copying static library"
if ! cp -a libpython${PYTHON_VERSION}.a ${TARGET_BUILD_DIR}/
then
    echo "Failed to copy static library"
fi

if ! [ -d ${SHARED_MODS_OUTPUT_DIR} ]
then
    echo " * Creating new shared modules output directory"
    if ! mkdir -p ${SHARED_MODS_OUTPUT_DIR}
    then
        echo "Failed to create shared mods output directory"
        exit 1
    fi
else
    echo " * Flushing existing shared modules"
    if ! find ${SHARED_MODS_OUTPUT_DIR} -type f -name '*.so' -exec rm {} \;
    then
        echo "Failed to flush existing modules, ignoring"
    fi
fi

build_dir=`ls -d build/lib.*-${PYTHON_VERSION}`
build_dir=${build_dir%% *}

echo " * Copying shared modules from `abspath ${build_dir}`"

if ! (cd ${build_dir} ; 
        find . -name '*.so' -type f \
            -exec cp -av {} ${SHARED_MODS_OUTPUT_DIR} \;) >> ${BUILD_LOG_PATH}
then
    echo "Failed to copy shared modules"
    exit 1
fi

echo " * Copying Python standard library files"

if ! rsync --delete -a --exclude=*.svn \
        ${SRCROOT}/../../third_party/python/Lib/ \
        ${STANDARD_LIBRARY_OUTPUT_DIR};
then
    echo "Failed to copy standard library files"
    exit 1
fi

echo " * Finished successfully."


