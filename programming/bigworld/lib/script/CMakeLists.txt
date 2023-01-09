CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( script )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	first_include.hpp
	init_time_job.cpp
	init_time_job.hpp
	pch.cpp
	pch.hpp
	pickler.hpp
	py_script_object.cpp
	py_script_object.hpp
	py_script_object.ipp
	py_script_args.ipp
	py_script_module.ipp
	py_script_dict.ipp
	py_script_sequence.ipp
	py_script_tuple.ipp
	py_script_list.ipp
	py_script_output_writer.cpp
	py_script_output_writer.hpp
	script_lib.hpp
	script_object.hpp
	script_output_hook.cpp
	script_output_hook.hpp
	script_output_writer.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )

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

BW_ADD_LIBRARY( script
	${BLOB_SRCS}
)

BW_TARGET_LINK_LIBRARIES( script INTERFACE pyscript )

IF(BW_PYTHON_AS_SOURCE)
	BW_TARGET_LINK_LIBRARIES( script INTERFACE libpython )
ENDIF()

BW_PRECOMPILED_HEADER( script pch.hpp )
BW_PROJECT_CATEGORY( script "BigWorld Libs" )
