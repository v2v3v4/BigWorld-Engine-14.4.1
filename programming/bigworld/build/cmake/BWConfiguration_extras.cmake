SET( USE_MEMHOOK ON )
ADD_DEFINITIONS( -DUSE_MEMHOOK )

SET( BW_LIBRARY_PROJECTS
	# BigWorld	
	chunk				lib/chunk
	connection			lib/connection
	connection_model	lib/connection_model
	cstdmf				lib/cstdmf
	duplo				lib/duplo
	entitydef			lib/entitydef
	input				lib/input
	math				lib/math
	memhook				lib/memhook	
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
	
	# Third party
	libpng				third_party/png/projects/visualc71
	libpython			third_party/python
	nedalloc			third_party/nedalloc
	re2					third_party/re2
	zip					third_party/zip
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
	simple_python_client		examples/client_integration/python/simple
	process_defs		tools/process_defs
	res_packer		tools/res_packer
	
)
