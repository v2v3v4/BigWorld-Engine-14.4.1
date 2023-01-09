CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( db_storage_xml )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	xml_billing_system.cpp
	xml_billing_system.hpp
	xml_database.cpp
	xml_database.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( db_storage_xml
	${ALL_SRCS}
)

BW_PROJECT_CATEGORY( db_storage_xml "BigWorld Server" )