CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( dependency )

ADD_DEFINITIONS( 
	-DBW_ASSET_PIPELINE
)

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	dependency.cpp
	dependency.hpp
	dependency_list.cpp
	dependency_list.hpp
	converter_dependency.hpp
	converter_dependency.cpp
	converter_params_dependency.hpp
	converter_params_dependency.cpp
	directory_dependency.hpp
	directory_dependency.cpp
	intermediate_file_dependency.hpp
	intermediate_file_dependency.cpp
	output_file_dependency.hpp
	output_file_dependency.cpp
	source_file_dependency.hpp
	source_file_dependency.cpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


BW_ADD_LIBRARY( dependency
	${ALL_SRCS}
)

BW_TARGET_LINK_LIBRARIES( dependency INTERFACE
	asset_pipeline
	cstdmf
	resmgr
	)

BW_PROJECT_CATEGORY( dependency "Asset Pipeline" )
