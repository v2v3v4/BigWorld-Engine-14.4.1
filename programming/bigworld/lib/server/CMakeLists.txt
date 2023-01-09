CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( server )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	add_to_manager_helper.cpp 
	add_to_manager_helper.hpp 
	anonymous_channel_client.cpp 
	anonymous_channel_client.hpp 
	app_script_timers.cpp 
	auto_backup_and_archive.cpp 
	auto_backup_and_archive.hpp 
	backup_hash.cpp 
	backup_hash.hpp 
	backup_hash_chain.cpp 
	backup_hash_chain.hpp 
	balance_config.cpp 
	balance_config.hpp 
	base_backup_switch_mailbox_visitor.cpp
	base_backup_switch_mailbox_visitor.hpp
	bwconfig.cpp 
	bwconfig.hpp 
	bwservice.cpp 
	bwservice.hpp 
	cell_app_init_data.hpp
	cell_properties_names.hpp
	child_process.cpp 
	child_process.hpp 
	client_method_calling_flags.hpp
	common.hpp
	config_reader.cpp 
	config_reader.hpp 
	entity_app.cpp 
	entity_app.hpp 
	entity_app_config.cpp 
	entity_app_config.hpp 
	entity_profiler.cpp
	entity_profiler.hpp
	entity_profiler.ipp
	entity_type_profiler.cpp
	entity_type_profiler.hpp
	entity_type_profiler.ipp
	event_history_stats.cpp 
	event_history_stats.hpp 
	external_app_config.cpp 
	external_app_config.hpp 
	find_lowest_interface_handler.cpp
	find_lowest_interface_handler.hpp
	glob.hpp
	id_client.cpp 
	id_client.hpp 
	id_generator.hpp
	look_up_entities.cpp 
	look_up_entities.hpp 
	manager_app.cpp 
	manager_app.hpp 
	manager_app_config.cpp 
	manager_app_config.hpp 
	manager_app_gateway.cpp 
	manager_app_gateway.hpp 
	migrate_mailbox_visitor.hpp
	murmur_hash.cpp
	murmur_hash.hpp
	nat_config.cpp 
	nat_config.hpp 
	plugin_library.cpp 
	plugin_library.hpp
	py_services_map.cpp
	py_services_map.hpp
	python_server.cpp 
	python_server.hpp 
	recording_options.hpp
	rendezvous_hash_scheme.hpp
	reviver_common.hpp
	reviver_subject.cpp 
	reviver_subject.hpp 
	script_app.cpp 
	script_app.hpp 
	script_frequent_tasks.cpp 
	script_frequent_tasks.hpp 
	script_timers.cpp 
	script_timers.hpp
	server_app.cpp 
	server_app.hpp 
	server_app_config.cpp 
	server_app_config.hpp 
	server_app_option.cpp 
	server_app_option.hpp 
	server_app_option_macros.hpp 
	server_info.cpp 
	server_info.hpp 
	services_map.cpp
	services_map.hpp
	shared_data.cpp 
	shared_data.hpp 
	shared_data_type.hpp
	signal_processor.cpp
	signal_processor.hpp
	signal_set.hpp
	stream_helper.hpp
	time_keeper.cpp 
	time_keeper.hpp 
	updatable.hpp
	updatables.cpp 
	updatables.hpp 
	util.hpp
	watcher_forwarding.cpp 
	watcher_forwarding.hpp 
	watcher_forwarding_collector.cpp 
	watcher_forwarding_collector.hpp 
	watcher_forwarding_types.hpp
	watcher_protocol.cpp 
	watcher_protocol.hpp 
	writedb.hpp
)

IF( BW_PLATFORM_WINDOWS )
	LIST( APPEND ALL_SRCS
		service.cpp
		service.hpp
	)
ENDIF()

SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_LIBRARY( server ${ALL_SRCS} )

BW_PROJECT_CATEGORY( server "BigWorld Server" )
