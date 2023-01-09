SET( BW_FMOD_SUPPORT OFF )
SET( BW_SPEEDTREE_SUPPORT OFF )
SET( BW_UMBRA_SUPPORT OFF )

# animationexporter
ADD_DEFINITIONS( -DBW_EXPORTER )

#If this isn't done then the exporters will crash while accessing stl vectors
#in the 3dsMax SDK. Ideally we would need to use the same compiler/stl version
#as that used to compile the SDK originally.
ADD_DEFINITIONS( -D_SECURE_SCL=1 )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools/exporter_common )

SET( EXPORTER_PROJECTS
	animationexporter_max2010		tools/animationexporter/max2010_exporter
	animationexporter_max2011		tools/animationexporter/max2011_exporter
	animationexporter_max2012		tools/animationexporter/max2012_exporter
	mayavisualexporter_maya2010		tools/mayavisualexporter/maya2010_exporter
	mayavisualexporter_maya2011		tools/mayavisualexporter/maya2011_exporter
	mayavisualexporter_maya2012		tools/mayavisualexporter/maya2012_exporter
	mayavisualexporter_maya2013		tools/mayavisualexporter/maya2013_exporter
	visualexporter_max2010			tools/visualexporter/max2010_exporter
	visualexporter_max2011			tools/visualexporter/max2011_exporter
	visualexporter_max2012			tools/visualexporter/max2012_exporter
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
	scene				lib/scene
	script				lib/script
	space				lib/space
	terrain				lib/terrain

	#third party
	libpng				third_party/png/projects/visualc71
	libpython			third_party/python
	nvmeshmender		third_party/nvmeshmender
	re2					third_party/re2
	zip					third_party/zip
	)

IF( ${BW_PLATFORM} STREQUAL "win64" )
	LIST( APPEND EXPORTER_PROJECTS
		mayavisualexporter_maya2014		tools/mayavisualexporter/maya2014_exporter
	)
ENDIF()

IF( ${BW_PLATFORM} STREQUAL "win32" )
	LIST( APPEND EXPORTER_PROJECTS
		animationexporter_max2009		tools/animationexporter/max2009_exporter
		mayavisualexporter_maya2009		tools/mayavisualexporter/maya2009_exporter
		visualexporter_max2009			tools/visualexporter/max2009_exporter
	)
ENDIF()

IF ( NOT ${BW_COMPILER_TOKEN} STREQUAL "vc9" )
	LIST( APPEND EXPORTER_PROJECTS
		animationexporter_max2013		tools/animationexporter/max2013_exporter
		visualexporter_max2013			tools/visualexporter/max2013_exporter
	)
ENDIF()

SET( BW_LIBRARY_PROJECTS
	${EXPORTER_PROJECTS}
)

