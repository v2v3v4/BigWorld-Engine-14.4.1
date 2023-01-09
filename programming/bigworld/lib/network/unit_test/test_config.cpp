#include "pch.hpp"

#include <stdio.h>
#include "network/network_interface.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This test verifies that the local machine is correctly configured for
 *  running unit tests.
 */
TEST( Config )
{
	Endpoint ep;
	ep.socket( SOCK_DGRAM );

	ASSERT_WITH_MESSAGE( ep.good(), "Couldn't bind endpoint" );

	ASSERT_WITH_MESSAGE( ep.setBufferSize( SO_RCVBUF, 16777216 ),
		"Insufficient recv buffer to run tests.\n"
		"Please consider runnning the following "
		"commands as root:\n"
		"echo 16777216 > /proc/sys/net/core/rmem_max\n"
		"echo 1048576 > /proc/sys/net/core/wmem_max\n"
		"echo 1048576 > /proc/sys/net/core/wmem_default\n"
		"Or to make permanent in /etc/sysctl.conf set:\n"
		"net.core.rmem_max = 16777216\n"
		"net.core.wmem_max = 1048576\n"
		"net.core.wmem_default = 1048576\n"
		);
}

BW_END_NAMESPACE

// test_config.cpp
