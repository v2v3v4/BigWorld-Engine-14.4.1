CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( baseapp )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp 
	add_to_baseappmgr_helper.hpp
	address_load_pair.hpp
	archiver.cpp 
	archiver.hpp 
	backed_up_base_app.cpp 
	backed_up_base_app.hpp 
	backed_up_base_apps.cpp 
	backed_up_base_apps.hpp 
	backup_sender.cpp 
	backup_sender.hpp 
	base.cpp 
	base.hpp 
	base_message_forwarder.cpp 
	base_message_forwarder.hpp 
	baseapp.cpp 
	baseapp.hpp 
	baseapp.ipp
	baseapp_config.cpp 
	baseapp_config.hpp 
	initial_connection_filter.cpp
	initial_connection_filter.hpp
	baseapp_int_interface.cpp
	baseapp_int_interface.hpp
	baseappmgr_gateway.cpp 
	baseappmgr_gateway.hpp 
	bases.cpp 
	bases.hpp 
	bwtracer.cpp 
	bwtracer.hpp 
	client_entity_mailbox.cpp 
	client_entity_mailbox.hpp 
	client_entity_mailbox_wrapper.cpp
	client_entity_mailbox_wrapper.hpp
	client_stream_filter_factory.hpp
	controlled_shutdown_handler.cpp 
	controlled_shutdown_handler.hpp 
	create_cell_entity_handler.cpp 
	create_cell_entity_handler.hpp 
	data_downloads.cpp 
	data_downloads.hpp 
	dead_cell_apps.cpp 
	dead_cell_apps.hpp 
	download_streamer.cpp 
	download_streamer.hpp 
	download_streamer_config.cpp 
	download_streamer_config.hpp 
	eg_tcpecho.cpp 
	entity_channel_finder.cpp 
	entity_channel_finder.hpp 
	entity_creator.cpp 
	entity_creator.hpp 
	entity_type.cpp 
	entity_type.hpp 
	external_interfaces.cpp 
	global_bases.cpp 
	global_bases.hpp 
	id_config.cpp 
	id_config.hpp 
	load_entity_handler.cpp 
	load_entity_handler.hpp 
	loading_thread.cpp 
	loading_thread.hpp 
	login_handler.cpp 
	login_handler.hpp 
	mailbox.cpp 
	mailbox.hpp 
	message_handlers.cpp 
	message_handlers.hpp 
	offloaded_backups.cpp 
	offloaded_backups.hpp 
	pending_logins.cpp 
	pending_logins.hpp 
	ping_manager.cpp 
	ping_manager.hpp 
	proxy.cpp 
	proxy.hpp 
	py_bases.cpp 
	py_bases.hpp 
	py_cell_data.cpp 
	py_cell_data.hpp 
	py_cell_spatial_data.cpp 
	py_cell_spatial_data.hpp 
	py_replay_data_file_reader.cpp
	py_replay_data_file_reader.hpp
	py_replay_data_file_writer.cpp 
	py_replay_data_file_writer.hpp 
	py_replay_header.cpp
	py_replay_header.hpp
	rate_limit_config.cpp 
	rate_limit_config.hpp 
	rate_limit_message_filter.cpp 
	rate_limit_message_filter.hpp 
	recording_recovery_data.hpp
	remote_client_method.cpp
	remote_client_method.hpp
	replay_data_file_writer.cpp 
	replay_data_file_writer.hpp 
	script_bigworld.cpp 
	script_bigworld.hpp 
	service.cpp 
	service.hpp 
	service_app.hpp
	service_starter.cpp 
	service_starter.hpp 
	shared_data_manager.cpp 
	shared_data_manager.hpp 
	sqlite_database.cpp 
	sqlite_database.hpp 
	worker_thread.cpp 
	worker_thread.hpp 
	write_to_db_reply.cpp 
	write_to_db_reply.hpp 
	../../common/py_network.cpp 
	../../common/py_network.hpp 
	../../common/py_server_lib.cpp 
	../../common/py_timer.cpp 
	../../common/py_timer.hpp 
	../../common/py_urlrequest.cpp
	baseapp_int_interface.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( baseapp ${ALL_SRCS} )

TARGET_INCLUDE_DIRECTORIES( baseapp PRIVATE ${BW_SOURCE_DIR} )
TARGET_INCLUDE_DIRECTORIES( baseapp PRIVATE ${BW_SOURCE_DIR}/server )

BW_TARGET_LINK_LIBRARIES( baseapp
	chunk
	scene
	connection
	entitydef
	entitydef_script
	server
	script
	pyscript
	sqlite
	delegate_interface
	urlrequest
	network
	resmgr
	math
	cstdmf
	zip
	db
)

BW_PROJECT_CATEGORY( baseapp "Executables" )
