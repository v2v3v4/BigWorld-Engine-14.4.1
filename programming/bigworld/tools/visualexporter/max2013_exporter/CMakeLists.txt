CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( visualexporter_max2013 )

SET( BW_NO_UNICODE OFF )
ADD_DEFINITIONS( -D_CRT_NON_CONFORMING_SWPRINTFS )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/3dsmax/2013/samples/modifiers/morpher/include )
IF( ${BW_PLATFORM} STREQUAL "win32" )
	LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/3dsmax/2013/samples/modifiers/morpher/lib/x32 )
ELSEIF( ${BW_PLATFORM} STREQUAL "win64" )
	LINK_DIRECTORIES( ${BW_SOURCE_DIR}/third_party/3dsmax/2013/samples/modifiers/morpher/lib/x64 )
ENDIF()

INCLUDE( ../VisualExporter.cmake )
BW_VISUAL_EXPORTER_PROJECT( visualexporter_max2013 2013 )

TARGET_LINK_LIBRARIES( visualexporter_max2013
	morpher
)

