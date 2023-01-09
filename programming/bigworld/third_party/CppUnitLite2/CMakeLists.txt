CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( CppUnitLite2 )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

BW_REMOVE_COMPILE_FLAGS( /WX )



SET( ALL_SRCS
	src/CppUnitLite2.h
	src/ExceptionHandler.cpp
	src/ExceptionHandler.h
	src/Failure.cpp
	src/Failure.h
	src/MockHelpers.h
	src/Test.cpp
	src/Test.h
	src/TestException.cpp
	src/TestException.h
	src/TestMacros.h
	src/TestRegistry.cpp
	src/TestRegistry.h
	src/TestResult.cpp
	src/TestResult.h
	src/TestResultStdErr.cpp
	src/TestResultStdErr.h
	src/Win32/TestResultDebugOut.cpp
	src/Win32/TestResultDebugOut.h
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )




ADD_LIBRARY( CppUnitLite2 
	${ALL_SRCS}
)

BW_PROJECT_CATEGORY( CppUnitLite2 "Unit Tests" )
