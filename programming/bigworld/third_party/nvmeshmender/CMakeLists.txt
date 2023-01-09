CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( nvmeshmender )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

BW_REMOVE_COMPILE_FLAGS( /WX )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/lib )

SET( ALL_SRCS
	nvmeshmender.cpp
	nvmeshmender.h
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

ADD_LIBRARY( nvmeshmender
	${ALL_SRCS}
)

BW_PROJECT_CATEGORY( nvmeshmender "ThirdParty" )

