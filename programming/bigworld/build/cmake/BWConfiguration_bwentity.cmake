
BW_ADD_CONSUMER_RELEASE_CONFIG()

SET( BW_USE_BWENTITY_DLL ON )

IF( BW_UNIT_TESTS_ENABLED )
	SET( BW_BWENTITY_UNIT_TEST_LIBRARIES
		unit_test_lib		lib/unit_test_lib
		CppUnitLite2		third_party/CppUnitLite2
		)

	SET( BW_BWENTITY_UNIT_TEST_BINARIES
		bwentity_unit_test	lib/bwentity/unit_test
		)

	MESSAGE( STATUS "Unit tests enabled for bwentity." )
	ENABLE_TESTING()
ENDIF()

SET( BW_LIBRARY_PROJECTS
	# BigWorld
	cstdmf				lib/cstdmf
	math				lib/math
	connection			lib/connection
	connection_model	lib/connection_model
	entitydef			lib/entitydef
	network				lib/network
	pyscript			lib/pyscript
	resmgr				lib/resmgr
	script				lib/script
	
	# Third party
	nedalloc			third_party/nedalloc
	zip					third_party/zip
	libpython			third_party/python
	re2					third_party/re2

	# Unit test libs
	${BW_BWENTITY_UNIT_TEST_LIBRARIES}
)

SET( BW_BINARY_PROJECTS
	# BigWorld
	bwentity	lib/bwentity

	# Unit tests
	${BW_BWENTITY_UNIT_TEST_BINARIES}
)

IF (MSVC)
	# Build bwentity.dll with wchar_t typedef'd to unsigned short
	# because it's easier than fixing the only current user
	SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:wchar_t-" )
ENDIF()
