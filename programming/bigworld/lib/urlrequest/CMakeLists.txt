CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( urlrequest )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )


SET( ALL_SRCS
	manager.cpp
	manager.hpp
	misc.hpp
	request.cpp
	request.hpp
	response.cpp
	response.hpp
	string_list.cpp
	string_list.hpp

)

SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( urlrequest
	${BLOB_SRCS}
)

FIND_PACKAGE( BWcurl REQUIRED )

IF( BWCURL_FOUND )
	ADD_DEFINITIONS( -DCURL_STATICLIB )
	INCLUDE_DIRECTORIES( ${BWCURL_INCLUDE_DIRS} )
	BW_TARGET_LINK_LIBRARIES( urlrequest INTERFACE ${BWCURL_LIBRARIES} )
ENDIF()

BW_TARGET_LINK_LIBRARIES( urlrequest INTERFACE
	cstdmf
	network
	script
	)

BW_PROJECT_CATEGORY( urlrequest "BigWorld Libs" )
