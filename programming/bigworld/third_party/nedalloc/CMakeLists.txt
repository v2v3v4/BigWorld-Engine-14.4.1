CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( nedalloc )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

BW_REMOVE_COMPILE_FLAGS( /WX )


SET( ALL_SRCS
	malloc.c.h
	nedmalloc.c
	nedmalloc.h
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )




ADD_LIBRARY( nedalloc 
	${ALL_SRCS}
)

BW_PROJECT_CATEGORY( nedalloc "ThirdParty" )
