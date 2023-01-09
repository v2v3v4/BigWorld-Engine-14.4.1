CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( scene_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	pch.hpp
	pch.cpp
	main.cpp
	
	test_scene.cpp
	test_scenetypesystem.cpp
	test_scene_object_flags.cpp
	test_sceneview.hpp
	test_sceneview.cpp
	test_staticsceneprovider.hpp
	test_staticsceneprovider.cpp
	test_drawoperation.hpp
	test_drawoperation.cpp
	test_polymesh.hpp
	test_polymesh.cpp
)

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_EXECUTABLE( scene_unit_test ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( scene_unit_test
	cstdmf
	physics2
	resmgr
	scene
	unit_test_lib
)

BW_ADD_TEST( scene_unit_test )

BW_PRECOMPILED_HEADER( scene_unit_test pch.hpp )
BW_PROJECT_CATEGORY( scene_unit_test "Unit Tests" )

