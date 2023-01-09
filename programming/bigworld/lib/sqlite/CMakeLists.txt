CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( sqlite )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	sqlite_connection.cpp
	sqlite_connection.hpp
	sqlite_statement.cpp
	sqlite_statement.hpp
	../../third_party/sqlite/sqlite3.c
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( sqlite
	${ALL_SRCS}
)

BW_PROJECT_CATEGORY( sqlite "BigWorld Server" )
