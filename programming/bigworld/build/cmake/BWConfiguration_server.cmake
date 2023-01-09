ADD_DEFINITIONS( -DMF_SERVER )

# Add commands for start, stop, status
BW_TARGET_LINUX_COMMAND( start_server 
	"python ${BW_LINUX_DIR}/../tools/server/control_cluster.py start ${BW_LINUX_HOST}" )
BW_TARGET_LINUX_COMMAND( stop_server 
	"python ${BW_LINUX_DIR}/../tools/server/control_cluster.py stop" )
BW_TARGET_LINUX_COMMAND( display_server 
	"python ${BW_LINUX_DIR}/../tools/server/control_cluster.py display" )

IF( BW_UNIT_TESTS_ENABLED )
	MESSAGE( STATUS "Unit tests enabled for server." )
	ENABLE_TESTING()
	
	SET( BW_SERVER_UNIT_TEST_LIBRARIES
		CppUnitLite2		third_party/CppUnitLite2
		unit_test_lib		lib/unit_test_lib
		)

	SET( BW_SERVER_UNIT_TEST_BINARIES
		cstdmf_unit_test		lib/cstdmf/unit_test
		db_unit_test			lib/db/unit_test
		entitydef_unit_test		lib/entitydef/unit_test
		math_unit_test			lib/math/unit_test
		network_unit_test		lib/network/unit_test
		physics2_unit_test		lib/physics2/unit_test
		pyscript_unit_test		lib/pyscript/unit_test
		resmgr_unit_test		lib/resmgr/unit_test
		scene_unit_test			lib/scene/unit_test
		server_unit_test		lib/server/unit_test
		script_unit_test		lib/script/unit_test
		terrain_unit_test		lib/terrain/unit_test
		unit_test_lib_unit_test	lib/unit_test_lib/unit_test
		baseapp_unit_test		server/baseapp/unit_test
		baseappmgr_unit_test	server/baseappmgr/unit_test
		cellapp_unit_test		server/cellapp/unit_test
		cellappmgr_unit_test	server/cellappmgr/unit_test
		dbapp_unit_test			server/dbapp/unit_test
		dbappmgr_unit_test		server/dbappmgr/unit_test
		loginapp_unit_test		server/loginapp/unit_test
		reviver_unit_test		server/reviver/unit_test
		)
ENDIF()

SET( BW_LIBRARY_PROJECTS
	# BigWorld Standard libraries
	network 			lib/network
	resmgr 				lib/resmgr
	math 				lib/math
	cstdmf				lib/cstdmf
	connection_model	lib/connection_model
	memhook				lib/memhook
	build				lib/build

	# All servers
	server				lib/server

	# DBMgr
	db					lib/db
	db_storage			lib/db_storage

	# DBMgr MySQL support
	bwengine_mysql		server/dbapp_extensions/bwengine_mysql
	db_storage_mysql	lib/db_storage_mysql

	# DBMgr XML database support
	bwengine_xml		server/dbapp_extensions/bwengine_xml
	db_storage_xml		lib/db_storage_xml

	# BaseApp
	sqlite				lib/sqlite

	# BaseApp + DBMgr + LoginApp
	connection			lib/connection

	# CellApp 
	waypoint			lib/waypoint
	terrain				lib/terrain
	physics2			lib/physics2
	moo					lib/moo
	chunk_loading		lib/chunk_loading
	
	# BaseApp + CellApp
	delegate_interface	lib/delegate_interface
	entitydef_script	lib/entitydef_script
	scene				lib/scene

	# BaseApp + CellApp + DBMgr
	entitydef			lib/entitydef
	pyscript			lib/pyscript
	chunk				lib/chunk
	urlrequest			lib/urlrequest
	script				lib/script

	# Third party
	libpython			third_party/python
	zip 				third_party/zip
	libpng				third_party/png/projects/visualc71
	
	# Unit test librarys
	${BW_SERVER_UNIT_TEST_LIBRARIES}
)

IF( BW_SPEEDTREE_SUPPORT )
	SET( BW_LIBRARY_PROJECTS ${BW_LIBRARY_PROJECTS}
		speedtree			lib/speedtree
	)
ENDIF()

SET( BW_BINARY_PROJECTS
	baseapp				server/baseapp
	baseappmgr			server/baseappmgr
	cellapp				server/cellapp
	cellappmgr			server/cellappmgr
	dbapp				server/dbapp
	dbappmgr			server/dbappmgr
	loginapp			server/loginapp
	reviver				server/reviver
	
	# Tools
	bwmachined			server/tools/bwmachined
	bots				server/tools/bots
	clear_auto_load		server/tools/clear_auto_load
	consolidate_dbs		server/tools/consolidate_dbs
	message_logger		server/tools/message_logger
	_bw_profile			server/tools/bw_profile
	snapshot_helper		server/tools/snapshot_helper
	sync_db				server/tools/sync_db
	transfer_db			server/tools/transfer_db
	
	cellapp_extension	examples/cellapp_extension
	
	${BW_SERVER_UNIT_TEST_BINARIES}
)

SET( BW_PCH_SUPPORT OFF )
SET( BW_FMOD_SUPPORT OFF )
SET( BW_SCALEFORM_SUPPORT OFF )
SET( BW_UMBRA_SUPPORT OFF )
