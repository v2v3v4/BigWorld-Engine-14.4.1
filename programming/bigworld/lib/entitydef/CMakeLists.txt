CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( entitydef )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	base_user_data_object_description.cpp
	base_user_data_object_description.hpp
	base_user_data_object_description.ipp
	constants.hpp
	data_description.cpp
	data_description.hpp
	data_description.ipp
	data_lod_level.cpp
	data_lod_level.hpp
	data_lod_level.ipp
	data_sink.hpp
	data_source.hpp
	data_type.cpp
	data_type.hpp
	data_types.cpp
	data_types.hpp
	entity_description.cpp
	entity_description.hpp
	entity_description.ipp
	entity_description_debug.cpp
	entity_description_debug.hpp
	entity_description_map.cpp
	entity_description_map.hpp
	entity_member_stats.cpp
	entity_member_stats.hpp
	entity_method_descriptions.cpp
	entity_method_descriptions.hpp
	member_description.cpp
	member_description.hpp
	meta_data_type.cpp
	meta_data_type.hpp
	method_args.cpp
	method_args.hpp
	method_description.cpp
	method_description.hpp
	method_description.ipp
	pch.cpp
	pch.hpp
	property_change.cpp
	property_change.hpp
	property_change_reader.cpp
	property_change_reader.hpp
	property_owner.cpp
	property_owner.hpp
	py_deferred.cpp
	py_deferred.hpp
	py_volatile_info.cpp
	py_volatile_info.hpp
	return_values_handler.cpp
	return_values_handler.hpp
	script_data_sink.cpp
	script_data_sink.hpp
	script_data_source.cpp
	script_data_source.hpp
	single_type_data_sinks.cpp
	single_type_data_sinks.hpp
	single_type_data_sources.cpp
	single_type_data_sources.hpp
	user_data_object_description.cpp
	user_data_object_description.hpp
	user_data_object_description.ipp
	user_data_object_description_map.cpp
	user_data_object_description_map.hpp
	volatile_info.cpp
	volatile_info.hpp
)

IF (BW_IS_SERVER)
	LIST( APPEND ALL_SRCS
		entity_delegate_helpers.cpp 
		entity_delegate_helpers.hpp 
		mailbox_base.cpp
		mailbox_base.hpp
		property_event_stamps.cpp
		property_event_stamps.hpp
		remote_entity_method.cpp
		remote_entity_method.hpp
	)
ENDIF()
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

SET( DATA_TYPES_SRCS
	data_types/array_data_type.cpp
	data_types/array_data_type.hpp
	data_types/blob_data_type.cpp
	data_types/blob_data_type.hpp
	data_types/class_data_type.cpp
	data_types/class_data_type.hpp
	data_types/class_meta_data_type.cpp
	data_types/class_meta_data_type.hpp
	data_types/dictionary_data_type.cpp
	data_types/fixed_dict_data_type.cpp
	data_types/fixed_dict_data_type.hpp
	data_types/fixed_dict_meta_data_type.cpp
	data_types/fixed_dict_meta_data_type.hpp
	data_types/float_data_types.cpp
	data_types/float_data_types.hpp
	data_types/integer_data_type.cpp
	data_types/integer_data_type.hpp
	data_types/long_integer_data_type.cpp
	data_types/long_integer_data_type.hpp
	data_types/python_data_type.cpp
	data_types/python_data_type.hpp
	data_types/simple_stream_element.hpp
	data_types/sequence_data_type.cpp
	data_types/sequence_data_type.hpp
	data_types/sequence_meta_data_type.cpp
	data_types/sequence_meta_data_type.hpp
	data_types/string_data_type.cpp
	data_types/string_data_type.hpp
	data_types/tuple_data_type.cpp
	data_types/tuple_data_type.hpp
	data_types/unicode_string_data_type.cpp
	data_types/unicode_string_data_type.hpp
	data_types/user_data_type.cpp
	data_types/user_data_type.hpp
	data_types/user_meta_data_type.cpp
	data_types/user_meta_data_type.hpp
	data_types/vector_data_types.cpp
	data_types/vector_data_types.hpp
)
IF (BW_IS_SERVER)
	LIST( APPEND DATA_TYPES_SRCS
		 data_types/mailbox_data_type.cpp
		 data_types/mailbox_data_type.hpp
	)
ENDIF()
SOURCE_GROUP( "data_types" FILES ${DATA_TYPES_SRCS} )


SET( DATA_INSTANCES_SRCS
	data_instances/array_data_instance.cpp
	data_instances/array_data_instance.hpp
	data_instances/class_data_instance.cpp
	data_instances/class_data_instance.hpp
	data_instances/fixed_dict_data_instance.cpp
	data_instances/fixed_dict_data_instance.hpp
	data_instances/intermediate_property_owner.cpp
	data_instances/intermediate_property_owner.hpp
)
SOURCE_GROUP( "data_instances" FILES ${DATA_INSTANCES_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${ALL_SRCS}
	${DATA_TYPES_SRCS}
	${DATA_INSTANCES_SRCS}
)

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

BW_ADD_LIBRARY( entitydef ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( entitydef INTERFACE
	cstdmf
	network
	pyscript
	resmgr
	script
	)

BW_PRECOMPILED_HEADER( entitydef pch.hpp )
BW_PROJECT_CATEGORY( entitydef "BigWorld Libs" )
