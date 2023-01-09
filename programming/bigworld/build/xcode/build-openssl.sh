#!/bin/bash

# Build script for libssl and libcrypto

# Based on Felix Schulze's build script:
#
# Automatic build script for libssl and libcrypto 
# for iPhoneOS and iPhoneSimulator
#
# Created by Felix Schulze on 16.12.10.
# Copyright 2010 Felix Schulze. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

OPENSSL_VERSION=1.0.0d
SDK_VERSION="${IPHONEOS_DEPLOYMENT_TARGET}"

ARCHS="i386 armv7 armv7s arm64"
DEVELOPER=`xcode-select -print-path`

SOURCE_PATH="${SRCROOT}/../../third_party/openssl"
COMPILE_PATH="${SRCROOT}/openssl/src"
BINARY_PATH="${SRCROOT}/openssl/bin"
OUTPUT_PATH="${SRCROOT}/openssl/lib"

MAKE_OPTIONS=-j$((2 * `sysctl -n hw.logicalcpu`))
CONFIGURE_OPTIONS=no-shared

set -e


mkdir -p "${BINARY_PATH}"
mkdir -p "${OUTPUT_PATH}"

if [ -d "${COMPILE_PATH}" ]; 
then
	rm -rf "${COMPILE_PATH}"
fi

if ! cp -R "${SOURCE_PATH}" "${COMPILE_PATH}" 2>&1 > /dev/null 
then
	echo Failed to copy "${SOURCE_PATH}" to "${COMPILE_PATH}"
	exit 1
fi

cd "${COMPILE_PATH}"

BUILT_ARCHES=""

for ARCH in ${ARCHS}
do
	if [ "${ARCH}" == "i386" ]
	then
		PLATFORM="iPhoneSimulator"
	else
		PLATFORM="iPhoneOS"
	fi

	# Patch for iOS builds
	if [ "${PLATFORM}" == "iPhoneOS" ]
	then
		sed -ie \
			"s!static volatile sig_atomic_t intr_signal;!static volatile intr_signal;!" \
			crypto/ui/ui_openssl.c
	fi	
	
	export CROSS_TOP="${DEVELOPER}/Platforms/${PLATFORM}.platform/Developer"
	export CROSS_SDK="${PLATFORM}${SDK_VERSION}.sdk"
	export BUILD_TOOLS="${DEVELOPER}"

    if [[ "$OPENSSL_VERSION" =~ 1.0.0. ]]
	then
		CONFIGURE_PLATFORM="BSD-generic32"
	else
		# 1.0.1 supports iOS cross-compile
		CONFIGURE_PLATFORM="iphoneos-cross"
	fi

	ARCH_BINARY_PATH="${BINARY_PATH}/${PLATFORM}${SDK_VERSION}-${ARCH}.sdk"
	LOG="${ARCH_BINARY_PATH}/build-openssl-${OPENSSL_VERSION}.log"
	if [ -d ${ARCH_BINARY_PATH} ]
	then
		rm -rf ${ARCH_BINARY_PATH}
	fi

	mkdir -p "${ARCH_BINARY_PATH}"
	echo ===================================== | tee -a ${LOG}
	date | tee -a ${LOG}
	echo "Building openssl-${OPENSSL_VERSION} for ${PLATFORM} " \
		"${SDK_VERSION} ${ARCH} ${CONFIGURE_PLATFORM}" | tee -a ${LOG}
	echo "Build log: ${LOG}" | tee -a ${LOG}

	echo "Configuring (with ${CONFIGURE_OPTIONS})..." | tee -a ${LOG}
	export CC="${BUILD_TOOLS}/usr/bin/gcc -arch ${ARCH}"

	if ! ./Configure ${CONFIGURE_PLATFORM} ${CONFIGURE_OPTIONS} \
		--openssldir="${ARCH_BINARY_PATH}"  >> "${LOG}" 2>&1
	then
		exit 1
	fi

	# add -isysroot to CC=
	sed -ie \
		"s!^CFLAG=!CFLAG=-isysroot ${CROSS_TOP}/SDKs/${CROSS_SDK} -miphoneos-version-min=7.0 !" \
		Makefile

	echo "Making..."
	if ! (make clean && 
		make ${MAKE_OPTIONS} build_crypto && 
		make ${MAKE_OPTIONS} build_ssl) >> "${LOG}" 2>&1
	then
		exit 1
	fi

	if [ ! -d ${ARCH_BINARY_PATH}/lib ]; then
		mkdir -p ${ARCH_BINARY_PATH}/lib
	fi

	echo "Copying to target dir ${ARCH_BINARY_PATH}/lib"
	cp libcrypto.a libssl.a ${ARCH_BINARY_PATH}/lib

	BUILT_ARCHES="${BUILT_ARCHES} ${ARCH_BINARY_PATH}"
done

echo "Combining libraries (${BUILT_ARCHES})..."
for lib in ssl crypto
do
	echo lipo -create `for arch in ${BUILT_ARCHES}; \
				do echo $arch/lib/lib${lib}.a; \
			done;` \
		-output ${OUTPUT_PATH}/lib${lib}-ios.a

	lipo -create `for arch in ${BUILT_ARCHES}; \
				do echo $arch/lib/lib${lib}.a; \
			done;` \
		-output ${OUTPUT_PATH}/lib${lib}-ios.a
done

echo "Copying ${COMPILE_PATH}/crypto/opensslconf.h to" \
		" ${SRCROOT}/openssl/include/openssl/opensslconf.h ..."

# Copy from the last arch built
cp "${COMPILE_PATH}/crypto/opensslconf.h" \
	"${SRCROOT}/openssl/include/openssl/opensslconf.h"

echo "Cleaning up..."
rm -rf "${COMPILE_PATH}" "${BINARY_PATH}"

echo "OpenSSL build complete."
