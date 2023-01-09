
BW_ADD_CONSUMER_RELEASE_CONFIG()

SET_PROPERTY( DIRECTORY APPEND PROPERTY
	COMPILE_DEFINITIONS_CONSUMER_RELEASE
	CONSUMER_CLIENT
	_RELEASE )

SET( USE_MEMHOOK ON )
# To enable Scaleform support, you need the Scaleform GFx SDK, and
# probably have to modify FindScaleformSDK.cmake to locate it.
SET( BW_SCALEFORM_SUPPORT OFF )

ADD_DEFINITIONS( -DBW_CLIENT )
ADD_DEFINITIONS( -DUSE_MEMHOOK )
ADD_DEFINITIONS( -DALLOW_STACK_CONTAINER )

IF( BW_UNIT_TESTS_ENABLED )
	MESSAGE( STATUS "Unit tests enabled for client." )
	ENABLE_TESTING()

	SET( BW_CLIENT_UNIT_TEST_LIBRARIES
		CppUnitLite2		third_party/CppUnitLite2
		unit_test_lib		lib/unit_test_lib
		)

	SET( BW_CLIENT_UNIT_TEST_BINARIES
		compiled_space_unit_test lib/compiled_space/unit_test
		cstdmf_unit_test	lib/cstdmf/unit_test
		duplo_unit_test		lib/duplo/unit_test
		entitydef_unit_test	lib/entitydef/unit_test
		math_unit_test		lib/math/unit_test
		model_unit_test		lib/model/unit_test
		moo_unit_test		lib/moo/unit_test
		network_unit_test	lib/network/unit_test
		particle_unit_test	lib/particle/unit_test
		physics2_unit_test	lib/physics2/unit_test
		pyscript_unit_test	lib/pyscript/unit_test
		resmgr_unit_test	lib/resmgr/unit_test
		scene_unit_test		lib/scene/unit_test
		script_unit_test	lib/script/unit_test
		space_unit_test		lib/space/unit_test
		)
ENDIF()


SET( BW_LIBRARY_PROJECTS
	# BigWorld
	memhook				lib/memhook
	ashes				lib/ashes
    asset_pipeline		lib/asset_pipeline
	camera				lib/camera
	chunk				lib/chunk
	chunk_scene_adapter lib/chunk_scene_adapter
	connection			lib/connection
	connection_model	lib/connection_model
	cstdmf				lib/cstdmf
	duplo				lib/duplo
	entitydef			lib/entitydef
	input				lib/input
	math				lib/math
	model				lib/model
	moo					lib/moo
	network				lib/network
	open_automate		lib/open_automate
	particle			lib/particle
	physics2			lib/physics2
	post_processing		lib/post_processing
	pyscript			lib/pyscript
	resmgr				lib/resmgr
	romp				lib/romp
	scene				lib/scene
	compiled_space		lib/compiled_space
	space				lib/space
	script				lib/script
	terrain				lib/terrain
	urlrequest			lib/urlrequest
	waypoint			lib/waypoint
	web_render			lib/web_render

	# Third party
	libpng				third_party/png/projects/visualc71
	nedalloc			third_party/nedalloc
	zip					third_party/zip
	libpython			third_party/python
	re2					third_party/re2

	# Unit test librarys
	${BW_CLIENT_UNIT_TEST_LIBRARIES}
)

IF( BW_FMOD_SUPPORT )
	SET( BW_LIBRARY_PROJECTS ${BW_LIBRARY_PROJECTS}
		fmodsound			lib/fmodsound
	)
ENDIF()

IF( BW_SCALEFORM_SUPPORT )
	SET( BW_LIBRARY_PROJECTS ${BW_LIBRARY_PROJECTS}
		scaleform			lib/scaleform
	)
ENDIF()

IF( BW_SPEEDTREE_SUPPORT )
	SET( BW_LIBRARY_PROJECTS ${BW_LIBRARY_PROJECTS}
		speedtree			lib/speedtree
	)
ENDIF()

SET( BW_BINARY_PROJECTS
	bwclient			client
	emptyvoip			lib/emptyvoip
	${BW_CLIENT_UNIT_TEST_BINARIES}
)

