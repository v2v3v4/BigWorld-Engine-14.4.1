CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( discovery )

ADD_DEFINITIONS( 
	-DBW_ASSET_PIPELINE
)

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	conversion_rule.hpp
	conversion_rules.hpp
	task_finder.cpp
	task_finder.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


BW_ADD_LIBRARY( discovery
	${ALL_SRCS}
)

BW_TARGET_LINK_LIBRARIES( discovery INTERFACE
	compiler
	conversion
	cstdmf
	resmgr
	)

BW_PROJECT_CATEGORY( discovery "Asset Pipeline" )