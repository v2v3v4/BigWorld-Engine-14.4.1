CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( cstdmf )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( DEBUG_SRCS
	callstack.cpp
	callstack.hpp
	config.hpp
	critical_error_handler.cpp
	critical_error_handler.hpp
	debug.cpp
	debug.hpp
	debug_filter.cpp
	debug_filter.hpp
	debug_filter.ipp
	debug_message_callbacks.hpp
	debug_message_file_logger.cpp
	debug_message_file_logger.hpp
	debug_message_priority.hpp
	debug_message_priority.ipp
	debug_message_source.cpp
	debug_message_source.hpp
	dogwatch.cpp
	dogwatch.hpp
	dogwatch.ipp
	dprintf.cpp
	dprintf.hpp
	guard.hpp
	log_meta_data.cpp
	log_meta_data.hpp
	log_msg.cpp
	log_msg.hpp
	polymorphic_watcher.cpp
	polymorphic_watcher.hpp
	profile.cpp
	profile.hpp
	profiler.cpp
	profiler.hpp
	profiler.ipp
	slot_tracker.cpp
	slot_tracker.hpp
	stack_tracker.cpp
	stack_tracker.hpp
	stack_tracker.ipp
	task_watcher.cpp
	task_watcher.hpp
	timestamp.cpp
	timestamp.hpp
	watcher.cpp
	watcher.hpp
)
IF( BW_PLATFORM_WINDOWS )
	LIST( APPEND DEBUG_SRCS
		callstack_windows.cpp
		debug_exception_filter.cpp
		debug_exception_filter.hpp
		external_profiler.cpp
		external_profiler.hpp
	)
ENDIF()

IF( BW_PLATFORM_LINUX )
	LIST( APPEND DEBUG_SRCS
		callstack_linux.cpp
	)
ENDIF()
SOURCE_GROUP( "Debug" FILES ${DEBUG_SRCS} )


SET( ALL_SRCS
	bw_namespace.hpp
	cstdmf_lib.hpp
	cstdmf_windows.hpp
	cpuinfo.cpp
	cpuinfo.hpp
	cstdmf_dll.hpp
	pch.cpp
	pch.hpp
	string_builder.cpp
	string_builder.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( MEMORY_SRCS
	allocator.cpp
	allocator.hpp
	ansi_allocator.cpp
	ansi_allocator.hpp
	bw_safe_allocatable.hpp
	bw_safe_allocatable.cpp
	protected_allocator.cpp
	protected_allocator.hpp
	bw_memory.cpp
	bw_memory.hpp
	grow_only_pod_allocator.cpp
	grow_only_pod_allocator.hpp
	fixed_sized_allocator.cpp
	fixed_sized_allocator.hpp
	memory_debug.cpp
	memory_debug.hpp
	stl_fixed_sized_allocator.hpp
)
IF( NOT BW_IS_SERVER )
	LIST( APPEND MEMORY_SRCS
		memory_load.cpp
		memory_load.hpp
		ned_allocator.cpp
		ned_allocator.hpp
	
	)
ENDIF()
SOURCE_GROUP( "Memory" FILES ${MEMORY_SRCS} )


SET( UTILITY_SRCS
	background_file_writer.cpp
	background_file_writer.hpp
	base64.cpp
	base64.h
	bgtask_manager.cpp
	bgtask_manager.hpp
	binary_stream.cpp
	binary_stream.hpp
	binary_stream.ipp
	binaryfile.cpp
	binaryfile.hpp
	bit_reader.cpp
	bit_reader.hpp
	bit_writer.cpp
	bit_writer.hpp
	bw_functor.hpp
	bw_hash.cpp
	bw_hash.hpp
	bw_list.hpp
	bw_map.hpp
	bw_noncopyable.hpp
	bw_platform_info.hpp
	bw_platform_info.cpp
	bw_set.hpp
	bw_string.hpp
	bw_string_ref.cpp
	bw_string_ref.hpp
	bw_unordered_map.hpp
	bw_unordered_set.hpp
	bw_util.cpp
	bw_util.hpp
	bw_vector.hpp
	bw_deque.hpp
	bwrandom.cpp
	bwrandom.hpp
	bwversion.cpp
	bwversion.hpp
	checksum_stream.cpp
	checksum_stream.hpp
	circular_queue.hpp
	circular_queue.ipp
	concurrency.cpp
	concurrency.hpp
	cstdmf_init.cpp
	cstdmf_init.hpp
	dependency_graph.hpp
	event.hpp
	fini_job.cpp
	fini_job.hpp
	fourcc.hpp
	ftp.hpp
	init_singleton.hpp
	intrusive_object.hpp
	locale.cpp
	locale.hpp
	lookup_table.hpp
	md5.cpp
	md5.hpp
	memory_stream.cpp
	memory_stream.hpp
	memory_stream.ipp
	named_object.hpp
	progress.hpp
	safe_fifo.hpp
	shared_ptr.hpp
	singleton.hpp
	slow_task.hpp
	smartpointer.hpp
	static_array.hpp
	stdmf.hpp
	string_utils.cpp
	string_utils.hpp
	stringmap.hpp
	time_queue.cpp
	time_queue.hpp
	time_queue.ipp
	timer_handler.hpp
	unique_id.cpp
	unique_id.hpp
	vectornodest.hpp
	watcher_path_request.cpp
	watcher_path_request.hpp
)
IF( NOT BW_IS_SERVER )
	LIST( APPEND UTILITY_SRCS
		bw_hash64.cpp
		bw_hash64.hpp
		command_line.cpp
		command_line.hpp
		critical_message_box.cpp
		critical_message_box.hpp
		date_time_utils.cpp
		date_time_utils.hpp
		main_loop_task.cpp
		main_loop_task.hpp
		restart.cpp
		restart.hpp
		shared_string.hpp
		shared_string.cpp
		singleton_manager.hpp
		singleton_manager.cpp
	)
ELSEIF( BW_IS_SERVER )
	LIST( APPEND UTILITY_SRCS
		arg_parser.cpp
		arg_parser.hpp
		blob_or_null.cpp
		blob_or_null.hpp
	)
ENDIF()

IF( BW_PLATFORM_LINUX )
	LIST( APPEND UTILITY_SRCS
		bw_util_linux.cpp
	)
ELSEIF( BW_PLATFORM_WINDOWS )
	LIST( APPEND UTILITY_SRCS
		bw_util_windows.cpp
		launch_windows_process.hpp
		launch_windows_process.cpp
		message_box.cpp
		message_box.hpp
		resource_counters.cpp
		resource_counters.hpp
	)
ENDIF()
SOURCE_GROUP( "Utility" FILES ${UTILITY_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${DEBUG_SRCS}
	${ALL_SRCS}
	${MEMORY_SRCS}
	${UTILITY_SRCS}
	${ASM_SRCS}
)

IF ( BW_USE_CSTDMF_IN_DLL )
	REMOVE_DEFINITIONS( 
		-DCSTDMF_IMPORT
	)

	ADD_DEFINITIONS( 
		-DCSTDMF_EXPORT
	)
ENDIF()

IF ( BW_USE_CSTDMF_DLL )
	BW_ADD_LIBRARY( cstdmf SHARED ${BLOB_SRCS} )

	IF( USE_MEMHOOK )
		BW_TARGET_LINK_LIBRARIES( cstdmf PRIVATE memhook )
	ENDIF()

	IF( NOT BW_IS_SERVER )
		BW_TARGET_LINK_LIBRARIES( cstdmf PRIVATE nedalloc )
	ENDIF()

	SET_TARGET_PROPERTIES( cstdmf PROPERTIES
		DEBUG_OUTPUT_NAME
		"cstdmf_d" )

	SET_TARGET_PROPERTIES( cstdmf PROPERTIES
		HYBRID_OUTPUT_NAME
		"cstdmf" )
	BW_SET_BINARY_DIR( cstdmf "${BW_GAME_DIR}/dll/${BW_PLATFORM}" )
ELSE()
	BW_ADD_LIBRARY( cstdmf ${BLOB_SRCS} )

	IF( USE_MEMHOOK )
		BW_TARGET_LINK_LIBRARIES( cstdmf INTERFACE memhook )
	ENDIF()

	# TODO: Create a USE_NEDALLOC variable instead of tying to config type
	IF( NOT BW_IS_SERVER AND NOT BW_IS_EXPORTERS )
		BW_TARGET_LINK_LIBRARIES( cstdmf INTERFACE nedalloc )
	ENDIF()

ENDIF()

IF( BW_PLATFORM_WINDOWS )

	FIND_PATH( GPA_INC_DIR ittnotify.h
		$ENV{GPA_INCLUDE_DIR}
		DOC "The directory where the Intel GPA SDK is installed." )

	IF( BW_ARCH_32 )
		SET( GPA_LIB_DIR ${GPA_INC_DIR}/../lib32)
	ELSEIF( BW_ARCH_64 )
		SET( GPA_LIB_DIR ${GPA_INC_DIR}/../lib64)
	ENDIF()
	FIND_LIBRARY( GPA_LIB libittnotify.lib "${GPA_LIB_DIR}" )

	# Test for successful find of lib files
	IF( GPA_INC_DIR AND GPA_LIB )

		MESSAGE( STATUS "-- Found GPA SDK. Include directory is: ${GPA_INC_DIR}" )
		MESSAGE( STATUS "-- Found GPA SDK. Library directory is: ${GPA_LIB}" )

		BW_TARGET_LINK_LIBRARIES( cstdmf INTERFACE "${GPA_LIB}" )
		INCLUDE_DIRECTORIES( ${GPA_INC_DIR} )

		ADD_DEFINITIONS(-DEXTERNAL_PROFILER_ITT_LIB="${GPA_LIB}")
		ADD_DEFINITIONS(-DEXTERNAL_PROFILER_AVAILABLE_ITT)
	ENDIF()

	FIND_PATH( NVTOOLS_INC_DIR nvToolsExt.h
		$ENV{NVTOOLSEXT_PATH}include
		DOC "The directory where the nVidia tool SDK is installed." )
	
	SET( NVTOOLS_INC_DIR ${NVTOOLS_INC_DIR} )
	SET( NVTOOLS_LIB_DIR ${NVTOOLS_INC_DIR}/../lib )

	IF( BW_ARCH_32 )
		SET( NVTOOLS_LIB_NAME nvToolsExt32_1.lib )
		SET( NVTOOLS_LIB_DIR ${NVTOOLS_LIB_DIR}/Win32)
	ELSEIF( BW_ARCH_64 )
		SET( NVTOOLS_LIB_NAME nvToolsExt64_1.lib )
		SET( NVTOOLS_LIB_DIR ${NVTOOLS_LIB_DIR}/x64)
	ENDIF()
	FIND_LIBRARY( NVTOOLS_LIB ${NVTOOLS_LIB_NAME} "${NVTOOLS_LIB_DIR}" )

	# Test for successful finding of lib files
	IF( NVTOOLS_INC_DIR AND NVTOOLS_LIB )

		MESSAGE( STATUS "-- Found Nvidia tools SDK. Include directory is: ${NVTOOLS_INC_DIR}" )
		MESSAGE( STATUS "-- Found Nvidia tools SDK. Library directory is: ${NVTOOLS_LIB}" )

		BW_TARGET_LINK_LIBRARIES( cstdmf INTERFACE "${NVTOOLS_LIB}" )
		INCLUDE_DIRECTORIES( ${NVTOOLS_INC_DIR} )

		ADD_DEFINITIONS(-DEXTERNAL_PROFILER_NVT_LIB="${NVTOOLS_LIB}")
		ADD_DEFINITIONS(-DEXTERNAL_PROFILER_AVAILABLE_NVT)
	ENDIF()
ELSEIF( BW_PLATFORM_LINUX )
	BW_TARGET_LINK_LIBRARIES( cstdmf INTERFACE bfd )
ENDIF()

BW_PRECOMPILED_HEADER( cstdmf pch.hpp )
BW_PROJECT_CATEGORY( cstdmf "BigWorld Libs" )
