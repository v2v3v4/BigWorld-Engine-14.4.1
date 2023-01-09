SET( BW_PYTHON_AS_SOURCE OFF )

ADD_DEFINITIONS( -DBWCLIENT_AS_PYTHON_MODULE=1 )
ADD_DEFINITIONS( -DPYMODULE )

SET( BW_LIBRARY_PROJECTS
	# BigWorld
	ashes				lib/ashes
	chunk				lib/chunk
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
	script				lib/script
	scene				lib/scene
	space				lib/space
	terrain				lib/terrain

	# Third party
	libpng				third_party/png/projects/visualc71
	nedalloc			third_party/nedalloc
	re2				third_party/re2
	zip				third_party/zip
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
	assetprocessor		tools/assetprocessor
)
