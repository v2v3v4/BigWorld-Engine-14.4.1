CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( bots )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	add_bots_callable_watcher.cpp
	add_bots_callable_watcher.hpp
	beeline_controller.cpp
	beeline_controller.hpp
	bots_config.cpp
	bots_config.hpp
	bot_entity.cpp
	bot_entity.hpp
	client_app.cpp
	client_app.hpp
	del_bots_callable_watcher.cpp
	del_bots_callable_watcher.hpp
	del_tagged_bots_callable_watcher.cpp
	del_tagged_bots_callable_watcher.hpp
	entity.cpp
	entity.hpp
	entity_type.cpp
	entity_type.hpp
	main.cpp
	main_app.cpp
	main_app.hpp
	movement_controller.cpp
	movement_controller.hpp
	patrol_graph.cpp
	patrol_graph.hpp
	py_bots.cpp
	py_bots.hpp
	py_entities.cpp
	py_entities.hpp
	py_entities.ipp
	py_entity.cpp
	py_entity.hpp
	py_server.cpp
	py_server.hpp
	script_bot_entity.cpp
	script_bot_entity.hpp
	update_movement_callable_watcher.cpp
	update_movement_callable_watcher.hpp
	zigzag_patrol_graph.cpp
	zigzag_patrol_graph.hpp
	../../../common/simple_client_entity.cpp
	../../../common/simple_client_entity.hpp
	../../../common/py_server_lib.cpp
	../../../common/py_timer.cpp
	../../../common/py_timer.hpp
	../../../common/py_terrain.cpp
	../../../common/py_physics2.cpp
	../../../common/py_physics2.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( bots ${ALL_SRCS} )

BW_TARGET_LINK_LIBRARIES( bots
	entitydef
	pyscript
	script
	server
	chunk_loading
	chunk
	terrain
	physics2
	scene
	connection_model
	connection
	moo
	png
)

BW_PROJECT_CATEGORY( bots "Tools" )
