ADD_DEFINITIONS( -DEDITOR_ENABLED )

SET ( USE_MEMHOOK OFF )
ADD_DEFINITIONS( -DALLOW_STACK_CONTAINER )
INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools )

IF( BW_UNIT_TESTS_ENABLED )
	SET( BW_TOOLS_UNIT_TEST_LIBRARIES
		test_compiler		tools/asset_pipeline/compiler/test_compiler
		test_converter		tools/asset_pipeline/compiler/test_converter
		unit_test_lib		lib/unit_test_lib
		CppUnitLite2		third_party/CppUnitLite2
		)

	SET( BW_TOOLS_UNIT_TEST_BINARIES
		compiler_unit_test			tools/asset_pipeline/compiler/unit_test
		dependency_unit_test		tools/asset_pipeline/dependency/unit_test
		)

	MESSAGE( STATUS "Unit tests enabled for tools." )
	ENABLE_TESTING()
ENDIF()

SET( BW_LIBRARY_PROJECTS
	# BigWorld
	appmgr				lib/appmgr
	guimanager			lib/guimanager
	guitabs				lib/guitabs
	ashes				lib/ashes
    asset_pipeline		lib/asset_pipeline
	compiled_space_writers lib/compiled_space/binary_writers
	controls			lib/controls
	chunk				lib/chunk
	chunk_scene_adapter lib/chunk_scene_adapter
	duplo				lib/duplo
	entitydef			lib/entitydef
	gizmo				lib/gizmo
	input				lib/input
	math				lib/math
	model				lib/model
	moo					lib/moo
	network				lib/network
	particle			lib/particle
	physics2			lib/physics2
	post_processing		lib/post_processing
	pyscript			lib/pyscript
	resmgr				lib/resmgr
	romp				lib/romp
	scene				lib/scene
	script				lib/script
	space				lib/space
	terrain				lib/terrain
	navigation_recast	lib/navigation_recast
	waypoint_generator	lib/waypoint_generator
	ual					lib/ual
	
	# Asset Pipeline
	compiler			tools/asset_pipeline/compiler
	conversion			tools/asset_pipeline/conversion
	dependency			tools/asset_pipeline/dependency
	discovery			tools/asset_pipeline/discovery
	
	# Third party
	nedalloc			third_party/nedalloc
	libpng				third_party/png/projects/visualc71
	nvmeshmender		third_party/nvmeshmender
	recast				third_party/recastnavigation/Recast
	detour				third_party/recastnavigation/Detour
	zip					third_party/zip
	libpython			third_party/python
	libpython_tools		third_party/python_modules/tools
	re2					third_party/re2

	tools_common		tools/common
	editor_shared		tools/editor_shared

	#Program Core
	modeleditor_core	tools/modeleditor_core

	# Unit test libs
	${BW_TOOLS_UNIT_TEST_LIBRARIES}
)

IF( USE_MEMHOOK )
	ADD_DEFINITIONS( -DUSE_MEMHOOK )

	SET( BW_LIBRARY_PROJECTS ${BW_LIBRARY_PROJECTS}
		memhook			lib/memhook
	)
ENDIF()


IF( BW_FMOD_SUPPORT )
	SET( BW_LIBRARY_PROJECTS ${BW_LIBRARY_PROJECTS}
		fmodsound			lib/fmodsound
	)
ENDIF()

IF( BW_SPEEDTREE_SUPPORT )
	SET( BW_LIBRARY_PROJECTS ${BW_LIBRARY_PROJECTS}
		speedtree			lib/speedtree
	)
ENDIF()

SET( BW_BINARY_PROJECTS
	# BigWorld
	cstdmf				lib/cstdmf
	
	# Apps
	worldeditor			tools/worldeditor
	modeleditor			tools/modeleditor
	particle_editor		tools/particle_editor
	offline_processor	tools/offline_processor
	batch_compiler		tools/batch_compiler
	jit_compiler		tools/jit_compiler

	# Asset Converters
	bsp_converter		tools/asset_pipeline/converters/bsp_converter
	effect_converter	tools/asset_pipeline/converters/effect_converter
	texformat_converter	tools/asset_pipeline/converters/texformat_converter
	texture_converter	tools/asset_pipeline/converters/texture_converter
	hierarchical_config_converter	tools/asset_pipeline/converters/hierarchical_config_converter
	space_converter		tools/asset_pipeline/converters/space_converter
	visual_processor		tools/asset_pipeline/converters/visual_processor
	primitive_processor		tools/asset_pipeline/converters/primitive_processor

	# Unit tests
	${BW_TOOLS_UNIT_TEST_BINARIES}
)
