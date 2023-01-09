#include "pch.hpp"

#include <stdio.h>
#include "network/basictypes.hpp"
#include "network/network_stats.hpp"
#include "network/endpoint.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This test verifies NetworkStats::getQueueSizes works correctly.
 */
TEST( NetworkStats )
{
	Endpoint server;
	Endpoint client;

	server.socket( SOCK_DGRAM );
	client.socket( SOCK_DGRAM );

	char failMessage[128];

	int ret = server.bind( INADDR_ANY, 0 );
	bw_snprintf( failMessage, sizeof( failMessage ),
		"Failed to bind any address: %s",
		strerror( errno ) );
	ASSERT_WITH_MESSAGE( !ret, failMessage );

	u_int16_t srvPort = 0;
	u_int32_t srvAddr = 0;
	server.getlocaladdress( &srvPort, &srvAddr );

	int tx = 0;
	int rx = 0;
	NetworkStats::getQueueSizes( server, tx, rx );
	bw_snprintf( failMessage, sizeof( failMessage ),
		"Get non-zero tx(%d)/rx(%d) queue size from beginning.", tx, rx );
	ASSERT_WITH_MESSAGE( rx == 0, failMessage );

	char buf[768] = {' '};
	ret = client.sendto( (void*)buf, 32, srvPort, srvAddr );
	bw_snprintf( failMessage, sizeof( failMessage ),
		"Failed to send message: %s",
		strerror( errno ) );
	ASSERT_WITH_MESSAGE( ret > 0, failMessage );
	// Sleep 400ms to wait the update.
	usleep( 400000 );

	// As it is not easy to get tx accumulated, we only check rx here.
	NetworkStats::getQueueSizes( server, tx, rx );
	bw_snprintf( failMessage, sizeof( failMessage ),
		"Failed to get the tx(%d)/rx(%d) queue size.", tx, rx );
	ASSERT_WITH_MESSAGE( rx > 0, failMessage );


	int tx2 = 0;
	int rx2 = 0;
	ret = client.sendto( (void*)buf, sizeof( buf ), srvPort, srvAddr );
	bw_snprintf( failMessage, sizeof( failMessage ),
		"Failed to send message: %s",
		strerror( errno ) );
	ASSERT_WITH_MESSAGE( ret > 0, failMessage );
	// Sleep 400ms to wait the update.
	usleep( 400000 );

	NetworkStats::getQueueSizes( server, tx2, rx2 );
	bw_snprintf( failMessage, sizeof( failMessage ),
		"Get the incorrect tx(%d)/rx(%d) queue size.", tx2, rx2 );
	ASSERT_WITH_MESSAGE( rx2 > rx, failMessage );
}

BW_END_NAMESPACE

// test_networkstats.cpp
