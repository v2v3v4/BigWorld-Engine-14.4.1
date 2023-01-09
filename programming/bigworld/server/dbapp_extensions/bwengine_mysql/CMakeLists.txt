CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( bwengine_mysql )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	mysql_engine_creator.cpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( bwengine_mysql MODULE
	${ALL_SRCS}
)

BW_PROJECT_CATEGORY( bwengine_mysql "DBApp Extensions" )
