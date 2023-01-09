CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( bwlauncher )

INCLUDE( BWStandardProject )

SET( RESOURCE_FILES_SRCS
	bwlauncher.rc
	res/bwlauncher.ico
)
SOURCE_GROUP( "Resource_Files" FILES ${RESOURCE_FILES_SRCS} )

SET( SOURCE_FILES_SRCS
	bwlauncher.cpp
	bwlauncher_config.cpp
	bwlauncher_config.hpp
	launch_db.cpp
	launch_db.hpp
	pch.cpp
	pch.hpp
	utils.cpp
	utils.hpp
)
SOURCE_GROUP( "Source_Files" FILES ${SOURCE_FILES_SRCS} )

SET( TINYXML_SRCS
	tinystr.cpp
	tinystr.h
	tinyxml.cpp
	tinyxml.h
	tinyxmlerror.cpp
	tinyxmlparser.cpp
)
SOURCE_GROUP( "tinyxml" FILES ${TINYXML_SRCS} )


BW_ADD_EXECUTABLE( bwlauncher WIN32
	${RESOURCE_FILES_SRCS}
	${SOURCE_FILES_SRCS}
	${TINYXML_SRCS}
)

BW_PROJECT_CATEGORY( bwlauncher "Executables" )