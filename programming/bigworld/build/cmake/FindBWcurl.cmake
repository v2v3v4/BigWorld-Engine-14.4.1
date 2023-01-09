# - Find BigWorld-bundled curl installation
# Try to find the curl libraries bundled with BigWorld
# Once done, this will define
#	BWCURL_FOUND - BigWorld-bundled curl is present and compiled
#	BWCURL_INCLUDE_DIRS - The BigWorld-bundled curl include directories
#	BWCURL_LIBRARIES - The libraries needed to use the BigWorld-bundled curl

# Based on FindScaleformSDK.cmake, which is
# based on http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries
# and FindALSA.cmake, which is apparently current best-practice

FIND_PATH( BWCURL_ROOT_DIR
	include/curl/curlver.h
	PATHS	${BW_SOURCE_DIR}/third_party/curl
)

# Hide all of the curl build madness.
IF ( BW_ARCH_32 )
	SET( _ARCH "x86" )
ELSE()
	SET( _ARCH "x64" )
ENDIF()

SET( BWCURL_BUILD_RELEASE ${BWCURL_ROOT_DIR}/builds/libcurl-${BW_COMPILER_TOKEN}-${_ARCH}-release-static-sspi )
SET( BWCURL_BUILD_DEBUG ${BWCURL_ROOT_DIR}/builds/libcurl-${BW_COMPILER_TOKEN}-${_ARCH}-debug-static-sspi )

UNSET( _ARCH )

# BWcurl Include dir
# TODO: Release headers always? Can we fix this?
# Not a huge issue, curlbuild.h doesn't differentiate debug or release
FIND_PATH( BWCURL_INCLUDE_DIR_RELEASE
	curl/curlver.h "${BWCURL_BUILD_RELEASE}/include"
)

# Extract the version from LIBCURL_VERSION in curl/curlver.h
IF( BWCURL_INCLUDE_DIR_RELEASE AND EXISTS "${BWCURL_INCLUDE_DIR_RELEASE}/curl/curlver.h" )
	FILE( STRINGS "${BWCURL_INCLUDE_DIR_RELEASE}/curl/curlver.h"
		_BWCURL_VERSION_STR
		REGEX "^#define[\t ]+LIBCURL_VERSION[\t ]+\"[0-9.]+\""
	)
	STRING( REGEX REPLACE "^.*\"([^\ ]*)\".*$" "\\1"
		BWCURL_VERSION_STRING ${_BWCURL_VERSION_STR}
	)
	UNSET( _BWCURL_VERSION_STR )
ENDIF()

# BWcurl library paths
FIND_LIBRARY( BWCURL_LIBRARY_RELEASE
	libcurl_a.lib ${BWCURL_BUILD_RELEASE}/lib
)

FIND_LIBRARY( BWCURL_LIBRARY_DEBUG
	libcurl_a_debug.lib ${BWCURL_BUILD_DEBUG}/lib
)

INCLUDE( FindPackageHandleStandardArgs )

FIND_PACKAGE_HANDLE_STANDARD_ARGS( BWcurl
	REQUIRED_VARS
		# Top of BWcurl tree
		BWCURL_ROOT_DIR 
		# Headers
		BWCURL_INCLUDE_DIR_RELEASE
		# Libraries
		BWCURL_LIBRARY_RELEASE
		BWCURL_LIBRARY_DEBUG
	VERSION_VAR BWCURL_VERSION_STRING
)

IF( BWCURL_FOUND )
	SET( BWCURL_INCLUDE_DIRS
		${BWCURL_INCLUDE_DIR_RELEASE}
	)

	SET( BWCURL_LIBRARIES
		optimized ${BWCURL_LIBRARY_RELEASE}
		debug ${BWCURL_LIBRARY_DEBUG}
		wldap32.lib
	)

ENDIF()

MARK_AS_ADVANCED( 
	BWCURL_INCLUDE_DIR_RELEASE
	BWCURL_LIBRARY_RELEASE
	BWCURL_LIBRARY_DEBUG
)
