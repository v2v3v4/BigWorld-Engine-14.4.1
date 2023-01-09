CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( test_compiler )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	test_compiler.cpp
	test_compiler.hpp
	../../../plugin_system/plugin.hpp
	../../../plugin_system/plugin_loader.cpp
	../../../plugin_system/plugin_loader.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( test_compiler 
	${ALL_SRCS}
)

BW_TARGET_LINK_LIBRARIES( test_compiler PRIVATE
	compiler
	conversion
	cstdmf
	dependency
	discovery
	resmgr
	)

BW_PROJECT_CATEGORY( test_compiler "Unit Tests/Asset Pipeline" )
BW_COPY_TARGET( test_compiler cstdmf )
