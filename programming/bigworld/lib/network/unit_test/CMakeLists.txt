CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( network_unit_test )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	common_interface.cpp
	common_interface.hpp
	main.cpp
	pch.cpp
	pch.hpp
	test_channel.cpp
	test_channel_interfaces.hpp
	test_elliptic_curve.cpp
	test_interface.cpp
	test_msgtypes.cpp
	test_network_checksum_schemes.cpp
	test_reliable.cpp
	test_tcp_channels.cpp
	test_tcp_channels_interfaces.hpp
	test_websockets.cpp
)

IF (BW_IS_SERVER)
	LIST( APPEND ALL_SRCS
		packet_generator.cpp
		packet_generator.hpp
		test_auto_switch.cpp
		test_baseapp_death.cpp
		test_channel_version.cpp
		test_compresslength.cpp
		test_config.cpp
		test_exposed_message_range.cpp
		test_flood.cpp
		test_flood_interfaces.hpp
		test_fragment.cpp
		test_fragment_interfaces.hpp
		test_mangle.cpp
		test_mangle_interfaces.hpp
		test_netmask.cpp
		test_networkstats.cpp
		test_overflow.cpp
		test_receive_window.cpp
		test_snprintf.cpp
		test_stream.cpp
		test_threadsafety.cpp
	)
ELSE()
	LIST( APPEND ALL_SRCS
		test_piggybacks_longer_than_normal.cpp
	)
ENDIF()

FIND_PACKAGE( BWOpenSSL REQUIRED )
IF( BWOPENSSL_FOUND )
	ADD_DEFINITIONS( -DUSE_OPENSSL )
	INCLUDE_DIRECTORIES( ${BWOPENSSL_INCLUDE_DIRS} )
ENDIF()

BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_EXECUTABLE( network_unit_test ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( network_unit_test
	cstdmf
	network
	unit_test_lib
	)

BW_ADD_TEST( network_unit_test )

BW_PRECOMPILED_HEADER( network_unit_test pch.hpp )
BW_PROJECT_CATEGORY( network_unit_test "Unit Tests" )
