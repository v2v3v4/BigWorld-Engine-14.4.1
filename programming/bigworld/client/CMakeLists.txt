CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( bwclient )

INCLUDE( BWStandardProject )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR} )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/lib/emptyvoip )

# TODO: Remove dependancy on Scaleform headers from bwclient
# Scaleform::Manager exposes Scaleform types to its users,
# but bwclient doesn't actually use them, so we don't version
# this dependancy
IF( BW_SCALEFORM_SUPPORT )
	FIND_PACKAGE( ScaleformSDK )
	IF( SCALEFORMSDK_FOUND )
		INCLUDE_DIRECTORIES( ${SCALEFORMSDK_INCLUDE_DIRS} )
	ENDIF()
ENDIF()

# TODO: Remove dependancy on curl headers from bwclient
# EntityManager holds a URL::Request pointer, which is a very thin
# wrapper around a curl handle.
FIND_PACKAGE( BWcurl REQUIRED )
IF( BWCURL_FOUND )
	ADD_DEFINITIONS( -DCURL_STATICLIB )
	INCLUDE_DIRECTORIES( ${BWCURL_INCLUDE_DIRS} )
ENDIF()

IF( BW_AWESOMIUM_SUPPORT )
	LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/awesomium/build/lib )
ENDIF()

SET( ALL_SRCS
	critical_handler.cpp
	critical_handler.hpp
	message_time_prefix.cpp
	message_time_prefix.hpp
	pch.cpp
	pch.hpp
	resource.h
	time_globals.cpp
	time_globals.hpp
	fly_through_camera.cpp
	fly_through_camera.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( PLAYER_SRCS
	linear_path.cpp
	linear_path.hpp
	motion_constants.cpp
	motion_constants.hpp
	path_seeker.cpp
	path_seeker.hpp
	physics.cpp
	physics.hpp
	pots.cpp
	pots.hpp
	script_player.cpp
	script_player.hpp
)
SOURCE_GROUP( "Player" FILES ${PLAYER_SRCS} )


SET( APP_SRCS
	adaptive_lod_controller.cpp
	adaptive_lod_controller.hpp
	adaptive_lod_controller.ipp
	app.cpp
	app.hpp
	app.ipp
	app_config.cpp
	app_config.hpp
	app_config.ipp
	client_camera.cpp
	client_camera.hpp
	client_camera.ipp
	pathed_filename.cpp
	pathed_filename.hpp
	settings_overrides.hpp
	settings_overrides.cpp
	version_info.cpp
	version_info.hpp
	version_info.ipp
)
SOURCE_GROUP( "App" FILES ${APP_SRCS} )


SET( SCRIPT_SRCS
	../common/py_chunk.cpp
	../common/py_physics2.cpp
	../common/py_physics2.hpp
	../common/py_urlrequest.cpp
	portal_state_changer.cpp
	portal_state_changer.hpp
	py_aoi_entities.cpp
	py_aoi_entities.hpp
	py_entities.cpp
	py_entities.hpp
	py_entities.ipp
	py_server.cpp
	py_server.hpp
	script_bigworld.cpp
	script_bigworld.hpp
)
SOURCE_GROUP( "Script" FILES ${SCRIPT_SRCS} )


SET( SERVER_SRCS
	../common/space_data_types.hpp
	connection_control.cpp
	connection_control.hpp
	python_server.cpp
	python_server.hpp
	server_discovery.cpp
	server_discovery.hpp
	space_data_storage.hpp
	space_data_storage.cpp
)
SOURCE_GROUP( "Server" FILES ${SERVER_SRCS} )


SET( ENTITIES_SRCS
	../common/simple_client_entity.cpp
	../common/simple_client_entity.hpp
	chunk_entity.cpp
	chunk_entity.hpp
	entity.cpp
	entity.hpp
	entity.ipp
	entity_flare_collider.cpp
	entity_flare_collider.hpp
	entity_flare_collider.ipp
	entity_factory.hpp
	entity_factory.cpp
	entity_manager.cpp
	entity_manager.hpp
	entity_udo_factory.cpp
	entity_udo_factory.hpp
	entity_picker.cpp
	entity_picker.hpp
	entity_picker.ipp
	entity_type.cpp
	entity_type.hpp
	py_entity.cpp
	py_entity.hpp
)
SOURCE_GROUP( "Entities" FILES ${ENTITIES_SRCS} )


SET( UI_SRCS
	frame_rate_graph.cpp
	frame_rate_graph.hpp
	latency_gui_component.cpp
	latency_gui_component.hpp
	latency_gui_component.ipp
	minimap.cpp
	minimap.hpp
	minimap.ipp
	py_scene_renderer.cpp
	py_scene_renderer.hpp
)
SOURCE_GROUP( "UI" FILES ${UI_SRCS} )


SET( FILTERS_SRCS
	avatar_drop_filter.cpp
	avatar_drop_filter.hpp
	avatar_filter.cpp
	avatar_filter.hpp
	avatar_filter_callback.cpp
	avatar_filter_callback.hpp
	boids_filter.cpp
	boids_filter.hpp
	bwclient_filter_environment.cpp
	bwclient_filter_environment.hpp
	dumb_filter.cpp
	dumb_filter.hpp
	filter.cpp
	filter.hpp
	player_avatar_filter.cpp
	player_avatar_filter.hpp
	py_avatar_drop_filter.cpp
	py_avatar_drop_filter.hpp
	py_avatar_filter.cpp
	py_avatar_filter.hpp
	py_boids_filter.cpp
	py_boids_filter.hpp
	py_dumb_filter.cpp
	py_dumb_filter.hpp
	py_filter.cpp
	py_filter.hpp
	py_player_avatar_filter.cpp
	py_player_avatar_filter.hpp
)
SOURCE_GROUP( "Filters" FILES ${FILTERS_SRCS} )


SET( MODEL_SRCS
	action_matcher.cpp
	action_matcher.hpp
	matrix_providers.cpp
	matrix_providers.hpp
	py_chunk_model.cpp
	py_chunk_model.hpp
)
SOURCE_GROUP( "Model" FILES ${MODEL_SRCS} )


SET( MAINLOOPTASKS_SRCS
	camera_app.cpp
	camera_app.hpp
	canvas_app.cpp
	canvas_app.hpp
	debug_app.cpp
	debug_app.hpp
	device_app.cpp
	device_app.hpp
	facade_app.cpp
	facade_app.hpp
	finale_app.cpp
	finale_app.hpp
	gui_app.cpp
	gui_app.hpp
	lens_app.cpp
	lens_app.hpp
	profiler_app.cpp
	profiler_app.hpp
	script_app.cpp
	script_app.hpp
	voip_app.cpp
	voip_app.hpp
	web_app.cpp
	web_app.hpp
	world_app.cpp
	world_app.hpp
)
SOURCE_GROUP( "MainLoopTasks" FILES ${MAINLOOPTASKS_SRCS} )


SET( WINMAIN_SRCS
	bw_winmain.cpp
	bw_winmain.hpp
	client.rc
	compile_time.cpp
	winmain.cpp
)
SOURCE_GROUP( "WinMain" FILES ${WINMAIN_SRCS} )


SET( VOIP_SRCS
	bwvoip.cpp
	bwvoip.hpp
	voip.hpp
)
SOURCE_GROUP( "VOIP" FILES ${VOIP_SRCS} )




BW_BLOB_SOURCES( BLOB_SRCS
	${ALL_SRCS}
	${PLAYER_SRCS}
	${APP_SRCS}
	${SCRIPT_SRCS}
	${SERVER_SRCS}
	${ENTITIES_SRCS}
	${UI_SRCS}
	${FILTERS_SRCS}
	${MODEL_SRCS}
	${MAINLOOPTASKS_SRCS}
	${WINMAIN_SRCS}
	${VOIP_SRCS}
	)

BW_ADD_EXECUTABLE( bwclient WIN32
	${BLOB_SRCS}
	)


BW_TARGET_LINK_LIBRARIES( bwclient
	ashes
	camera
	chunk
	chunk_scene_adapter
	compiled_space
	connection
	connection_model
	cstdmf
	duplo
	entitydef
	input
	math
	model
	moo
	network
	open_automate
	particle
	physics2
	post_processing
	pyscript
	resmgr
	romp
	scene
	space
	terrain
	urlrequest
	waypoint
	web_render

	emptyvoip
	# external libraries
	winmm	
	wtsapi32
)

IF( BW_FMOD_SUPPORT )
	BW_TARGET_LINK_LIBRARIES( bwclient
		fmodsound
	)
ENDIF()

IF( BW_SCALEFORM_SUPPORT )
	BW_TARGET_LINK_LIBRARIES( bwclient
		scaleform
	)
ENDIF()

SET_TARGET_PROPERTIES( bwclient PROPERTIES
		DEBUG_OUTPUT_NAME
		"bwclient_d" )

SET_TARGET_PROPERTIES( bwclient PROPERTIES
		HYBRID_OUTPUT_NAME
		"bwclient_h" )

SET_TARGET_PROPERTIES( bwclient PROPERTIES
		CONSUMER_RELEASE_OUTPUT_NAME
		"bwclient" )

BW_SET_BINARY_DIR( bwclient "${BW_GAME_DIR}/client/${BW_PLATFORM}" )

BW_PRECOMPILED_HEADER( bwclient pch.hpp )
BW_PROJECT_CATEGORY( bwclient "Executables" )

# Delay load Awesomium
IF( BW_AWESOMIUM_SUPPORT AND MSVC )
	BW_APPEND_TARGET_PROPERTIES( bwclient LINK_FLAGS "/DELAYLOAD:awesomium.dll" )
	BW_TARGET_LINK_LIBRARIES( bwclient DelayImp )
ENDIF()

