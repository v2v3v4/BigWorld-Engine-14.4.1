CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( unit_test_lib )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )




SET( ALL_SRCS
	TestResultBWOut.cpp
	TestResultBWOut.hpp
	multi_proc_test_case.cpp
	multi_proc_test_case.hpp
	pch.cpp
	pch.hpp
	unit_test.cpp
	unit_test.hpp
	base_resmgr_unit_test_harness.cpp
	base_resmgr_unit_test_harness.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


BW_ADD_LIBRARY( unit_test_lib 
	${ALL_SRCS}
)

IF( BW_USE_BWENTITY_DLL )
	# bwentity.dll includes both cstdmf and resmgr
	SET( RESMGR_CSTDMF_TARGET_LIBS bwentity )
ELSE()
	SET( RESMGR_CSTDMF_TARGET_LIBS cstdmf resmgr )
ENDIF()

BW_TARGET_LINK_LIBRARIES( unit_test_lib ${RESMGR_CSTDMF_TARGET_LIBS} CppUnitLite2 )

BW_PRECOMPILED_HEADER( unit_test_lib pch.hpp )
BW_PROJECT_CATEGORY( unit_test_lib "Unit Tests" )
