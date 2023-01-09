ADD_DEFINITIONS( -D_NAVGEN )

SET( BW_LIBRARY_PROJECTS
	# BigWorld
	ashes				lib/ashes
	asset_pipeline		lib/asset_pipeline
	chunk				lib/chunk
	chunk_scene_adapter lib/chunk_scene_adapter
	cstdmf				lib/cstdmf
	duplo				lib/duplo
	entitydef			lib/entitydef
	input				lib/input
	math				lib/math
	model				lib/model
	moo					lib/moo
	network				lib/network
	particle			lib/particle
	physics2			lib/physics2
	pyscript			lib/pyscript
	resmgr				lib/resmgr
	romp				lib/romp
	scene				lib/scene
	script				lib/script
	space				lib/space
	terrain				lib/terrain
	waypoint_generator	lib/waypoint_generator
	
	# Third party
	libpng				third_party/png/projects/visualc71
	nedalloc			third_party/nedalloc
	zip					third_party/zip
	libpython			third_party/python
	re2					third_party/re2
)

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
	navgen				tools/navgen
)
