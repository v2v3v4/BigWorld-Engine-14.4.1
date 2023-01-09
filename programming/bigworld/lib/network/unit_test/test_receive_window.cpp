#include "pch.hpp"

#include "common_interface.hpp"
#include "packet_generator.hpp"

#include "cstdmf/debug.hpp"

#include "network/basictypes.hpp"
#include "network/bundle.hpp"
#include "network/channel.hpp"
#include "network/endpoint.hpp"
#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"
#include "network/packet.hpp"
#include "network/packet_filter.hpp"
#include "network/packet_receiver.hpp"

#include <memory>


BW_BEGIN_NAMESPACE

namespace Util
{

const uint32 MAGIC = 0xbeefcafe;

uint32 getLocalInterface()
{
	Endpoint endpoint;
	endpoint.socket( SOCK_DGRAM );
	BW::map< uint32, BW::string > interfaces;
	endpoint.getInterfaces( interfaces );
	BW::map< uint32, BW::string>::iterator iInterface = interfaces.begin();
	while (iInterface != interfaces.end())
	{
		if (iInterface->first != 0)
		{
			return iInterface->first;
		}
		++iInterface;
	}
	return 0;
}


} // namespace Util

class TestReceiveWindowHandler : public CommonHandler
{
public:
	TestReceiveWindowHandler() : 
			inOrder_( true ),
			nextExpected_( 0 )
	{}

	bool inOrder() const 
		{ return inOrder_; }

	uint numReceived() const
		{ return nextExpected_; }

protected:
	virtual void on_msg1( const Mercury::Address & srcAddr,
			const CommonInterface::msg1Args & args )
	{
		if (inOrder_ && args.data != Util::MAGIC && args.seq != nextExpected_)
		{
			inOrder_ = false;
		}

		++nextExpected_;
	}

private:
	bool inOrder_;
	uint nextExpected_;
};


class TestReceiveWindowFixture
{
public:
	TestReceiveWindowFixture();
	~TestReceiveWindowFixture();

protected:
	Mercury::NetworkInterface networkInterface_;
	TestReceiveWindowHandler handler_;
};


TestReceiveWindowFixture::TestReceiveWindowFixture():
		networkInterface_( NULL, Mercury::NETWORK_INTERFACE_INTERNAL ),
		handler_()
{
	CommonInterface::registerWithInterface( networkInterface_ );
	networkInterface_.pExtensionData( &handler_ );
}


TestReceiveWindowFixture::~TestReceiveWindowFixture()
{
}


/**
 *	This test case represents sending reliable fragmented bundle packets to an
 *	already established channel and dropping the first packet of the fragmented
 *	bundle, then pushing the channel's buffered receives array to double
 *	itself. This tests a bug where the reallocation and copy of the buffered
 *	receives resulted in a error in the order of the packets in the newly
 *	allocated buffered receives.
 */
TEST_F( TestReceiveWindowFixture, ReceiveWindow_test )
{
	Mercury::PacketReceiver packetReceiver( networkInterface_.socket(), 
		networkInterface_ );

	PacketGenerator generator;

	CommonInterface::msg1Args args;	
	args.data = Util::MAGIC;

	// This endpoint is used as the address from where the faked packets are
	// apparently from. We need this to be separate from the
	// TestReceiveWindowFixture's packet generator endpoint so that the ACKs
	// sent from the network interface are sent instead to the fake endpoint.
	Endpoint endpoint;
	endpoint.socket( SOCK_DGRAM );
	endpoint.bind( 0, Util::getLocalInterface() );
	Mercury::Address fakeAddr = endpoint.getLocalAddress();

	// Need to establish the channel with the initial packet.
	{
		generator.setReliable( 0, true );
		generator.setOnChannel( /*isCreate*/ true );
		args.seq = 0;
		generator.addFixedLengthMessage( CommonInterface::msg1.id(),
			&args, sizeof( args ) );

		Mercury::PacketPtr pPacket = generator.createPacket();
		generator.clearMessageData();

		packetReceiver.processPacket( fakeAddr, pPacket.get(), NULL );
	}

	CHECK( handler_.inOrder() );
	CHECK( handler_.numReceived() == 1 );

	const Mercury::SeqNum TEST_START_SEQ = 1;
	const Mercury::SeqNum TEST_END_SEQ = TEST_START_SEQ + 4097; 

	generator.setFragment( TEST_START_SEQ, TEST_END_SEQ );
	generator.setOnChannel( /*isCreate*/ false );

	for (Mercury::SeqNum seq = TEST_START_SEQ + 1; seq <= TEST_END_SEQ; ++seq)
	{
		generator.setReliable( seq );
		args.seq = seq;
		generator.addFixedLengthMessage( CommonInterface::msg1.id(),
			&args, sizeof( args ) );
		Mercury::PacketPtr pPacket = generator.createPacket();
		generator.clearMessageData();

		packetReceiver.processPacket( fakeAddr, pPacket.get(), NULL );
	}

	// Send the first packet now after forcing the buffered receives to double.
	{
		generator.setReliable( TEST_START_SEQ );
		args.seq = TEST_START_SEQ;
		generator.addFixedLengthMessage( CommonInterface::msg1.id(),
			&args, sizeof( args ) );
		Mercury::PacketPtr pPacket = generator.createPacket();
		generator.clearMessageData();

		packetReceiver.processPacket( fakeAddr, pPacket.get(), NULL );
	}

	CHECK( handler_.inOrder() );
	CHECK_EQUAL( TEST_END_SEQ + 1, handler_.numReceived() );
}

BW_END_NAMESPACE

// test_receive_window.cpp

