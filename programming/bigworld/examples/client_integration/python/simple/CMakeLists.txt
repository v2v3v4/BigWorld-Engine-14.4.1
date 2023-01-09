CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( simple_python_client )

INCLUDE( BWStandardProject )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR} )


SET( SOURCE_SRCS
	main.cpp
	entity.cpp
	entity.hpp
	entity_type.cpp
	entity_type.hpp
	main.cpp
	pch.cpp
	pch.hpp
	py_entities.cpp
	py_entities.hpp
	py_entities.ipp
	py_entity.cpp
	py_entity.hpp
	py_server.cpp
	py_server.hpp
	../../../../common/simple_client_entity.cpp
	../../../../common/simple_client_entity.hpp
)
SOURCE_GROUP( "Source" FILES ${SOURCE_SRCS} )


BW_ADD_EXECUTABLE( simple_python_client
	${SOURCE_SRCS}
)


BW_TARGET_LINK_LIBRARIES( simple_python_client
	memhook
	chunk
	connection
	connection_model
	cstdmf
	entitydef
	math
	model
	network
	physics2
	pyscript
	resmgr
	script
	
	# Third party
	libpython
	nedalloc
	re2
	zip
	)


SET_TARGET_PROPERTIES( simple_python_client PROPERTIES
		DEBUG_OUTPUT_NAME
		"simple_python_client_d" )

SET_TARGET_PROPERTIES( simple_python_client PROPERTIES
		HYBRID_OUTPUT_NAME
		"simple_python_client" )

BW_SET_BINARY_DIR( simple_python_client "${BW_GAME_DIR}/examples/${BW_PLATFORM}" )

BW_PRECOMPILED_HEADER( simple_python_client pch.hpp )
BW_PROJECT_CATEGORY( simple_python_client "Executables" )
