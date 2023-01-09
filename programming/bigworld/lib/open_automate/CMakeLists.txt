CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( open_automate )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

BW_REMOVE_COMPILE_FLAGS( /WX )


SET( ALL_SRCS
	${BW_SOURCE_DIR}/third_party/openautomate/inc/OpenAutomate.h
	${BW_SOURCE_DIR}/third_party/openautomate/inc/OpenAutomate_Internal.h
	${BW_SOURCE_DIR}/third_party/openautomate/oautillib/OpenAutomateUtil.c
	${BW_SOURCE_DIR}/third_party/openautomate/oautillib/OpenAutomateUtil.h
	${BW_SOURCE_DIR}/third_party/openautomate/src/OpenAutomate.c
	open_automate_tester.cpp
	open_automate_tester.hpp
	open_automate_wrapper.cpp
	open_automate_wrapper.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( open_automate
	${ALL_SRCS}
)

TARGET_INCLUDE_DIRECTORIES( open_automate
	PRIVATE
	"${BW_SOURCE_DIR}/third_party/openautomate/oautillib"
	PUBLIC
	"${BW_SOURCE_DIR}/third_party/openautomate/inc"
)

BW_PROJECT_CATEGORY( open_automate "BigWorld Libs" )
