CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( sync_db )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
	mysql_dry_run_connection.cpp
	mysql_dry_run_connection.hpp
	mysql_dry_run_wrapper.cpp
	mysql_dry_run_wrapper.hpp
	mysql_synchronise.cpp
	mysql_synchronise.hpp
	mysql_table_initialiser.cpp
	mysql_table_initialiser.hpp
	mysql_upgrade_database.cpp
	mysql_upgrade_database.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( sync_db ${ALL_SRCS} )

BW_TARGET_LINK_LIBRARIES( sync_db
	chunk
	scene
	db_storage
	db_storage_mysql
	entitydef
	pyscript
	script
	server
)

BW_PROJECT_CATEGORY( sync_db "Tools" )
