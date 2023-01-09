# Additional include directories
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/lib )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party )
INCLUDE_DIRECTORIES( $ENV{DXSDK_DIR}/Include )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/python/Include )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/python/PC )

# Add include directories for remote build system header files
IF (BW_IS_REMOTE_ONLY AND  NOT ${BW_LINUX_CONN_TYPE} MATCHES "RSYNC" )
	INCLUDE_DIRECTORIES(BEFORE SYSTEM ${BW_REMOTE_INCLUDE_DIRS} )
ENDIF()

BW_ADD_COMPILE_FLAGS(
	/fp:fast  # Fast floating point model
)

# Arch dependent library directories and definitions
IF( BW_PLATFORM_WINDOWS AND BW_ARCH_32 ) 
	LINK_DIRECTORIES( $ENV{DXSDK_DIR}/Lib/x86 )
	IF( BW_FMOD_SUPPORT )
		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/fmod/api/lib )
		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/fmod/fmoddesignerapi/api/lib )
	ENDIF()
	IF( BW_UMBRA_SUPPORT )
		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/umbra/lib/win32 )
	ENDIF()
ELSEIF( BW_PLATFORM_WINDOWS AND BW_ARCH_64 )
	LINK_DIRECTORIES( ${BW_SOURCE_DIR}/lib )
	LINK_DIRECTORIES( $ENV{DXSDK_DIR}/Lib/x64 )
	IF( BW_FMOD_SUPPORT )
		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/fmod/api/lib/x64 )
		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/fmod/fmoddesignerapi/api/lib/x64 )
	ENDIF()
	IF( BW_UMBRA_SUPPORT )
		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/umbra/lib/win64 )
	ENDIF()
ELSEIF( BW_PLATFORM_LINUX )
ELSE()
	MESSAGE( FATAL_ERROR "invalid platform '${BW_PLATFORM}'!" )
ENDIF()

ADD_DEFINITIONS( -DBW_BUILD_PLATFORM="${BW_PLATFORM}" )

# TODO: Re-enable the eval version when we can build it
#IF( MSVC90 )
#	LINK_DIRECTORIES( ${BW_SOURCE_DIR}/lib/speedtreert/lib/VC90 )
#ELSEIF( MSVC10 OR MSVC11 )
#	IF( ${BW_PLATFORM} STREQUAL "win64" )
#		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/lib/speedtreert/lib/VC100/x64 )
#	ELSE()
#		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/lib/speedtreert/lib/VC100 )
#	ENDIF()
#ELSE()
#	MESSAGE( FATAL_ERROR "CMake standard project for '${CMAKE_GENERATOR}' not currently supported by BigWorld." )
#ENDIF()

LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/nvtt/lib/${BW_COMPILER_TOKEN} )

IF( BW_PLATFORM_WINDOWS )
# Windows version definitions
ADD_DEFINITIONS(
	-DNTDDI_VERSION=0x05010000 # NTDDI_WINXP
	-D_WIN32_WINNT=0x0501   # Windows XP
	-D_WIN32_WINDOWS=0x0501
	-D_WIN32_IE=0x0600
)

# Standard preprocessor definitions
ADD_DEFINITIONS(
	-D_CRT_SECURE_NO_WARNINGS
	-D_CRT_NONSTDC_NO_DEPRECATE
	-D_CRT_SECURE_NO_DEPRECATE
	-D_CRT_NONSTDC_NO_DEPRECATE
	#-DD3DXFX_LARGEADDRESS_HANDLE # NOT YET FOR WOWP
)

ENDIF()

IF ( MSVC AND BW_IS_REMOTE_ONLY )
BW_ADD_COMPILE_FLAGS( 
	/u # Remove all predefined macros
	/FI"${CMAKE_BINARY_DIR}/gcc_defines.h" # Force include GCC defines
)
ENDIF()

# Tell python extensions that we are not building against python27.dll
# Ideally this definition should only be enabled in project which depend on python
# But since in BW nearly everything depends on python it's enabled for every project
IF( NOT BW_PYTHON_AS_SOURCE )
	IF( BW_ARCH_32 ) 
		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/python/PCbuild )
	ELSEIF( BW_ARCH_64 )
		LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/python/PCbuild/amd64 )
	ENDIF()
ELSE()
	ADD_DEFINITIONS( -DPy_BUILD_CORE )
ENDIF()

IF( NOT BW_NO_UNICODE )
	ADD_DEFINITIONS( -DUNICODE -D_UNICODE )
ENDIF()

# Remove /STACK:10000000 set by CMake. This value for stack size
# is very high, limiting the number of threads we can spawn.
# Default value used by Windows is 1MB which is good enough.
STRING( REGEX REPLACE "/STACK:[0-9]+" ""
	CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" )
