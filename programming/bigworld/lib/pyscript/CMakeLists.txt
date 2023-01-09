CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( pyscript )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	pch.cpp
	pch.hpp
	personality.cpp
	personality.hpp
	pickler.cpp
	pickler.hpp
	py_to_stl.cpp
	py_to_stl.hpp
	py_to_stl.ipp
	pyobject_base.hpp
	pyobject_plus.cpp
	pyobject_plus.hpp
	pyscript_lib.hpp
	py_factory_method_link.cpp
	py_factory_method_link.hpp
	py_import_paths.cpp
	py_import_paths.hpp
	py_logging.cpp
	python_input_substituter.hpp
	python_input_substituter.cpp
	pywatcher.cpp
	pywatcher.hpp
	py_debug_message_file_logger.cpp
	py_debug_message_file_logger.hpp
	script.cpp
	script.hpp
	script.ipp
	script_math.cpp
	script_math.hpp
	stl_to_py.cpp
	stl_to_py.hpp
	stl_to_py.ipp
)
IF( NOT BW_IS_SERVER )
	LIST( APPEND ALL_SRCS
		automation.cpp
		automation.hpp
		py_callback.cpp
		py_callback.hpp
	)
ELSE()
	LIST( APPEND ALL_SRCS
		py_traceback.cpp
		py_traceback.hpp
		script_events.cpp
		script_events.hpp
	)
ENDIF()
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( RESMGR_SRCS
	py_data_section.cpp
	py_data_section.hpp
	py_data_section.ipp
	res_mgr_script.cpp
	res_mgr_script.hpp
	res_mgr_script.ipp
	resource_table.cpp
	resource_table.hpp
)
SOURCE_GROUP( "ResMgr" FILES ${RESMGR_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${ALL_SRCS}
	${RESMGR_SRCS}
)

IF ( BW_USE_BWENTITY_IN_DLL )
	REMOVE_DEFINITIONS( 
		-DBWENTITY_DLL_IMPORT
		-DCSTDMF_IMPORT
	)

	ADD_DEFINITIONS( 
		-DBWENTITY_DLL_EXPORT
		-DCSTDMF_EXPORT
	)
ENDIF()

BW_ADD_LIBRARY( pyscript ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( pyscript INTERFACE
	cstdmf
	math
	script
	)

IF(BW_PYTHON_AS_SOURCE)
	BW_TARGET_LINK_LIBRARIES( pyscript INTERFACE libpython )
ENDIF()

BW_PRECOMPILED_HEADER( pyscript pch.hpp )
BW_PROJECT_CATEGORY( pyscript "BigWorld Libs" )
