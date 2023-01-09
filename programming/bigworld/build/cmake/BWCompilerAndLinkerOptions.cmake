### Initialise compiler and linker flags ###

IF( MSVC )
	### MSVC COMPILER FLAGS ###

	# Flags used by C and C++ compilers for all build types
	SET( BW_COMPILER_FLAGS
		# Preprocessor definitions
		/DWIN32
		/D_WINDOWS

		# General
		/W3		# Warning level 3
		/Zi		# Always generate debug information
		/MP		# Enable parallel builds
		/WX		# Enable warnings as errors
		
		# Code generation
		/Gy		# Enable function level linking	
		
		/w34302 # Enable warning 'conversion': truncation from 'type1' to 'type2'
		/d2Zi+	# Put local variables and inline functions into the PDB
		)

	# Flags used by C and C++ compilers for specific architectures	
	IF( CMAKE_SIZEOF_VOID_P EQUAL 4 )
		LIST( APPEND BW_COMPILER_FLAGS 
			# Code generation
			/arch:SSE	# Streaming SIMD extensions
			)
	ELSEIF( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		LIST( APPEND BW_COMPILER_FLAGS
			# Preprocessor definitions
			/D_WIN64
			)
	ENDIF()

	# Flags used by C and C++ compilers for Debug builds
	SET( BW_COMPILER_FLAGS_DEBUG
		# Preprocessor definitions
		/D_DEBUG
		 
		# Optimization
		/Od		# Disable optimization
		/Ob0	# Disable inline function expansion
		
		# Code generation
		/MDd	# Multi-threaded debug DLL runtime library
		/RTC1	# Basic runtime checks
		)

	# Flags used by C and C++ compilers for Hybrid Consumer_Release builds
	SET( BW_COMPILER_FLAGS_OPTIMIZED
		# Preprocessor definitions
		/DNDEBUG
		
		# Optimization
		/Ox		# Full optimization
		/Ob2	# Any suitable inline function expansion
		/Oi		# Enable intrinsic functions
		/Ot		# Favor fast code
		
		# Code generation
		/GF		# Enable string pooling
		/MD		# Multi-threaded debug DLL runtime library
		)

	# Flags used by C and C++ compilers for Hybrid builds
	SET( BW_COMPILER_FLAGS_HYBRID ${BW_COMPILER_FLAGS_OPTIMIZED} )

	# Flags used by C and C++ compilers for Consumer_Release builds
	SET( BW_COMPILER_FLAGS_CONSUMER_RELEASE ${BW_COMPILER_FLAGS_OPTIMIZED} )

	# Flags used by the C compiler 
	SET( BW_C_FLAGS ${BW_COMPILER_FLAGS} )
	SET( BW_C_FLAGS_DEBUG ${BW_COMPILER_FLAGS_DEBUG} )
	SET( BW_C_FLAGS_HYBRID ${BW_COMPILER_FLAGS_HYBRID} )
	SET( BW_C_FLAGS_CONSUMER_RELEASE ${BW_COMPILER_FLAGS_CONSUMER_RELEASE} )

	# Flags used by the C++ compiler
	SET( BW_CXX_FLAGS ${BW_COMPILER_FLAGS}
		# Language
		/GR		# Enable Runtime Type Information
		
		# Code generation
		/EHsc	# Enable C++ exceptions
		
		# Additional options
		/w34263 # Enable virtual function is hidden warning at /W3
		
		# Fix for errors in Visual Studio 2012
		# "c1xx : fatal error C1027: Inconsistent values for /Ym between creation
		# and use of precompiled header"
		# http://www.ogre3d.org/forums/viewtopic.php?f=2&t=60015
		/Zm282 
		)
	SET( BW_CXX_FLAGS_DEBUG ${BW_COMPILER_FLAGS_DEBUG} )
	SET( BW_CXX_FLAGS_HYBRID ${BW_COMPILER_FLAGS_HYBRID} )
	SET( BW_CXX_FLAGS_CONSUMER_RELEASE ${BW_COMPILER_FLAGS_CONSUMER_RELEASE} )


	### MSVC Linker Flags ###

	# Flags used by the linker for all build types
	SET( BW_LINKER_FLAGS )

	# Flags used by the linker for different arch types
	IF( CMAKE_SIZEOF_VOID_P EQUAL 4 )
		LIST( APPEND BW_LINKER_FLAGS
			/MACHINE:X86 )
	ELSEIF( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		LIST( APPEND BW_LINKER_FLAGS
			/MACHINE:X64 )
	ENDIF()

	# Flags used by the linker for Debug builds
	SET( BW_LINKER_FLAGS_DEBUG
		# General
		/INCREMENTAL	# Enable incremental linking
		
		# Debugging
		/DEBUG			# Generate debug info
		)

	# Flags used by the linker for optimized builds
	SET( BW_LINKER_FLAGS_OPTIMIZED
		# General
		/INCREMENTAL:NO	# Disable incremental linking
		
		# Debugging
		/DEBUG			# Generate debug info
		
		# Optimization
		/OPT:REF		# Eliminate unreferenced functions and data
		/OPT:ICF		# Enable COMDAT folding
		)
	SET( BW_LINKER_FLAGS_HYBRID ${BW_LINKER_FLAGS_OPTIMIZED} )
	SET( BW_LINKER_FLAGS_CONSUMER_RELEASE ${BW_LINKER_FLAGS_OPTIMIZED} )

	# Set up variables for EXE, MODULE, SHARED, and STATIC linking

	# Flags used by the linker
	SET( BW_EXE_LINKER_FLAGS ${BW_LINKER_FLAGS} )
	SET( BW_EXE_LINKER_FLAGS_DEBUG ${BW_LINKER_FLAGS_DEBUG} )
	SET( BW_EXE_LINKER_FLAGS_HYBRID ${BW_LINKER_FLAGS_HYBRID} )
	SET( BW_EXE_LINKER_FLAGS_CONSUMER_RELEASE ${BW_LINKER_FLAGS_CONSUMER_RELEASE} )

	# Flags used by the linker for the creation of modules.
	SET( BW_MODULE_LINKER_FLAGS ${BW_LINKER_FLAGS} )
	SET( BW_MODULE_LINKER_FLAGS_DEBUG ${BW_LINKER_FLAGS_DEBUG} )
	SET( BW_MODULE_LINKER_FLAGS_HYBRID ${BW_LINKER_FLAGS_HYBRID} )
	SET( BW_MODULE_LINKER_FLAGS_CONSUMER_RELEASE ${BW_LINKER_FLAGS_CONSUMER_RELEASE} )

	# Flags used by the linker for the creation of dll's.
	SET( BW_SHARED_LINKER_FLAGS ${BW_LINKER_FLAGS} )
	SET( BW_SHARED_LINKER_FLAGS_DEBUG ${BW_LINKER_FLAGS_DEBUG} )
	SET( BW_SHARED_LINKER_FLAGS_HYBRID ${BW_LINKER_FLAGS_HYBRID} )
	SET( BW_SHARED_LINKER_FLAGS_CONSUMER_RELEASE ${BW_LINKER_FLAGS_CONSUMER_RELEASE} )

	# Flags used by the linker for the creation of static libraries.
	SET( BW_STATIC_LINKER_FLAGS ${BW_LINKER_FLAGS} )
	SET( BW_STATIC_LINKER_FLAGS_DEBUG "" )
	SET( BW_STATIC_LINKER_FLAGS_HYBRID "" )
	SET( BW_STATIC_LINKER_FLAGS_CONSUMER_RELEASE "" )


	### MSVC Resource Compiler Flags ###

	# Flags for the resource compiler
	SET( BW_RC_FLAGS
		/nologo		# Suppress startup banner
		)


	IF( ${CMAKE_GENERATOR} STREQUAL "Ninja" )
		# Force compiling against XP with Ninja
		# - see http://blogs.msdn.com/b/vcblog/archive/2012/10/08/windows-xp-targeting-with-c-in-visual-studio-2012.aspx
		ADD_DEFINITIONS( -D_USING_V110_SDK71_ )
		IF( ${BW_PLATFORM} STREQUAL "win32" )
			SET( CMAKE_CREATE_WIN32_EXE "/subsystem:windows,5.01" )
		ELSEIF( ${BW_PLATFORM} STREQUAL "win64" )
			SET( CMAKE_CREATE_WIN32_EXE "/subsystem:windows,5.02" )
		ENDIF()
	ENDIF()

ELSE()
	# TODO: Add compiler and linker support for other compilers/platforms here
	# TODO: Split into per compiler files for the BW_* compiler and linker settings
	MESSAGE( FATAL "Unsupported compiler" )
ENDIF()

# Define a _${_CONFIG_NAME} preprocessor variable (e.g. _DEBUG)
SET_PROPERTY( DIRECTORY APPEND PROPERTY
	COMPILE_DEFINITIONS_DEBUG "_DEBUG" )
SET_PROPERTY( DIRECTORY APPEND PROPERTY
	COMPILE_DEFINITIONS_HYBRID "_HYBRID" )
SET_PROPERTY( DIRECTORY APPEND PROPERTY
	COMPILE_DEFINITIONS_CONSUMER_RELEASE "_CONSUMER_RELEASE" )

### Overide all CMake compile and link flags
### Don't modify these, modify the BW versions

FUNCTION( _BW_SET_LIST CACHENAME VALUE )
	# replace ';' separated list items with ' '
	STRING( REPLACE ";" " " TEMPVAR "${VALUE}" )
	SET( "${CACHENAME}" "${TEMPVAR}" ${ARGN} )
ENDFUNCTION()

_BW_SET_LIST( CMAKE_C_FLAGS "${BW_C_FLAGS}"
	CACHE STRING "Flags used by the compiler for all build types" FORCE )
_BW_SET_LIST( CMAKE_C_FLAGS_DEBUG "${BW_C_FLAGS_DEBUG}"
	CACHE STRING "Flags used by the compiler for all Debug builds" FORCE )
_BW_SET_LIST( CMAKE_C_FLAGS_HYBRID "${BW_C_FLAGS_HYBRID}"
	CACHE STRING "Flags used by the compiler for all Hybrid builds" FORCE )
_BW_SET_LIST( CMAKE_C_FLAGS_CONSUMER_RELEASE "${BW_C_FLAGS_CONSUMER_RELEASE}"
	CACHE STRING "Flags used by the compiler for all Consumer_Release builds" FORCE )

_BW_SET_LIST( CMAKE_CXX_FLAGS "${BW_CXX_FLAGS}"
	CACHE STRING "Flags used by the compiler for all build types" FORCE )
_BW_SET_LIST( CMAKE_CXX_FLAGS_DEBUG "${BW_CXX_FLAGS_DEBUG}"
	CACHE STRING "Flags used by the compiler for all Debug builds" FORCE )
_BW_SET_LIST( CMAKE_CXX_FLAGS_HYBRID "${BW_CXX_FLAGS_HYBRID}"
	CACHE STRING "Flags used by the compiler for all Hybrid builds" FORCE )
_BW_SET_LIST( CMAKE_CXX_FLAGS_CONSUMER_RELEASE "${BW_CXX_FLAGS_CONSUMER_RELEASE}"
	CACHE STRING "Flags used by the compiler for all Consumer_Release builds" FORCE )

_BW_SET_LIST( CMAKE_EXE_LINKER_FLAGS "${BW_EXE_LINKER_FLAGS}"
	CACHE STRING "Flags used by the linker" FORCE )
_BW_SET_LIST( CMAKE_EXE_LINKER_FLAGS_DEBUG "${BW_EXE_LINKER_FLAGS_DEBUG}"
	CACHE STRING "Flags used by the linker for Debug builds" FORCE )
_BW_SET_LIST( CMAKE_EXE_LINKER_FLAGS_HYBRID "${BW_EXE_LINKER_FLAGS_HYBRID}"
	CACHE STRING "Flags used by the linker for Hybrid builds" FORCE )
_BW_SET_LIST( CMAKE_EXE_LINKER_FLAGS_CONSUMER_RELEASE "${BW_EXE_LINKER_FLAGS_CONSUMER_RELEASE}"
	CACHE STRING "Flags used by the linker for Consumer_Release builds" FORCE )

_BW_SET_LIST( CMAKE_MODULE_LINKER_FLAGS "${BW_MODULE_LINKER_FLAGS}"
	CACHE STRING "Flags used by the linker for the creation of modules" FORCE )
_BW_SET_LIST( CMAKE_MODULE_LINKER_FLAGS_DEBUG "${BW_MODULE_LINKER_FLAGS_DEBUG}"
	CACHE STRING "Flags used by the linker for Debug builds" FORCE )
_BW_SET_LIST( CMAKE_MODULE_LINKER_FLAGS_HYBRID "${BW_MODULE_LINKER_FLAGS_HYBRID}"
	CACHE STRING "Flags used by the linker for Hybrid builds" FORCE )
_BW_SET_LIST( CMAKE_MODULE_LINKER_FLAGS_CONSUMER_RELEASE "${BW_MODULE_LINKER_FLAGS_CONSUMER_RELEASE}"
	CACHE STRING "Flags used by the linker for Consumer_Release builds" FORCE )

_BW_SET_LIST( CMAKE_SHARED_LINKER_FLAGS "${BW_SHARED_LINKER_FLAGS}"
	CACHE STRING "Flags used by the linker for the creation of dll's" FORCE )
_BW_SET_LIST( CMAKE_SHARED_LINKER_FLAGS_DEBUG "${BW_SHARED_LINKER_FLAGS_DEBUG}"
	CACHE STRING "Flags used by the linker for Debug builds" FORCE )
_BW_SET_LIST( CMAKE_SHARED_LINKER_FLAGS_HYBRID "${BW_SHARED_LINKER_FLAGS_HYBRID}"
	CACHE STRING "Flags used by the linker for Hybrid builds" FORCE )
_BW_SET_LIST( CMAKE_SHARED_LINKER_FLAGS_CONSUMER_RELEASE "${BW_SHARED_LINKER_FLAGS_CONSUMER_RELEASE}"
	CACHE STRING "Flags used by the linker for Consumer_Release builds" FORCE )

_BW_SET_LIST( CMAKE_STATIC_LINKER_FLAGS "${BW_STATIC_LINKER_FLAGS}"
	CACHE STRING "Flags used by the linker for the creation of static libraries" FORCE )
_BW_SET_LIST( CMAKE_STATIC_LINKER_FLAGS_DEBUG "${BW_STATIC_LINKER_FLAGS_DEBUG}"
	CACHE STRING "Flags used by the linker for Debug builds" FORCE )
_BW_SET_LIST( CMAKE_STATIC_LINKER_FLAGS_HYBRID "${BW_STATIC_LINKER_FLAGS_HYBRID}"
	CACHE STRING "Flags used by the linker for Hybrid builds" FORCE )
_BW_SET_LIST( CMAKE_STATIC_LINKER_FLAGS_CONSUMER_RELEASE "${BW_STATIC_LINKER_FLAGS_CONSUMER_RELEASE}"
	CACHE STRING "Flags used by the linker for Consumer_Release builds" FORCE )

_BW_SET_LIST( CMAKE_RC_FLAGS "${BW_RC_FLAGS}"
	CACHE STRING "Flags used by the resource compiler" FORCE )

IF( CMAKE_VERBOSE )
	MESSAGE( STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}" )
	MESSAGE( STATUS "CMAKE_C_FLAGS_DEBUG: ${CMAKE_C_FLAGS_DEBUG}" )
	MESSAGE( STATUS "CMAKE_C_FLAGS_HYBRID: ${CMAKE_C_FLAGS_HYBRID}" )
	MESSAGE( STATUS "CMAKE_C_FLAGS_CONSUMER_RELEASE: ${CMAKE_C_FLAGS_CONSUMER_RELEASE}" )
	MESSAGE( STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}" )
	MESSAGE( STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}" )
	MESSAGE( STATUS "CMAKE_CXX_FLAGS_HYBRID: ${CMAKE_CXX_FLAGS_HYBRID}" )
	MESSAGE( STATUS "CMAKE_CXX_FLAGS_CONSUMER_RELEASE: ${CMAKE_CXX_FLAGS_CONSUMER_RELEASE}" )
	MESSAGE( STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}" )
	MESSAGE( STATUS "CMAKE_EXE_LINKER_FLAGS_DEBUG: ${CMAKE_EXE_LINKER_FLAGS_DEBUG}" )
	MESSAGE( STATUS "CMAKE_EXE_LINKER_FLAGS_HYBRID: ${CMAKE_EXE_LINKER_FLAGS_HYBRID}" )
	MESSAGE( STATUS "CMAKE_EXE_LINKER_FLAGS_CONSUMER_RELEASE: ${CMAKE_EXE_LINKER_FLAGS_CONSUMER_RELEASE}" )
	MESSAGE( STATUS "CMAKE_MODULE_LINKER_FLAGS: ${CMAKE_MODULE_LINKER_FLAGS}" )
	MESSAGE( STATUS "CMAKE_MODULE_LINKER_FLAGS_DEBUG: ${CMAKE_MODULE_LINKER_FLAGS_DEBUG}" )
	MESSAGE( STATUS "CMAKE_MODULE_LINKER_FLAGS_HYBRID: ${CMAKE_MODULE_LINKER_FLAGS_HYBRID}" )
	MESSAGE( STATUS "CMAKE_MODULE_LINKER_FLAGS_CONSUMER_RELEASE: ${CMAKE_MODULE_LINKER_FLAGS_CONSUMER_RELEASE}" )
	MESSAGE( STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}" )
	MESSAGE( STATUS "CMAKE_SHARED_LINKER_FLAGS_DEBUG: ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}" )
	MESSAGE( STATUS "CMAKE_SHARED_LINKER_FLAGS_HYBRID: ${CMAKE_SHARED_LINKER_FLAGS_HYBRID}" )
	MESSAGE( STATUS "CMAKE_SHARED_LINKER_FLAGS_CONSUMER_RELEASE: ${CMAKE_SHARED_LINKER_FLAGS_CONSUMER_RELEASE}" )
	MESSAGE( STATUS "CMAKE_STATIC_LINKER_FLAGS: ${CMAKE_STATIC_LINKER_FLAGS}" )
	MESSAGE( STATUS "CMAKE_STATIC_LINKER_FLAGS_DEBUG: ${CMAKE_STATIC_LINKER_FLAGS_DEBUG}" )
	MESSAGE( STATUS "CMAKE_STATIC_LINKER_FLAGS_HYBRID: ${CMAKE_STATIC_LINKER_FLAGS_HYBRID}" )
	MESSAGE( STATUS "CMAKE_STATIC_LINKER_FLAGS_CONSUMER_RELEASE: ${CMAKE_STATIC_LINKER_FLAGS_CONSUMER_RELEASE}" )
	MESSAGE( STATUS "CMAKE_RC_FLAGS: ${CMAKE_RC_FLAGS}" )
ENDIF()

MARK_AS_ADVANCED(
	CMAKE_C_FLAGS_DEBUG
	CMAKE_C_FLAGS_HYBRID
	CMAKE_C_FLAGS_CONSUMER_RELEASE
	CMAKE_CXX_FLAGS_DEBUG
	CMAKE_CXX_FLAGS_HYBRID
	CMAKE_CXX_FLAGS_CONSUMER_RELEASE
	CMAKE_EXE_LINKER_FLAGS_DEBUG
	CMAKE_EXE_LINKER_FLAGS_HYBRID
	CMAKE_EXE_LINKER_FLAGS_CONSUMER_RELEASE
	CMAKE_MODULE_LINKER_FLAGS_DEBUG
	CMAKE_MODULE_LINKER_FLAGS_HYBRID
	CMAKE_MODULE_LINKER_FLAGS_CONSUMER_RELEASE
	CMAKE_SHARED_LINKER_FLAGS_DEBUG
	CMAKE_SHARED_LINKER_FLAGS_HYBRID
	CMAKE_SHARED_LINKER_FLAGS_CONSUMER_RELEASE
	CMAKE_STATIC_LINKER_FLAGS_DEBUG
	CMAKE_STATIC_LINKER_FLAGS_HYBRID
	CMAKE_STATIC_LINKER_FLAGS_CONSUMER_RELEASE
	CMAKE_RC_FLAGS
)
