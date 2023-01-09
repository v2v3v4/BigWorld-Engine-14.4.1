CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( dbapp )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp 
	authenticate_account_handler.cpp 
	authenticate_account_handler.hpp 
	consolidator.cpp 
	consolidator.hpp 
	custom.cpp 
	custom.hpp 
	dbapp.cpp 
	dbapp.hpp 
	dbapp.ipp
	delete_entity_handler.cpp 
	delete_entity_handler.hpp 
	dbapp_config.cpp 
	dbapp_config.hpp 
	dbappmgr_gateway.cpp
	dbappmgr_gateway.hpp
	entity_auto_loader.cpp 
	entity_auto_loader.hpp 
	get_entity_handler.cpp 
	get_entity_handler.hpp load_entity_handler.cpp 
	load_entity_handler.hpp 
	login_app_check_status_reply_handler.cpp 
	login_app_check_status_reply_handler.hpp 
	login_conditions_config.cpp 
	login_conditions_config.hpp 
	login_handler.cpp 
	login_handler.hpp 
	log_on_records_cache.cpp
	log_on_records_cache.hpp
	look_up_dbid_handler.cpp 
	look_up_dbid_handler.hpp 
	look_up_entity_handler.cpp 
	look_up_entity_handler.hpp 
	look_up_entities_handler.cpp 
	look_up_entities_handler.hpp 
	message_handlers.cpp 
	relogon_attempt_handler.cpp 
	relogon_attempt_handler.hpp 
	write_entity_handler.cpp 
	write_entity_handler.hpp 
	../baseappmgr/baseappmgr_interface.cpp 
	../baseappmgr/baseappmgr_interface.hpp 
	../baseapp/baseapp_int_interface.cpp 
	../baseapp/baseapp_int_interface.hpp 
	../../common/py_network.cpp 
	../../common/py_network.hpp 
	../../common/py_server_lib.cpp 
	../../common/py_urlrequest.cpp
	../../lib/db/dbappmgr_interface.cpp 
	../../lib/db/dbappmgr_interface.hpp 
	../../lib/db/dbapp_interface.cpp 
	../../lib/db/dbapp_interface.hpp 
)

SOURCE_GROUP( "" FILES ${ALL_SRCS} )

SET ( BILLING_SYSTEM_SRCS
	standard_billing_system.cpp 
	py_billing_system.cpp 
	py_billing_system.hpp 
	custom_billing_system.cpp 
	custom_billing_system.hpp 
)

SOURCE_GROUP( "Billing systems" FILES ${BILLING_SYSTEM_SRCS} )

BW_ADD_EXECUTABLE( dbapp ${ALL_SRCS} ${BILLING_SYSTEM_SRCS} )

TARGET_INCLUDE_DIRECTORIES( dbapp PRIVATE ${BW_SOURCE_DIR} )
TARGET_INCLUDE_DIRECTORIES( dbapp PRIVATE ${BW_SOURCE_DIR}/server )

BW_TARGET_LINK_LIBRARIES( dbapp
	db
	db_storage
	server
	chunk
	entitydef
	pyscript
	script
	connection
	urlrequest
	network
	resmgr
	math
	cstdmf
	zip
)

BW_PROJECT_CATEGORY( dbapp "Executables" )
