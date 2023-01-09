#include "pch.hpp"

#include "CppUnitLite2/src/CppUnitLite2.h"
#include "common_interface.hpp"

#include "network/reliable_order.hpp"

#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"
#include "network/channel_finder.hpp"


BW_BEGIN_NAMESPACE

// TODO: Regular channels never see a packet as lost if they
// never get an ACK (i.e. total loss of all reliable traffic)
//#define TEST_TOTAL_LOSS_ON_REGULAR_CHANNELS

// TODO: Piggybacks fail on indexed channels
//#define TEST_PIGGYBACKS_ON_INDEXED_CHANNELS
namespace TestReliable
{

typedef BW::vector< CommonInterface::msg1Args * > msg1ArgsVector;

/*
 * This handler runs until it has seen all the reliable
 * msg1s and a disconnect message.
 * Reliable message1s have an odd sequence number
 *
 * It will also send unreliable messges when asked to by a timer, to give
 * acks somewhere to ride.
 *
 * It includes a static method to generate an appropriate sequence
 */
class RunUntilCompleteHandler : public CommonHandler, public TimerHandler
{
public:
	RunUntilCompleteHandler( Mercury::EventDispatcher & dispatcher ) :
		dispatcher_( dispatcher ),
		lastSeq_( std::numeric_limits< uint32 >::max() ),
		seenSeqs_(),
		ticksSent_(),
		timedOut_( false )
	{
	}

	int ticksSent( Mercury::UDPChannel * pChannel )
	{
		return ticksSent_[ pChannel ];
	}

	bool timedOut() const
	{
		return timedOut_;
	}

	void timedOut( bool value )
	{
		timedOut_ = value;
	}

	// Load up a bundle with an appropriate set of messages for this handler.
	// Optionally populate a vector of pointers to the msg1 messages
	// Odd-numbered messages are reliable
	static void generateMessages( Mercury::Bundle & bundle, uint32 count,
									msg1ArgsVector * msg1s = NULL )
	{
		for (uint32 i = 0; i < count; ++i)
		{
			CommonInterface::msg1Args & msg1 = 
				CommonInterface::msg1Args::start( bundle,
					(i%2 == 1) ? Mercury::RELIABLE_DRIVER
							   : Mercury::RELIABLE_NO );
			msg1.seq = i;
			msg1.data = 0;
			if (msg1s)
			{
				msg1s->push_back( &msg1 );
			}
		}

		CommonInterface::disconnectArgs & disconnect =
			CommonInterface::disconnectArgs::start( bundle,
				Mercury::RELIABLE_DRIVER );
		disconnect.seq = count;
	}

protected:
	virtual void on_msg1( const Mercury::Address & srcAddr,
			const CommonInterface::msg1Args & msg1 )
	{
		if (msg1.data == 0)
		{
			gotSeq( msg1.seq );
		}
	}

	virtual void on_disconnect( const Mercury::Address & srcAddr,
			const CommonInterface::disconnectArgs & disconnect )
	{
		MF_ASSERT( disconnect.seq != std::numeric_limits< uint32 >::max() );
		lastSeq_ = disconnect.seq;
		gotSeq( disconnect.seq );
	}

	void handleTimeout( TimerHandle timer, void * arg )
	{
		if ( arg == NULL )
		{
			TRACE_MSG( "RunUntilCompleteHandler::handleTimeout: "
				"Ran out of time\n" );
			timedOut_ = true;
			dispatcher_.breakProcessing();
		} else {
			Mercury::UDPChannel * pChannel =
				*(static_cast<Mercury::UDPChannel **>( arg ));
			Mercury::Bundle & bundle = pChannel->bundle();

			CommonInterface::msg1Args & msg1 =
				CommonInterface::msg1Args::start( bundle,
					Mercury::RELIABLE_NO );
			msg1.seq = 0;
			msg1.data = 1;

			pChannel->send();

			++ticksSent_[pChannel];
		}
	}

private:
	void gotSeq( uint32 seq )
	{
		MF_ASSERT( seenSeqs_.count( seq ) == 0 );
		// Odd-numbered messages are reliable
		if (seq%2 == 1 || seq == lastSeq_)
		{
			seenSeqs_.insert( seq );

			if (lastSeq_ != std::numeric_limits< uint32 >::max()
				&& seenSeqs_.size() == lastSeq_ / 2 + 1)
			{
				dispatcher_.breakProcessing();
			}
		}
	}

	Mercury::EventDispatcher & dispatcher_;
	uint32 lastSeq_;
	BW::set<uint32> seenSeqs_;
	BW::map<Mercury::UDPChannel *, int> ticksSent_;
	bool timedOut_;
};


class MyChannelFinder : public Mercury::ChannelFinder
{
public:
	MyChannelFinder ( Mercury::UDPChannel*& myChannel ):
		myChannel_( myChannel )
	{
	}

	Mercury::UDPChannel* find( Mercury::ChannelID id, 
							const Mercury::Address & srcAddr,
							const Mercury::Packet * pPacket,
							bool & rHasBeenHandled )
	{
		if ( id == 1 )
		{
			rHasBeenHandled = false;
			return myChannel_;
		}
		return NULL;
	}

	Mercury::UDPChannel *& myChannel_;
};

struct ExternalLink : ReferenceCount {
	Mercury::EventDispatcher dispatcher_;
	Mercury::NetworkInterface fromInterface_;
	Mercury::NetworkInterface toInterface_;
	RunUntilCompleteHandler handler_;

	Mercury::UDPChannel * pFromChannel_;
	Mercury::UDPChannel * pToChannel_;
	MyChannelFinder fromChannelFinder_;
	MyChannelFinder toChannelFinder_;

#if ENABLE_WATCHERS
	Watcher * pWatcher_;
#endif // ENABLE_WATCHERS

	TimerHandle hFrom_;
	TimerHandle hTo_;

	ExternalLink( bool regular, bool indexed = false ) :
		dispatcher_(),
		fromInterface_( &dispatcher_, Mercury::NETWORK_INTERFACE_EXTERNAL ),
		toInterface_( &dispatcher_, Mercury::NETWORK_INTERFACE_EXTERNAL ),
		handler_( dispatcher_ ),
		pFromChannel_( new Mercury::UDPChannel( fromInterface_,
			toInterface_.address(), Mercury::UDPChannel::EXTERNAL,
			1.f, NULL, (indexed ? 1 : Mercury::CHANNEL_ID_NULL) )),
		pToChannel_( new Mercury::UDPChannel( toInterface_,
			fromInterface_.address(), Mercury::UDPChannel::EXTERNAL,
			1.f, NULL, (indexed ? 1 : Mercury::CHANNEL_ID_NULL) )),
		fromChannelFinder_( pFromChannel_ ),
		toChannelFinder_( pToChannel_ )
	{
#if ENABLE_WATCHERS
		pWatcher_ = &Watcher::rootWatcher();
#endif // ENABLE_WATCHERS

		fromInterface_.setIrregularChannelsResendPeriod( 0.1f );
		fromInterface_.pExtensionData( &handler_ );
		fromInterface_.registerChannelFinder( &fromChannelFinder_ );
		CommonInterface::registerWithInterface( fromInterface_ );
		pFromChannel_->isLocalRegular( regular );
		pFromChannel_->isRemoteRegular( regular );
#if ENABLE_WATCHERS
		pWatcher_->addChild( "fromInterface",
			Mercury::NetworkInterface::pWatcher(), &fromInterface_ );
#endif // ENABLE_WATCHERS

		toInterface_.setIrregularChannelsResendPeriod( 0.1f );
		toInterface_.pExtensionData( &handler_ );
		toInterface_.registerChannelFinder( &toChannelFinder_ );
		CommonInterface::registerWithInterface( toInterface_ );
		pToChannel_->isLocalRegular( regular );
		pToChannel_->isRemoteRegular( regular );
#if ENABLE_WATCHERS
		pWatcher_->addChild( "toInterface",
			Mercury::NetworkInterface::pWatcher(), &toInterface_ );
#endif // ENABLE_WATCHERS

		if (regular)
		{
			// Every so often, send an uninteresting packet through
			hFrom_ = dispatcher_.addTimer( 500000, &handler_, &pFromChannel_ );
			hTo_ = dispatcher_.addTimer( 500000, &handler_, &pToChannel_ );
		}
	}

	~ExternalLink()
	{
		hFrom_.cancel();
		hTo_.cancel();
#if ENABLE_WATCHERS
		pWatcher_->removeChild( "fromInterface" );
		pWatcher_->removeChild( "toInterface" );
#endif // ENABLE_WATCHERS
		pFromChannel_->destroy();
		pFromChannel_ = NULL;
		pToChannel_->destroy();
		pToChannel_ = NULL;
	}

#if ENABLE_WATCHERS
	template <class VALUE_TYPE>
	bool readWatcher( const char * path, VALUE_TYPE &value)
	{
		BW::string watcherVal;
		Watcher::Mode watcherMode;

		if (!pWatcher_->getAsString( NULL, path, watcherVal, watcherMode ))
		{
			return false;
		}
		return watcherStringToValue( watcherVal.c_str(), value );
	}
#endif // ENABLE_WATCHERS

	void run( int secondsToRun = 5 )
	{
		handler_.timedOut( false );
		TimerHandle timeout = dispatcher_.addOnceOffTimer(
			secondsToRun * 1000000, &handler_, NULL );
		dispatcher_.processUntilBreak();
		while (!handler_.timedOut() && pFromChannel_->hasUnackedPackets() )
		{
			pToChannel_->send();
			dispatcher_.processOnce();
		}
		if (!handler_.timedOut())
			timeout.cancel();
	}

	bool complete()
	{
		return !(handler_.timedOut() || pFromChannel_->hasUnsentData()
			|| pFromChannel_->hasUnackedPackets()
			|| pToChannel_->hasUnsentData()
			|| pToChannel_->hasUnackedPackets());
	}
};

typedef SmartPointer<ExternalLink> ExternalLinkPtr;

// Check that reliableOrder contains what we expect
TEST( Bundle_reliableOrder )
{
	ExternalLinkPtr pLink = new ExternalLink( false, false );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;
	msg1ArgsVector msg1s;

	const uint32 msgCount = 401;

	if (CommonInterface::msg1.lengthStyle() != Mercury::FIXED_LENGTH_MESSAGE)
	{
		FAIL( "Expected CommonInterface::msg1 to be FIXED_LENGTH\n" );
	}

	Mercury::UDPBundle & bundle = static_cast< Mercury::UDPBundle & >( 
		pChannel->bundle() );
	CHECK( bundle.isEmpty() );
	CHECK( !bundle.isReliable() );

	msg1s.reserve( msgCount );
	RunUntilCompleteHandler::generateMessages( bundle, msgCount, &msg1s );

	CHECK( bundle.isReliable() );

	CHECK_EQUAL( msgCount + 1, (unsigned int)bundle.numMessages() );
	const int numPackets = bundle.numDataUnits();

	// The meat of this test: Validate Bundle::reliableOrder_
	uint32 curMsg = 1;
	for (Mercury::Packet * pPacket = bundle.pFirstPacket();
			pPacket != NULL;
			pPacket = pPacket->next())
	{
		const Mercury::ReliableOrder * roBeg, * roEnd;
		bundle.reliableOrders( pPacket, roBeg, roEnd );

		Mercury::ReliableVector reliableOrders_( roBeg, roEnd );

		for (Mercury::ReliableVector::const_iterator ro =
					reliableOrders_.begin();
				ro != reliableOrders_.end() && curMsg < msgCount; ++ro)
		{
			// TODO: We can't check this, it's set by NetworkInterface::send
			// but calling pChannel->send() clears the bundle
			// XXX: We could cheat, we know unacked packets will be kept alive
			// until acked or piggybacked...
//			CHECK( pPacket->hasFlags(
//					Mercury::Packet::FLAG_IS_RELIABLE ) );
//			CHECK( pPacket->hasFlags(
//					Mercury::Packet::FLAG_HAS_SEQUENCE_NUMBER ) );

			// XXX: FIXED_LENGTH messages won't be broken across a packet.
			CHECK_EQUAL( 
				CommonInterface::msg1.nominalBodySize()
					+ CommonInterface::msg1.headerSize(),
				ro->segLength );

			CHECK( !memcmp( msg1s[ curMsg ],
				ro->segBegin + CommonInterface::msg1.headerSize(),
				ro->segLength -  + CommonInterface::msg1.headerSize() ) );
			// Odd-numbered messages are reliable and appear in reliableOrders
			curMsg += 2;
		}
	}
	CHECK( curMsg == msgCount || curMsg == msgCount + 1 );

	pChannel->send();

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	CHECK_EQUAL( numPackets,
		packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	CHECK_EQUAL( 0, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	CHECK_EQUAL( 0, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

// Four tests for losing a single packeet in a multi-packet bundle.
// Expect only the missing packet to be resent
TEST( Bundle_loseOnePacketOfMany_IrregularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( false, false );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;

	const uint32 msgCount = 401;

	Mercury::UDPBundle & bundle = static_cast< Mercury::UDPBundle & >( 
		pChannel->bundle() );

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	const int numPackets = bundle.numDataUnits();

	pLink->fromInterface_.dropNextSend();

	pChannel->send();

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// We should have sent one extra packet (a resend of the dropped packet)
	CHECK_EQUAL( numPackets + 1,
		packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// We should have sent one extra packet (a resend of the dropped packet)
	CHECK_EQUAL( 1, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Cannot piggy-back packets out of multi-packet bundles
	CHECK_EQUAL( 0, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

TEST( Bundle_loseOnePacketOfMany_RegularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( true, false );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;

	const uint32 msgCount = 401;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	const int numPackets = bundle.numDataUnits();

	pLink->fromInterface_.dropNextSend();

	pChannel->send();

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// We should have sent one extra packet (a resend of the dropped packet)
	CHECK_EQUAL( numPackets + 1,
		packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// We should have sent one extra packet (a resend of the dropped packet)
	CHECK_EQUAL( 1, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Cannot piggy-back packets out of multi-packet bundles
	CHECK_EQUAL( 0, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

TEST( Bundle_loseOnePacketOfMany_IndexedIrregularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( false, true );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;

	const uint32 msgCount = 401;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	const int numPackets = bundle.numDataUnits();

	pLink->fromInterface_.dropNextSend();

	pChannel->send();

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// We should have sent one extra packet (a resend of the dropped packet)
	CHECK_EQUAL( numPackets + 1,
		packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// We should have sent one extra packet (a resend of the dropped packet)
	CHECK_EQUAL( 1, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Cannot piggy-back packets out of multi-packet bundles
	CHECK_EQUAL( 0, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

TEST( Bundle_loseOnePacketOfMany_IndexedRegularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( true, true );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;

	const uint32 msgCount = 401;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	const int numPackets = bundle.numDataUnits();

	pLink->fromInterface_.dropNextSend();

	pChannel->send();

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// We should have sent one extra packet (a resend of the dropped packet)
	CHECK_EQUAL( numPackets + 1,
		packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// We should have sent one extra packet (a resend of the dropped packet)
	CHECK_EQUAL( 1, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Cannot piggy-back packets out of multi-packet bundles
	CHECK_EQUAL( 0, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

// Four tests for losing every packet in a multi-packet bundle.
// Expect only the missing packets to be resent
TEST( Bundle_loseAllPacketsOfMany_IrregularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( false, false );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;

	const uint32 msgCount = 401;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	const int numPackets = bundle.numDataUnits();

	if (bundle.numDataUnits() <= 1)
	{
		FAIL( "Not enough messages: will piggyback rather than resend\n" );
	}

	pLink->fromInterface_.setLossRatio( 1.f );

	pChannel->send();

	pLink->fromInterface_.setLossRatio( 0.f );

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// We should have sent one extra packet (a resend) for each packet
	// in the bundle
	CHECK_EQUAL( numPackets * 2,
		packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// We should have sent one extra packet (a resend) for each packet
	// in the bundle
	CHECK_EQUAL( numPackets, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Cannot piggy-back packets out of multi-packet bundles
	CHECK_EQUAL( 0, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

#ifdef TEST_TOTAL_LOSS_ON_REGULAR_CHANNELS
TEST( Bundle_loseAllPacketsOfMany_RegularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( true, false );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;

	const uint32 msgCount = 401;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	const int numPackets = bundle.numDataUnits();

	if (!bundle.isMultiPacket())
	{
		FAIL( "Not enough messages: will piggyback rather than resend\n" );
	}

	pLink->fromInterface_.setLossRatio( 1.f );

	pChannel->send();

	pLink->fromInterface_.setLossRatio( 0.f );

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// We should have sent one extra packet (a resend) for each packet
	// in the bundle
	CHECK_EQUAL( numPackets * 2,
		packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// We should have sent one extra packet (a resend) for each packet
	// in the bundle
	CHECK_EQUAL( numPackets, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Cannot piggy-back packets out of multi-packet bundles
	CHECK_EQUAL( 0, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}
#endif // TEST_TOTAL_LOSS_ON_REGULAR_CHANNELS

TEST( Bundle_loseAllPacketsOfMany_IndexedIrregularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( false, true );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;

	const uint32 msgCount = 401;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	const int numPackets = bundle.numDataUnits();

	if (bundle.numDataUnits() <= 1)
	{
		FAIL( "Not enough messages: will piggyback rather than resend\n" );
	}

	pLink->fromInterface_.setLossRatio( 1.f );

	pChannel->send();

	pLink->fromInterface_.setLossRatio( 0.f );

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// We should have sent one extra packet (a resend) for each packet
	// in the bundle
	CHECK_EQUAL( numPackets * 2,
		packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// We should have sent one extra packet (a resend) for each packet
	// in the bundle
	CHECK_EQUAL( numPackets, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Cannot piggy-back packets out of multi-packet bundles
	CHECK_EQUAL( 0, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

#ifdef TEST_TOTAL_LOSS_ON_REGULAR_CHANNELS
TEST( Bundle_loseAllPacketsOfMany_IndexedRegularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( true, true );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;

	const uint32 msgCount = 401;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	const int numPackets = bundle.numDataUnits();

	if (numPackets <= 1)
	{
		FAIL( "Not enough messages: will piggyback rather than resend\n" );
	}

	pLink->fromInterface_.setLossRatio( 1.f );

	pChannel->send();

	pLink->fromInterface_.setLossRatio( 0.f );

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// We should have sent one extra packet (a resend) for each packet
	// in the bundle
	CHECK_EQUAL( numPackets * 2,
		packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// We should have sent one extra packet (a resend) for each packet
	// in the bundle
	CHECK_EQUAL( numPackets, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Cannot piggy-back packets out of multi-packet bundles
	CHECK_EQUAL( 0, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}
#endif // TEST_TOTAL_LOSS_ON_REGULAR_CHANNELS

// Four tests for losing the only packet in a single-packet bundle.
// Expect the data to be piggybacked onto the next packet that goes by
// after the stack chooses to resend
TEST( Bundle_loseThePacketFromASinglePacketBundle_IrregularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( false, false );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;
	CHECK( pChannel->isExternal() );

	const uint32 msgCount = 20;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	if (bundle.numDataUnits() > 1)
	{
		FAIL( "Too many messages: cannot piggyback a multi-packet bundle\n" );
	}

	pLink->fromInterface_.setLossRatio( 1.f );

	pChannel->send();

	pLink->fromInterface_.setLossRatio( 0.f );

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// An irregular channel eventually times out and sends another packet
	CHECK_EQUAL( 2, packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// No resends
	CHECK_EQUAL( 0, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Piggyback one packet
	CHECK_EQUAL( 1, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

#ifdef TEST_TOTAL_LOSS_ON_REGULAR_CHANNELS
TEST( Bundle_loseThePacketFromASinglePacketBundle_RegularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( true, false );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;
	CHECK( pChannel->isExternal() );

	const uint32 msgCount = 20;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	if (bundle.isMultiPacket())
	{
		FAIL( "Too many messages: cannot piggyback a multi-packet bundle\n" );
	}

	pLink->fromInterface_.setLossRatio( 1.f );

	pChannel->send();

	pLink->fromInterface_.setLossRatio( 0.f );

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// Expect no extra packets sent beyond the ticks
	CHECK_EQUAL( 1, packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// No resends
	CHECK_EQUAL( 0, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Piggyback one packet
	CHECK_EQUAL( 1, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}
#endif // TEST_TOTAL_LOSS_ON_REGULAR_CHANNELS

#ifdef TEST_PIGGYBACKS_ON_INDEXED_CHANNELS
TEST( Bundle_loseThePacketFromASinglePacketBundle_IndexedIrregularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( false, true );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;
	CHECK( pChannel->isExternal() );

	const uint32 msgCount = 20;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	if (bundle.isMultiPacket())
	{
		FAIL( "Too many messages: cannot piggyback a multi-packet bundle\n" );
	}

	pLink->fromInterface_.setLossRatio( 1.f );

	pChannel->send();

	pLink->fromInterface_.setLossRatio( 0.f );

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// An irregular channel eventually times out and sends another packet
	CHECK_EQUAL( 2, packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// No resends
	CHECK_EQUAL( 0, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Piggyback one packet
	CHECK_EQUAL( 1, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

#ifdef TEST_TOTAL_LOSS_ON_REGULAR_CHANNELS
TEST( Bundle_loseThePacketFromASinglePacketBundle_IndexedRegularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( true, true );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;
	CHECK( pChannel->isExternal() );

	const uint32 msgCount = 20;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	if (bundle.isMultiPacket())
	{
		FAIL( "Too many messages: cannot piggyback a multi-packet bundle\n" );
	}

	pLink->fromInterface_.setLossRatio( 1.f );

	pChannel->send();

	pLink->fromInterface_.setLossRatio( 0.f );

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// Expect no extra packets sent beyond the ticks
	CHECK_EQUAL( 1, packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// No resends
	CHECK_EQUAL( 0, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Piggyback one packet
	CHECK_EQUAL( 1, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}
#endif // TEST_TOTAL_LOSS_ON_REGULAR_CHANNELS
#endif // TEST_PIGGYBACKS_ON_INDEXED_CHANNELS

// Four tests for losing the only packet in a single-packet bundle,
// then losing the packet it was piggybacked onto.
// We expect to have seen two piggybacks as the second lost packet
// will be piggybacked onto a third.
TEST( Bundle_loseThePacketFromASinglePacketBundleTwice_IrregularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( false, false );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;
	CHECK( pChannel->isExternal() );

	const uint32 msgCount = 20;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	if (bundle.numDataUnits() > 1)
	{
		FAIL( "Too many messages: cannot piggyback a multi-packet bundle\n" );
	}

	pLink->fromInterface_.setLossRatio( 1.f );

	pChannel->send();

	// XXX: This is a little bodgie... MinRTT is one second, so we resend
	// after two seconds. But if we resend twice, we'll have three piggybacks,
	// not two.
	pLink->run( 3 );
	CHECK( !pLink->complete() );

	pLink->fromInterface_.setLossRatio( 0.f );

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// An irregular channel eventually times out and sends another packet, twice
	CHECK_EQUAL( 3, packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// No resends
	CHECK_EQUAL( 0, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Piggyback one packet twice
	CHECK_EQUAL( 2, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

#ifdef TEST_TOTAL_LOSS_ON_REGULAR_CHANNELS
TEST( Bundle_loseThePacketFromASinglePacketBundleTwice_RegularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( true, false );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;
	CHECK( pChannel->isExternal() );

	const uint32 msgCount = 20;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	if (bundle.isMultiPacket())
	{
		FAIL( "Too many messages: cannot piggyback a multi-packet bundle\n" );
	}

	pLink->fromInterface_.setLossRatio( 1.f );

	pChannel->send();

	// XXX: This is a little bodgie... MinRTT is one second, so we resend
	// after two seconds. But if we resend twice, we'll have three piggybacks,
	// not two.
	pLink->run( 3 );
	CHECK( !pLink->complete() );

	pLink->fromInterface_.setLossRatio( 0.f );

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// Expect no extra packets sent beyond the ticks
	CHECK_EQUAL( 1, packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// No resends
	CHECK_EQUAL( 0, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Piggyback one packet twice
	CHECK_EQUAL( 2, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}
#endif // TEST_TOTAL_LOSS_ON_REGULAR_CHANNELS

#ifdef TEST_PIGGYBACKS_ON_INDEXED_CHANNELS
TEST( Bundle_loseThePacketFromASinglePacketBundleTwice_IndexedIrregularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( false, true );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;
	CHECK( pChannel->isExternal() );

	const uint32 msgCount = 20;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	if (bundle.isMultiPacket())
	{
		FAIL( "Too many messages: cannot piggyback a multi-packet bundle\n" );
	}

	pLink->fromInterface_.setLossRatio( 1.f );

	pChannel->send();

	// XXX: This is a little bodgie... MinRTT is one second, so we resend
	// after two seconds. But if we resend twice, we'll have three piggybacks,
	// not two.
	pLink->run( 3 );
	CHECK( !pLink->complete() );

	pLink->fromInterface_.setLossRatio( 0.f );

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// An irregular channel eventually times out and sends another packet, twice
	CHECK_EQUAL( 3, packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// No resends
	CHECK_EQUAL( 0, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Piggyback one packet twice
	CHECK_EQUAL( 2, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

#ifdef TEST_TOTAL_LOSS_ON_REGULAR_CHANNELS
TEST( Bundle_loseThePacketFromASinglePacketBundleTwice_IndexedRegularChannel )
{
	ExternalLinkPtr pLink = new ExternalLink( true, true );

	Mercury::UDPChannel * pChannel = pLink->pFromChannel_;
	CHECK( pChannel->isExternal() );

	const uint32 msgCount = 20;

	Mercury::Bundle & bundle = pChannel->bundle();

	RunUntilCompleteHandler::generateMessages( bundle, msgCount, NULL );

	if (bundle.isMultiPacket())
	{
		FAIL( "Too many messages: cannot piggyback a multi-packet bundle\n" );
	}

	pLink->fromInterface_.setLossRatio( 1.f );

	pChannel->send();

	// XXX: This is a little bodgie... MinRTT is one second, so we resend
	// after two seconds. But if we resend twice, we'll have three piggybacks,
	// not two.
	pLink->run( 3 );
	CHECK( !pLink->complete() );

	pLink->fromInterface_.setLossRatio( 0.f );

	pLink->run();
	if( !pLink->complete() )
	{
		FAIL( "ExternalLink failed to processes all packets" );
	}

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	// Expect no extra packets sent beyond the ticks
	CHECK_EQUAL( 1, packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	// No resends
	CHECK_EQUAL( 0, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	// Piggyback one packet twice
	CHECK_EQUAL( 2, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}
#endif // TEST_TOTAL_LOSS_ON_REGULAR_CHANNELS
#endif // TEST_PIGGYBACKS_ON_INDEXED_CHANNELS

} // TestReliable namespace

BW_END_NAMESPACE

// test_piggybacks.cpp
