CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( web_render )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

# Disable optimisations for Awesomium
# so that the library can be delay loaded
IF (BW_AWESOMIUM_SUPPORT AND MSVC)
	STRING( REGEX REPLACE "/Ox" "/Od" 
		CMAKE_CXX_FLAGS_HYBRID "${CMAKE_CXX_FLAGS_HYBRID}" )

	STRING( REGEX REPLACE "/Ox" "/Od" 
		CMAKE_CXX_FLAGS_CONSUMER_RELEASE "${CMAKE_CXX_FLAGS_CONSUMER_RELEASE}" )
ENDIF()


IF (BW_AWESOMIUM_SUPPORT)
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/awesomium/include )
ENDIF()

SET( ALL_SRCS
	awesomium_provider.cpp
	awesomium_provider.hpp
	pch.cpp
	pch.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( web_render
	${ALL_SRCS}
)

BW_TARGET_LINK_LIBRARIES( web_render INTERFACE
	cstdmf
	moo
	resmgr
	)

IF (BW_AWESOMIUM_SUPPORT)
	TARGET_COMPILE_DEFINITIONS( web_render PUBLIC "BW_AWESOMIUM_SUPPORT=1" )
ELSE()
	TARGET_COMPILE_DEFINITIONS( web_render PUBLIC "BW_AWESOMIUM_SUPPORT=0" )
ENDIF()

BW_PRECOMPILED_HEADER( web_render pch.hpp )
BW_PROJECT_CATEGORY( web_render "BigWorld Libs" )
