CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( libpng )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

BW_REMOVE_COMPILE_FLAGS( /WX )



SET( ALL_SRCS
	PRJ0041.mak
	README.txt
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( RESOURCE_FILES_SRCS
	../../scripts/pngwin.rc
)
SOURCE_GROUP( "Resource_Files" FILES ${RESOURCE_FILES_SRCS} )


SET( SOURCE_FILES_SRCS
	../../png.c
	../../pngerror.c
	../../pngget.c
	../../pngmem.c
	../../pngpread.c
	../../pngread.c
	../../pngrio.c
	../../pngrtran.c
	../../pngrutil.c
	../../pngset.c
	../../pngtrans.c
	../../pngwio.c
	../../pngwrite.c
	../../pngwtran.c
	../../pngwutil.c
)
SOURCE_GROUP( "Source_Files" FILES ${SOURCE_FILES_SRCS} )


SET( HEADER_FILES_SRCS
	../../png.h
	../../pngconf.h
)
SOURCE_GROUP( "Header_Files" FILES ${HEADER_FILES_SRCS} )




ADD_LIBRARY( libpng 
	${ALL_SRCS}
	${RESOURCE_FILES_SRCS}
	${SOURCE_FILES_SRCS}
	${HEADER_FILES_SRCS}
)

BW_PROJECT_CATEGORY( libpng "ThirdParty" )
