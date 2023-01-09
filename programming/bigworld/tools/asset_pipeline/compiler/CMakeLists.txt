CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( compiler )

ADD_DEFINITIONS( 
	-DBW_ASSET_PIPELINE
)

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	asset_compiler.cpp
	asset_compiler.hpp
	asset_compiler_options.cpp
	asset_compiler_options.hpp
	generic_conversion_rule.cpp
	generic_conversion_rule.hpp
	resource_callbacks.cpp
	resource_callbacks.hpp
	compiler.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


BW_ADD_LIBRARY( compiler
	${ALL_SRCS}
)

BW_TARGET_LINK_LIBRARIES( compiler
	INTERFACE
	conversion
	cstdmf
	dependency
	discovery
	re2
	resmgr
	)

TARGET_INCLUDE_DIRECTORIES( compiler PRIVATE
	"${BW_SOURCE_DIR}/third_party/re2" )

BW_PROJECT_CATEGORY( compiler "Asset Pipeline" )
