CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( _bw_profile )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	bw_profile.cpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( _bw_profile MODULE ${ALL_SRCS} )


BW_TARGET_LINK_LIBRARIES( _bw_profile
	cstdmf
)

BW_PROJECT_CATEGORY( _bw_profile "Tools" )
