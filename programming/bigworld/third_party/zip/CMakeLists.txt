CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( zip )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

BW_REMOVE_COMPILE_FLAGS( /WX )

ADD_DEFINITIONS(-DMY_ZCALLOC=1)

SET( ALL_SRCS
	adler32.c
	compress.c
	crc32.c
	deflate.c
	deflate.h
	gzread.c
	gzwrite.c
	infback.c
	inffast.c
	inffast.h
	inffixed.h
	inflate.c
	inftrees.c
	inftrees.h
	trees.c
	trees.h
	uncompr.c
	zconf.h
	zlib.h
	zutil.c
	zutil.h
	bw_zlib_mem.cpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( zip
	${ALL_SRCS}
)

BW_PROJECT_CATEGORY( zip "ThirdParty" )
