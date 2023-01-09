CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( bwmachined )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	main.cpp
	bwmachined.cpp
	bwmachined.hpp
	bwmachined.ipp
	cluster.cpp
	cluster.hpp
	common_machine_guard.hpp
	incoming_packet.cpp
	incoming_packet.hpp
	linux_machine_guard.cpp
	linux_machine_guard.hpp
	listeners.cpp
	listeners.hpp
	message_with_destination.hpp
	process_binary_version.hpp
	server_platform.cpp
	server_platform.hpp
	usermap.cpp
	usermap.hpp
)

IF( BW_PLATFORM_LINUX )
	LIST( APPEND ALL_SRCS
		server_platform_linux.cpp
		server_platform_linux.hpp
	)
ENDIF()

SOURCE_GROUP( "" FILES ${ALL_SRCS} )

BW_ADD_EXECUTABLE( bwmachined ${ALL_SRCS} )

BW_TARGET_LINK_LIBRARIES( bwmachined
	server 
	network 
	cstdmf
)

BW_PROJECT_CATEGORY( bwmachined "Tools" )
