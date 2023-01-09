CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( transfer_db )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	consolidate.cpp
	consolidate.hpp
	main.cpp
	snapshot.cpp
	snapshot.hpp
	transfer_db.cpp
	transfer_db.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( transfer_db ${ALL_SRCS} )

BW_TARGET_LINK_LIBRARIES( transfer_db
	db
	server
	sqlite
)

BW_PROJECT_CATEGORY( transfer_db "Tools" )
