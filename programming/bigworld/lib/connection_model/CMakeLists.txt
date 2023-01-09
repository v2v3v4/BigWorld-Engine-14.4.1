CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( connection_model )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR} )

SET( ALL_SRCS
	active_entities.cpp
	active_entities.hpp
	bw_connection.cpp
	bw_connection.hpp
	bw_connection_helper.hpp
	bw_connection_helper.cpp
	bw_entities.cpp
	bw_entities.hpp
	bw_entities_listener.hpp
	bw_entity.cpp
	bw_entity.hpp
	bw_entity_factory.cpp
	bw_entity_factory.hpp
	bw_null_connection.cpp
	bw_null_connection.hpp
	bw_replay_connection.cpp
	bw_replay_connection.hpp
	bw_replay_event_handler.hpp
	bw_server_connection.cpp
	bw_server_connection.hpp
	bw_server_message_handler.cpp
	bw_server_message_handler.hpp
	bw_space_data_listener.hpp
	bw_space_data_storage.hpp
	bw_stream_data_handler.hpp
	deferred_passengers.cpp
	deferred_passengers.hpp
	entity_entry_blocker.cpp
	entity_entry_blocker.hpp
	entity_entry_blocking_condition.hpp
	entity_entry_blocking_condition_handler.cpp
	entity_entry_blocking_condition_handler.hpp
	entity_entry_blocking_condition_impl.cpp
	entity_entry_blocking_condition_impl.hpp
	entity_extension.cpp
	entity_extension.hpp
	entity_extension_factory_base.cpp
	entity_extension_factory_base.hpp
	entity_extension_factory_manager.cpp
	entity_extension_factory_manager.hpp
	entity_extensions.cpp
	entity_extensions.hpp
	nested_property_change.cpp
	nested_property_change.hpp
	pch.cpp
	pch.hpp
	pending_entities.cpp
	pending_entities.hpp
	pending_entities_blocking_condition_handler.cpp
	pending_entities_blocking_condition_handler.hpp
	sequence_value_type.hpp
	server_entity_mail_box.cpp
	server_entity_mail_box.hpp
	simple_space_data_storage.hpp
	simple_space_data_storage.cpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

IF ( BW_USE_BWENTITY_IN_DLL )
	REMOVE_DEFINITIONS( 
		-DBWENTITY_DLL_IMPORT
		-DCSTDMF_IMPORT
	)

	ADD_DEFINITIONS( 
		-DBWENTITY_DLL_EXPORT
		-DCSTDMF_EXPORT
	)
ENDIF()

BW_ADD_LIBRARY( connection_model
	${ALL_SRCS}
)

BW_TARGET_LINK_LIBRARIES( connection_model INTERFACE
	connection
	cstdmf
	math
	network
	)

BW_PRECOMPILED_HEADER( connection_model pch.hpp )
BW_PROJECT_CATEGORY( connection_model "BigWorld Libs" )
