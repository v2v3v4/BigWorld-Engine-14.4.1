#include "pch.hpp"

#include "CppUnitLite2/src/CppUnitLite2.h"
#include "common_interface.hpp"

#include "network/reliable_order.hpp"

#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"
#include "network/channel_finder.hpp"

/* Testing the following failure:
WorldOfWarplanes.exe!RaiseEngineException(const char * msg=0x0017e9f4)  Line 733 + 0x9 bytes C++
  WorldOfWarplanes.exe!DebugMsgHelper::criticalMessageHelper(bool isDevAssertion=false, const char * format=0x01bde3b8, char * argPtr=0x0018ea14)  Line 350 + 0xa bytes C++
  WorldOfWarplanes.exe!DebugMsgHelper::criticalMessage(const char * format=0x01bde3b8, ...)  Line 148 C++
  WorldOfWarplanes.exe!EntityDescription::clientServerProperty(unsigned int n=94)  Line 1517 + 0x2e bytes C++
  WorldOfWarplanes.exe!Entity::getPropertyStreamSize(int propertyID=94)  Line 1909 + 0xc bytes C++
  WorldOfWarplanes.exe!EntityManager::getEntityPropertyStreamSize(int id=2417, int propertyID=94)  Line 791 + 0xc bytes C++
  WorldOfWarplanes.exe!ServerConnection::entityPropertyGetStreamSize(unsigned char msgID)  Line 1049 + 0x12 bytes C++
  WorldOfWarplanes.exe!ClientCallbackMessageHandler::getMessageStreamSize(Mercury::NetworkInterface & networkInterface={...}, unsigned char msgID)  Line 165 C++
  WorldOfWarplanes.exe!Mercury::InterfaceElement::updateLengthDetails(Mercury::NetworkInterface & networkInterface={...})  Line 59 C++
  WorldOfWarplanes.exe!Mercury::Bundle::dispatchMessages(Mercury::InterfaceTable & interfaceTable={...}, const Mercury::Address & addr={...}, Mercury::Channel * pChannel=0x1161b878, Mercury::NetworkInterface & networkInterface={...}, Mercury::ProcessSocketStatsHelper * pStatsHelper=0x0018f2a4)  Line 821 C++
  WorldOfWarplanes.exe!Mercury::PacketReceiver::processOrderedPacket(const Mercury::Address & addr={...}, Mercury::Packet * p=0x29150be0, Mercury::Channel * pChannel=0x1161b878, Mercury::ProcessSocketStatsHelper * pStatsHelper=0x0018f2a4)  Line 967 + 0x2c bytes C++
  WorldOfWarplanes.exe!Mercury::PacketReceiver::processFilteredPacket(const Mercury::Address & addr={...}, Mercury::Packet * p=0x29150be0, Mercury::ProcessSocketStatsHelper * pStatsHelper=0x0018f2a4)  Line 722 C++
  WorldOfWarplanes.exe!Mercury::`anonymous namespace'::PiggybackProcessor::onPacket(SmartPointer<Mercury::Packet> pPacket={...})  Line 769 C++
  WorldOfWarplanes.exe!Mercury::Packet::processPiggybackPackets(Mercury::PacketVisitor & visitor={...})  Line 380 + 0x1a bytes C++
> WorldOfWarplanes.exe!Mercury::PacketReceiver::processPiggybacks(const Mercury::Address & addr={...}, Mercury::Packet * pPacket=0x243399c0, Mercury::ProcessSocketStatsHelper * pStatsHelper=0x0018f2a4)  Line 790 C++
  WorldOfWarplanes.exe!Mercury::PacketReceiver::processFilteredPacket(const Mercury::Address & addr={...}, Mercury::Packet * p=0x243399c0, Mercury::ProcessSocketStatsHelper * pStatsHelper=0x0018f2a4)  Line 347 + 0x11 bytes C++
  WorldOfWarplanes.exe!Mercury::PacketFilter::recv(Mercury::PacketReceiver & receiver={...}, const Mercury::Address & addr={...}, Mercury::Packet * pPacket=0x243399c0, Mercury::ProcessSocketStatsHelper * pStatsHelper=0x0018f2a4)  Line 38 C++
  WorldOfWarplanes.exe!Mercury::EncryptionFilter::recv(Mercury::PacketReceiver & receiver={...}, const Mercury::Address & addr={...}, Mercury::Packet * pPacket=0x243399c0, Mercury::ProcessSocketStatsHelper * pStatsHelper=0x0018f2a4)  Line 233 + 0x1c bytes C++
  WorldOfWarplanes.exe!Mercury::PacketReceiver::processPacket(const Mercury::Address & addr={...}, Mercury::Packet * p=0x0493cc40, Mercury::ProcessSocketStatsHelper * pStatsHelper=0x0018f2a4)  Line 242 + 0x25 bytes C++
  WorldOfWarplanes.exe!Mercury::PacketReceiver::processSocket(bool expectingPacket=true)  Line 92 C++
  WorldOfWarplanes.exe!Mercury::PacketReceiver::handleInputNotification(int fd=684)  Line 51 + 0x9 bytes C++
  WorldOfWarplanes.exe!Mercury::SelectPoller::handleInputNotifications(int & countReady=, fd_set & readFDs={...}, fd_set & writeFDs={...})  Line 305 + 0x29 bytes C++
  WorldOfWarplanes.exe!Mercury::SelectPoller::processPendingEvents(double maxWait=0.00000000000000000)  Line 398 + 0x19 bytes C++
  WorldOfWarplanes.exe!Mercury::DispatcherCoupling::doTask()  Line 34 + 0x38 bytes C++
  WorldOfWarplanes.exe!Mercury::FrequentTasks::process()  Line 112 C++
  WorldOfWarplanes.exe!Mercury::EventDispatcher::processOnce(bool shouldIdle=false)  Line 381 C++
  WorldOfWarplanes.exe!ServerConnection::processInput()  Line 922 C++
=====
WorldOfWarplanes.exe!Mercury::PacketReceiver::processPiggybacks(const Mercury::Address & addr={...}, Mercury::Packet * pPacket=0x243399c0, Mercury::ProcessSocketStatsHelper * pStatsHelper=0x0018f2a4) Line 790 C++
Is still actual.
Reproduce is packet size more than 255 bytes and get packet lost
*/

BW_BEGIN_NAMESPACE
namespace
{

/*
 * This handler runs until it has seen all the reliable
 * msgVar1s and a disconnect message.
 *
 * It will also send unreliable messages when asked to by a timer, to give
 * acks and piggybacks somewhere to ride.
 *
 * It includes a static method to generate an appropriate sequence
 */
class RunUntilCompleteHandler : public CommonHandler, public TimerHandler
{
public:
	RunUntilCompleteHandler( Mercury::EventDispatcher & dispatcher ) :
		dispatcher_( dispatcher ),
		lastSeq_( std::numeric_limits< uint32 >::max() ),
		nextSeq_( 0 ),
		ticksSent_(),
		timedOut_( false )
	{
	}

	int ticksSent( Mercury::Channel * pChannel )
	{
		return ticksSent_[ pChannel ];
	}

	bool timedOut() const
	{
		return timedOut_;
	}

	bool hasReceivedCorrectly() const
	{
		return lastSeq_ != std::numeric_limits< uint32 >::max() &&
			nextSeq_ == lastSeq_ + 1;
	}

	uint32 expectedSeq() const
	{
		return nextSeq_;
	}

	void timedOut( bool value )
	{
		timedOut_ = value;
	}

	// Send a sequence of bundles, each with a message at least 8
	// bytes long, starting with a sequence number and the message size.
	static void generateBundles( Mercury::Channel & channel, uint32 msgCount,
		uint32 msgSize, uint32 sendFrequency )
	{
		if (msgSize < sizeof(uint32) * 2)
		{
			msgSize = sizeof(uint32) * 2;
		}

		int fullMsgSize = msgSize + CommonInterface::msgVar1.headerSize();
		if (!CommonInterface::msgVar1.canHandleLength( msgSize ))
		{
			// This is the data from specialCompressLength
			fullMsgSize += sizeof(int32);
		}

		for (uint32 seq = 0; seq < msgCount; ++seq)
		{
			Mercury::UDPBundle & bundle = static_cast< Mercury::UDPBundle & >(
				channel.bundle() );

			bundle.startMessage( CommonInterface::msgVar1,
				Mercury::RELIABLE_DRIVER );

			bundle << seq;
			bundle << msgSize;

			for (uint32 i = 0; i < (msgSize - sizeof(uint32) * 2); ++i)
			{
				bundle << uint8(i % 256);
			}

			if (seq % sendFrequency == sendFrequency - 1)
			{
				// Force the last message to have its size compressed
				bundle.finalise();

				// Validate reliableOrders
				uint32 reliableSize = 0;
				for (Mercury::Packet * pPacket = bundle.pFirstPacket();
					pPacket != NULL;
					pPacket = pPacket->next() )
				{
					const Mercury::ReliableOrder * roBeg, * roEnd;
					bundle.reliableOrders( pPacket, roBeg, roEnd );
					Mercury::ReliableVector reliableOrders( roBeg, roEnd );

					for (Mercury::ReliableVector::const_iterator ro =
								reliableOrders.begin();
							ro != reliableOrders.end();
							++ro )
					{
						reliableSize += ro->segLength;
					}
				}

				MF_ASSERT( reliableSize == sendFrequency * fullMsgSize );

				channel.send();
			}
		}

		Mercury::Bundle & bundle = channel.bundle();
		CommonInterface::disconnectArgs & disconnect =
			CommonInterface::disconnectArgs::start( bundle,
				Mercury::RELIABLE_DRIVER );
		disconnect.seq = msgCount;
		channel.send();
	}

	// Send a sequence of bundles, each with a message at least 8
	// bytes long, starting with a sequence number and the message size.
	// Unlike generateBundles, this puts them at the end, to ensure that
	// specialCompressLength() will create a new packet if called.
	static void generateBundlesAtEnd( Mercury::Channel & channel,
		uint32 msgCount, uint32 msgSize, uint32 sendFrequency )
	{
		if (msgSize < sizeof(uint32) * 2)
		{
			msgSize = sizeof(uint32) * 2;
		}

		int fullMsgSize = msgSize + CommonInterface::msgVar1.headerSize();
		if (!CommonInterface::msgVar1.canHandleLength( msgSize ))
		{
			// This is the data from specialCompressLength
			fullMsgSize += sizeof(int32);
		}

		for (uint32 seq = 0; seq < msgCount; ++seq)
		{
			Mercury::UDPBundle & bundle = static_cast< Mercury::UDPBundle & >(
				channel.bundle() );

			// Pad out with a non-reliable message we don't care about
			bundle.startMessage( CommonInterface::msgVar2,
				Mercury::RELIABLE_NO );

			// Ensure we're one byte short to hold fullMsgSize
			// This assumes we're not going to accidentally trigger
			// specialCompressLength on _this_ message, or we'll
			// be multi-packet too early.
			while (bundle.freeBytesInLastDataUnit() > fullMsgSize - 1)
			{
				bundle << (uint8)0xff;
			}

			bundle.startMessage( CommonInterface::msgVar1,
				Mercury::RELIABLE_DRIVER );

			bundle << seq;
			bundle << msgSize;

			for (uint32 i = 0; i < (msgSize - sizeof(uint32) * 2); ++i)
			{
				bundle << uint8(i % 256);
			}

			if (seq % sendFrequency == sendFrequency - 1)
			{
				// If we only put one reliable packet into the bundle,
				// then we shouldn't be multi-packet before finalise()
				MF_ASSERT( sendFrequency > 1 ||
					!bundle.hasMultipleDataUnits() );

				// Force the last message to have its size compressed
				bundle.finalise();

				MF_ASSERT( bundle.hasMultipleDataUnits() );

				// Validate reliableOrders
				uint32 reliableSize = 0;
				for (Mercury::Packet * pPacket = bundle.pFirstPacket();
					pPacket != NULL;
					pPacket = pPacket->next() )
				{
					const Mercury::ReliableOrder * roBeg, * roEnd;
					bundle.reliableOrders( pPacket, roBeg, roEnd );
					Mercury::ReliableVector reliableOrders( roBeg, roEnd );

					for (Mercury::ReliableVector::const_iterator ro =
								reliableOrders.begin();
							ro != reliableOrders.end();
							++ro )
					{
						reliableSize += ro->segLength;
					}
				}

				MF_ASSERT( reliableSize == sendFrequency * fullMsgSize );

				channel.send();
			}
		}

		Mercury::Bundle & bundle = channel.bundle();
		CommonInterface::disconnectArgs & disconnect =
			CommonInterface::disconnectArgs::start( bundle,
			Mercury::RELIABLE_DRIVER );
		disconnect.seq = msgCount;
		channel.send();
	}

protected:
	virtual void on_msgVar1( const Mercury::Address & srcAddr,
			BinaryIStream & data )
	{
		uint32 size = data.remainingLength();
		uint32 seq;
		data >> seq;
		uint32 sizeData;
		data >> sizeData;
		MF_ASSERT( sizeData == size );
		data.finish();
		gotSeq( seq );
	}

	virtual void on_disconnect( const Mercury::Address & srcAddr,
			const CommonInterface::disconnectArgs & disconnect )
	{
		MF_ASSERT( disconnect.seq != std::numeric_limits< uint32 >::max() );
		MF_ASSERT( lastSeq_ == std::numeric_limits< uint32 >::max() );
		lastSeq_ = disconnect.seq;
		gotSeq( disconnect.seq );
		dispatcher_.breakProcessing();
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
			Mercury::Channel * pChannel =
				*(static_cast<Mercury::Channel **>( arg ));
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
		if (seq == nextSeq_)
		{
			// Expected message arrived
			nextSeq_ = seq + 1;
			return;
		}

		// Didn't get the seq we wanted. Abort!
		dispatcher_.breakProcessing();
	}

	Mercury::EventDispatcher & dispatcher_;
	uint32 lastSeq_;
	uint32 nextSeq_;
	BW::map<Mercury::Channel *, int> ticksSent_;
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
		BW::string desc;
		Watcher::Mode watcherMode;

		if (!pWatcher_->getAsString( NULL, path, watcherVal, desc, watcherMode ))
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

} // anonymous namespace

TEST( Large_piggyback_compress_length_adds_new_packet )
{
	ExternalLinkPtr pLink = new ExternalLink( true, false );

	Mercury::Channel * pChannel = pLink->pFromChannel_;

	const uint32 msgCount = 1;
	const uint32 msgPerPacket = 1;
	const uint32 msgSize = 256;

	pLink->fromInterface_.dropNextSend();

	RunUntilCompleteHandler::generateBundlesAtEnd( *pChannel, msgCount, msgSize,
		msgPerPacket );

	pLink->run();

	CHECK( pLink->complete() );
	CHECK( pLink->handler_.hasReceivedCorrectly() );
	CHECK_EQUAL( msgCount + 1, pLink->handler_.expectedSeq() );

	// Note that this test is kind-of invalid.
	// Specifically, a multi-packet bundle cannot have any of its
	// packets piggybacked, so we won't ever see the bug in
	// reliableOrders these tests are looking for.
#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
		"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	CHECK_EQUAL( 4, packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
		"fromInterface/sending/stats/totals/packetsResent",
		packetsResent ) );
	CHECK_EQUAL( 1, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
		"fromInterface/sending/stats/totals/packetsPiggybacked",
		packetsPiggybacked ) );
	CHECK_EQUAL( 0, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

TEST( Large_piggyback_lost_two_per_packet )
{
	ExternalLinkPtr pLink = new ExternalLink( true, false );

	Mercury::Channel * pChannel = pLink->pFromChannel_;

	const uint32 msgCount = 5;
	const uint32 msgPerPacket = 2;
	const uint32 msgSize = 256;

	pLink->fromInterface_.dropNextSend();

	RunUntilCompleteHandler::generateBundles( *pChannel, msgCount, msgSize,
		msgPerPacket );

	pLink->run();

	CHECK( pLink->complete() );
	CHECK( pLink->handler_.hasReceivedCorrectly() );
	CHECK_EQUAL( msgCount + 1, pLink->handler_.expectedSeq() );

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	CHECK_EQUAL( 3, packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	CHECK_EQUAL( 0, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	CHECK_EQUAL( 1, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

TEST( Large_piggyback_lost_one_per_packet )
{
	ExternalLinkPtr pLink = new ExternalLink( true, false );

	Mercury::Channel * pChannel = pLink->pFromChannel_;

	const uint32 msgCount = 5;
	const uint32 msgPerPacket = 1;
	const uint32 msgSize = 256;

	pLink->fromInterface_.dropNextSend();

	RunUntilCompleteHandler::generateBundles( *pChannel, msgCount, msgSize,
		msgPerPacket );

	pLink->run();

	CHECK( pLink->complete() );
	CHECK( pLink->handler_.hasReceivedCorrectly() );
	CHECK_EQUAL( msgCount + 1, pLink->handler_.expectedSeq() );

#if ENABLE_WATCHERS
	int packetsSent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsSent", packetsSent ) );
	CHECK_EQUAL( 6, packetsSent - pLink->handler_.ticksSent( pChannel ) );

	int packetsResent = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsResent",
			packetsResent ) );
	CHECK_EQUAL( 0, packetsResent );

	int packetsPiggybacked = std::numeric_limits< int >::max();
	CHECK( pLink->readWatcher(
			"fromInterface/sending/stats/totals/packetsPiggybacked",
			packetsPiggybacked ) );
	CHECK_EQUAL( 1, packetsPiggybacked );
#endif // ENABLE_WATCHERS
}

BW_END_NAMESPACE

// test_piggybacks_longer_than_normal.cpp

