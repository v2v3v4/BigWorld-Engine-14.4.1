CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( bwengine_xml )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	xml_engine_creator.cpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( bwengine_xml MODULE
	${ALL_SRCS}
)

BW_PROJECT_CATEGORY( bwengine_xml "DBApp Extensions" )
