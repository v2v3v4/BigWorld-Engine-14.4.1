#include "pch.hpp"

#include "common_interface.hpp"

#include "packet_generator.hpp"

#include "network/channel_finder.hpp"
#include "network/event_dispatcher.hpp"
#include "network/misc.hpp"
#include "network/network_interface.hpp"
#include "network/packet_receiver.hpp"
#include "network/udp_channel.hpp"

#include <memory>
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

namespace // (anonymous)
{

const Mercury::ChannelID CHANNEL_ID = 0x01;

} // end namespace (anonymous)


class TestChannelVersionLocalHandler : public CommonHandler
{
public:
	TestChannelVersionLocalHandler( Mercury::UDPChannelPtr pChannel ):
			CommonHandler(),
			count_( 0 ),
			pChannel_( pChannel )
	{}

	virtual ~TestChannelVersionLocalHandler()
	{}

	void reset()
		{ count_ = 0; }

	uint count() const 
		{ return count_; }

protected:
	virtual void on_msg1( const Mercury::Address & srcAddr, 
			const CommonInterface::msg1Args & args )
	{
		INFO_MSG( "on_msg1: args.seq = %u, args.data = %u\n", 
			args.seq, args.data );
		++count_;

		if (args.data)
		{
			// This simulates the base entity channel receiving cellEntityLost
			// from a recently destroyed cell entity, and then receiving
			// currentCell for a new cell entity.
			Mercury::Address addr = pChannel_->addr();
			pChannel_->reset( Mercury::Address::NONE, false );
			pChannel_->setAddress( addr );
		}
	}

private:
	uint count_;
	Mercury::UDPChannelPtr pChannel_;
};

class SingleChannelFinder : public Mercury::ChannelFinder
{
public:
	SingleChannelFinder( Mercury::UDPChannelPtr pChannel ):
			pChannel_( pChannel )
	{}

	virtual ~SingleChannelFinder()
	{}


	virtual Mercury::UDPChannel * find( Mercury::ChannelID id, 
		const Mercury::Address & srcAddr,
		const Mercury::Packet * pPacket,
		bool & rHasBeenHandled )
	{
		rHasBeenHandled = false;
		return pChannel_.get();
	}


private:
	Mercury::UDPChannelPtr pChannel_;
};

class TestChannelVersionFixture
{
public:
	TestChannelVersionFixture():
			pHandler_(),
			eventDispatcher_(),
			networkInterface_( &eventDispatcher_, 
				Mercury::NETWORK_INTERFACE_INTERNAL ),
			packetReceiver_( networkInterface_.socket(), 
				networkInterface_ ),
			endpoint_(),
			pChannel_( NULL ),
			pChannelFinder_( NULL ),
			nextSeqNum_( 0 ),
			version_( 0 )
	{
		CommonInterface::registerWithInterface( networkInterface_ );

		endpoint_.socket( SOCK_DGRAM );
		endpoint_.bind();

		pChannel_ = new Mercury::UDPChannel( networkInterface_, 
				endpoint_.getLocalAddress(), Mercury::UDPChannel::INTERNAL, 
				DEFAULT_INACTIVITY_RESEND_DELAY, NULL, CHANNEL_ID );

		pHandler_.reset( new TestChannelVersionLocalHandler( pChannel_ ) );
		networkInterface_.pExtensionData( pHandler_.get() );

		pChannelFinder_.reset( new SingleChannelFinder( pChannel_ ) );
		networkInterface_.registerChannelFinder( pChannelFinder_.get() );

		this->setupChannel();

		pHandler_->reset();
	}


	void setupChannel()
	{
		PacketGenerator generator;

		// We need to ensure that the channel is ready, this is to set up and
		// send the CREATE_CHANNEL packet.
		generator.setIndexedChannel( CHANNEL_ID, version_ );
		generator.setReliable( this->nextSeqNum() ); // should be packet 0
		generator.setOnChannel( true );

		Mercury::PacketPtr pPacket( generator.createPacket() );

		packetReceiver_.processPacket( endpoint_.getLocalAddress(),
			pPacket.get(), NULL );
	}

	~TestChannelVersionFixture()
	{
		pHandler_.reset();
		pChannel_->destroy();
	}

protected:
	Mercury::SeqNum version() const
		{ return version_; }

	Mercury::SeqNum nextVersion()
		{ return ++version_; }

	Mercury::SeqNum nextSeqNum()
		{ return nextSeqNum_++; }

	void resetSequenceNumber()
		{ nextSeqNum_ = 0; }


	std::auto_ptr< TestChannelVersionLocalHandler > pHandler_;

	Mercury::EventDispatcher eventDispatcher_;
	Mercury::NetworkInterface networkInterface_;

	Mercury::PacketReceiver packetReceiver_;	
	// Pretend it came from this address, acks will be sent here.
	Endpoint endpoint_;

	Mercury::UDPChannelPtr pChannel_;
	std::auto_ptr< SingleChannelFinder > pChannelFinder_;

private:
	Mercury::SeqNum nextSeqNum_;
	Mercury::ChannelVersion version_;
};


/**
 *	This case tests that an indexed channel will drop packets with older
 *	versions than its creation version.
 *
 *	We use the msg1 arg data member to determine if we should change the
 *	creation channel version. The msg1 seq data member indicates the channel
 *	version to change to.
 *
 *	We simulate the case where a cell entity is destroyed and recreated.
 *	The cell entity will send a cellEntityLost message to the base, which
 *	receives it and resets the channel, incrementing the version. 
 *	The simulated source indexed-channel packets are:
 *	A. version 0, seq S, dummy data.
 *	B. version 0, seq S + 1, creation version change data to version 1.
 *	C. version 1, seq 0, dummy data.
 *
 * 	Note that C cannot be received before B has been processed, as C is coming
 * 	from the new cell entity, and can not be created before B has been
 * 	processed.
 *
 *	We simulate packet sends in the following order (note the repetitions
 *	indicate simulated resends): 
 *		[B[0], A[0], B[1], A[1], C]
 *
 *	A[0] will be buffered because it is a future packet above inSeqAt.
 *	A[0] will be processed. This causes the buffered B[0] to be processed
 *	immediately afterwards. 
 *	B[1] will be dropped because it has a lower version number.
 *	A[1] will be dropped because it has a lower version number.
 *	C is processed.
 *
 */
TEST_F( TestChannelVersionFixture, ChannelVersion )
{
	typedef BW::vector< PacketGenerator > PacketGenerators;
	PacketGenerators generators( 3 );

	CommonInterface::msg1Args args;
	args.traits = Mercury::UDPChannel::INTERNAL;

	// Packet A
	generators[0].setIndexedChannel( CHANNEL_ID, this->version() );
	generators[0].setReliable( this->nextSeqNum() );
	generators[0].setOnChannel();
	args.seq = 0;
	args.data = 0;
	generators[0].addFixedLengthMessage( CommonInterface::msg1.id(),
		&args, sizeof( args ) );

	// Packet B
	generators[1].setIndexedChannel( CHANNEL_ID, this->version() );
	generators[1].setReliable( this->nextSeqNum() );
	generators[1].setOnChannel();
	// Processing this packet will change the channel creation version to the
	// next version.
	args.seq = this->nextVersion();
	args.data = 1;
	generators[1].addFixedLengthMessage( CommonInterface::msg1.id(),
		&args, sizeof( args ) );

	// We are now talking to the new cell entity.
	this->resetSequenceNumber();

	// Packet C
	generators[2].setIndexedChannel( CHANNEL_ID, this->version() );
	Mercury::SeqNum seqNum = this->nextSeqNum();
	CHECK_EQUAL( 0U, seqNum );
	generators[2].setReliable( seqNum );
	generators[2].setOnChannel();
	args.seq = 0;
	args.data = 0;
	generators[2].addFixedLengthMessage( CommonInterface::msg1.id(),
		&args, sizeof( args ) );

	typedef BW::vector< size_t > SendOrder;
	SendOrder sendOrder;
	sendOrder.push_back( 1 );
	sendOrder.push_back( 0 );
	sendOrder.push_back( 1 );
	sendOrder.push_back( 0 );
	sendOrder.push_back( 2 );

	// Serve it up!
	SendOrder::const_iterator iOrder = sendOrder.begin();
	while (iOrder != sendOrder.end())
	{
		INFO_MSG( "Sending packet #%c\n", 'A' + uint8( *iOrder ) );
		Mercury::PacketPtr pPacket = generators[*iOrder].createPacket();
		packetReceiver_.processPacket( endpoint_.getLocalAddress(),
			pPacket.get(), NULL );
		++iOrder;
	}

	CHECK_EQUAL( 3U, pHandler_->count() );
	CHECK_EQUAL( 1U, pChannel_->creationVersion() );
}

BW_END_NAMESPACE

// test_channel_version.cpp

