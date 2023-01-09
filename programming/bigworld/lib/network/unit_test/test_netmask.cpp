#include "pch.hpp"

#include <stdio.h>
#include "network/basictypes.hpp"
#include "network/netmask.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This test verifies that the local machine is correctly configured for
 *  running unit tests.
 */
TEST( NetMask )
{
	NetMask netMask1;
	NetMask netMask2;

	netMask1.parse( "192.168.1.102/24" );
	netMask2.parse( "192.168.1.0/20" );

	Mercury::Address goodAddr;
	Mercury::Address badAddr;

	bool isOkay = watcherStringToValue( "192.168.1.102:42392", goodAddr ) &&
		watcherStringToValue( "192.168.2.102:42392", badAddr );

	ASSERT_WITH_MESSAGE( isOkay, "Failed to parse addresses" );
	ASSERT_WITH_MESSAGE( netMask1.containsAddress( goodAddr.ip ), "Good didn't match" );
	ASSERT_WITH_MESSAGE( !netMask1.containsAddress( badAddr.ip ), "Bad did match" );

	ASSERT_WITH_MESSAGE( netMask2.containsAddress( goodAddr.ip ), "Good didn't match" );
	ASSERT_WITH_MESSAGE( !netMask2.containsAddress( badAddr.ip ), "Bad did match" );
}

BW_END_NAMESPACE

// test_netmask.cpp
