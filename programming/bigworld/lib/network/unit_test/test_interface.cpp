#include "pch.hpp"

#include "network/network_interface.hpp"


BW_BEGIN_NAMESPACE

namespace
{

struct Fixture
{
	Fixture() :
		interface_( NULL, Mercury::NETWORK_INTERFACE_INTERNAL )
	{
	}

	~Fixture()
	{
	}

	Mercury::NetworkInterface interface_;
};

TEST_F( Fixture, Nub_testConstruction )
{
	// Test that it has correctly opened a socket.
	CHECK( interface_.socket().fileno() != -1 );
}

};

BW_END_NAMESPACE

// test_interface.cpp
