CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( conversion )

ADD_DEFINITIONS( 
	-DBW_ASSET_PIPELINE
)

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	content_addressable_cache.hpp
	content_addressable_cache.cpp
	conversion_task.hpp
	conversion_task_queue.hpp
	converter.hpp
	converter_creator.hpp
	converter_info.hpp
	converter_map.hpp
	task_processor.cpp
	task_processor.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


BW_ADD_LIBRARY( conversion
	${ALL_SRCS}
)

BW_TARGET_LINK_LIBRARIES( conversion INTERFACE
	compiler
	cstdmf
	dependency
	resmgr
	)

BW_PROJECT_CATEGORY( conversion "Asset Pipeline" )