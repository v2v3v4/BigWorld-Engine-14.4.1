#include "pch.hpp"

#include "once_off_packet.hpp"

#include "event_dispatcher.hpp"
#include "network_interface.hpp"
#include "packet_sender.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: OnceOffReceiver
// -----------------------------------------------------------------------------

OnceOffReceiver::OnceOffReceiver() :
	currOnceOffReceipts_(),
	prevOnceOffReceipts_(),
	fragmentedBundles_(),
	clearFragmentedBundlesTimerHandle_(),
	onceOffPacketCleaningTimerHandle_()
{
}


/**
 *	Destructor.
 */
OnceOffReceiver::~OnceOffReceiver()
{
}


/**
 *	This method initialises this object.
 */
void OnceOffReceiver::init( EventDispatcher & dispatcher )
{
	// Clear stale incomplete fragmented bundles every so often.
	clearFragmentedBundlesTimerHandle_ = dispatcher.addTimer(
		FragmentedBundle::MAX_AGE * 1000000, this, NULL,
		"ClearFragmentedBundles" );
}


/**
 *	This method finalises this object.
 */
void OnceOffReceiver::fini()
{
	onceOffPacketCleaningTimerHandle_.cancel();
	clearFragmentedBundlesTimerHandle_.cancel();
}


/**
 *	This method records the receipt of a packet sent once-off reliably. This
 *	needs to be done because we may receive this packet again but we should not
 *	process it again.
 *
 *  Returns true if the packet had already been received.
 */
bool OnceOffReceiver::onReliableReceived( EventDispatcher & dispatcher,
		const Address & addr, SeqNum seq )
{
	this->initOnceOffPacketCleaning( dispatcher );

	OnceOffReceipt oor( addr, seq );

	if (currOnceOffReceipts_.find( oor ) != currOnceOffReceipts_.end() ||
		prevOnceOffReceipts_.find( oor ) != prevOnceOffReceipts_.end())
	{
		// ++numOnceOffReliableDupesReceived_;
		TRACE_MSG( "OnceOffReceiver::onReliableReceived( %s ): "
				"#%u already received\n",
			addr.c_str(), seq );

		return true;
	}

	// ++numOnceOffReliableReceived_;
	currOnceOffReceipts_.insert( oor );

	return false;
}


/**
 *	This method makes sure that the cleanup timer is initialised.
 */
void OnceOffReceiver::initOnceOffPacketCleaning( EventDispatcher & dispatcher )
{
	if (!onceOffPacketCleaningTimerHandle_.isSet())
	{
		// TODO: Should remove this magic number. It should be dependant on
		// onceOffMaxResends_, onceOffResendPeriod_ and the timeout period.
		long onceOffPacketCleaningPeriod = 30000000; // 30 seconds
		onceOffPacketCleaningTimerHandle_ =
			dispatcher.addTimer( onceOffPacketCleaningPeriod, this, NULL,
			"OnceOffPacketCleaning" );
	}
}


/**
 *	This method cleans up old information about received once-off reliable
 *	packets. This is done by keeping two collections - one for the recent
 *	information and another for the less recent information. This method clears
 *	the less recent information and demotes the recent information to be less
 *	recent.
 */
void OnceOffReceiver::onceOffReliableCleanup()
{
	prevOnceOffReceipts_.clear();
	currOnceOffReceipts_.swap( prevOnceOffReceipts_ );
}


/**
 *	This methods handles the timers associated with this object.
 */
void OnceOffReceiver::handleTimeout( TimerHandle handle, void * arg )
{
	if (handle == onceOffPacketCleaningTimerHandle_)
	{
		this->onceOffReliableCleanup();
	}
	else if (handle == clearFragmentedBundlesTimerHandle_)
	{
		FragmentedBundles::iterator iter = fragmentedBundles_.begin();

		while (iter != fragmentedBundles_.end())
		{
			FragmentedBundles::iterator oldIter = iter++;
			const FragmentedBundle::Key & key = oldIter->first;
			FragmentedBundlePtr pFragments = oldIter->second;

			if (pFragments->isOld())
			{
				WARNING_MSG( "OnceOffReceiver::handleTimeout: "
					"Discarded stale fragmented bundle from %s "
					"(%.1fs old, %d packets)\n",
					key.addr_.c_str(),
					pFragments->age(),
					pFragments->chainLength() );

				fragmentedBundles_.erase( oldIter );
			}
		}
	}
}


// -----------------------------------------------------------------------------
// Section: OnceOffPacket
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
OnceOffPacket::OnceOffPacket( const Address & addr, SeqNum footerSequence,
				OnceOffSender & onceOffSender,
				Packet * pPacket ) :
	OnceOffReceipt( addr, footerSequence ),
	pPacket_( pPacket ),
	onceOffSender_( onceOffSender ),
	timerHandle_(),
	retries_( 0 )
{
}


/**
 *	This method adds a timer for this packet.
 */
void OnceOffPacket::registerTimer( EventDispatcher & dispatcher,
		PacketSender & packetSender, int period )
{
	timerHandle_ = dispatcher.addTimer( period /* microseconds */,
		this /* handler */,
		&packetSender /* arg */,
		"OnceOffPacket" );
}


/**
 *	This method handles a timer expiring.
 */
void OnceOffPacket::handleTimeout( TimerHandle handle, void * arg )
{
	PacketSender * pPacketSender = static_cast< PacketSender * >( arg );
	onceOffSender_.expireOnceOffResendTimer( *this, *pPacketSender );
}


// -----------------------------------------------------------------------------
// Section: OnceOffSender
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
OnceOffSender::OnceOffSender() :
	onceOffPackets_(),
	onceOffMaxResends_( DEFAULT_ONCEOFF_MAX_RESENDS ),
	onceOffResendPeriod_( DEFAULT_ONCEOFF_RESEND_PERIOD )
{
}


/**
 *	Destructor.
 */
OnceOffSender::~OnceOffSender()
{
	OnceOffPackets::iterator iter = onceOffPackets_.begin();

	while (iter != onceOffPackets_.end())
	{
		TimerHandle handle = iter->timerHandle_;
		handle.cancel();
		++iter;
	}
}


/**
 *	This method adds a resend timer for a once-off reliable packet.
 */
void OnceOffSender::addOnceOffResendTimer( const Address & addr,
						SeqNum seq,
						Packet * pPacket,
						PacketSender & packetSender,
						EventDispatcher & dispatcher )
{
//	TRACE_MSG( "addOnceOffResendTimer(%s,%u)\n", (char*)addr, seq );
	OnceOffPacket temp( addr, seq, *this, pPacket );

	// a bit tricky here because STL set will make a copy of temp, and only
	// give out const references to it through the iterator
	// It is the set's copy that we want to call registerTimer on.
	OnceOffPackets::iterator actualIter = onceOffPackets_.insert( temp ).first;
	OnceOffPacket * pOnceOffPacket =
		const_cast< OnceOffPacket * > (&(*actualIter) );

	pOnceOffPacket->registerTimer( dispatcher, packetSender,
			onceOffResendPeriod_ );

	// Note: while it would be better to avoid registering a timer for
	// every packet, this does have the advantage of spreading the resend
	// load nicely, as long as this facility is only used sparingly
	// (in which case the resend load is not that great anyway.. :-/ )
}


/**
 *	This method removes the resend timer associated with a packet sent once-off
 *	reliably. It should be called when receipt of the packet has been
 *	acknowledged.
 */
void OnceOffSender::delOnceOffResendTimer( const Address & addr, SeqNum seq )
{
	OnceOffPacket oop( addr, seq, *this );
	OnceOffPackets::iterator found = onceOffPackets_.find( oop );

	if (found != onceOffPackets_.end())
	{
		this->delOnceOffResendTimer( found );
	}
	else
	{
		DEBUG_MSG( "OnceOffSender::delOnceOffResendTimer( %s ): "
				"Called for #%u that we no longer have (usually ok)\n",
			addr.c_str(), seq );
	}
}


/**
 *	This method cancels the timer that is responsible for resending a packet.
 */
void OnceOffSender::delOnceOffResendTimer( OnceOffPackets::iterator & iter )
{
	TimerHandle handle = iter->timerHandle_;
	handle.cancel();
	onceOffPackets_.erase( iter );
}


/**
 *	This method resends a packet that was sent once-off reliably.
 */
void OnceOffSender::expireOnceOffResendTimer( OnceOffPacket & oop,
											PacketSender & packetSender )
{
	if (++oop.retries_ <= onceOffMaxResends_)
	{
		// If the packet has been dropped or delayed, skip it.
		if (packetSender.rescheduleSend( oop.addr_, oop.pPacket_.get(), NULL ))
		{
			return;
		}

		packetSender.sendPacket( oop.addr_, oop.pPacket_.get(), NULL, true );
	}
	else
	{
		OnceOffPackets::iterator iter = onceOffPackets_.find( oop );

		if (iter != onceOffPackets_.end())
		{
			DEBUG_MSG( "OnceOffReceiver::expOnceOffResendTimer( %s ): "
				"Discarding #%u after %d retries\n",
				oop.addr_.c_str(), oop.footerSequence_, onceOffMaxResends_ );

			this->delOnceOffResendTimer( iter );
		}
		else
		{
			CRITICAL_MSG( "OnceOffReceiver::expireOnceOffResendTimer( %s ): "
				"Called for #%u that we haven't got!\n",
				oop.addr_.c_str(), oop.footerSequence_ );
		}
	}
}


/**
 *	This method is called when a remote process dies. It allows for resend
 *	timers to be cleaned up.
 */
void OnceOffSender::onAddressDead( const Address & addr )
{
	// Iterate through all the unacked once-off sends and remove those going to
	// the dead address.
	OnceOffPackets::iterator iter = onceOffPackets_.begin();
	int numRemoved = 0;

	while (iter != onceOffPackets_.end())
	{
		if (iter->addr_ == addr)
		{
			OnceOffPackets::iterator deadIter = iter++;
			this->delOnceOffResendTimer( deadIter );
			numRemoved++;
		}
		else
		{
			++iter;
		}
	}

	if (numRemoved)
	{
		WARNING_MSG( "OnceOffSender::onAddressDead( %s ): "
			"Discarded %d unacked once-off sends\n",
			addr.c_str(), numRemoved );
	}
}

} // namespace Mercury

BW_END_NAMESPACE

// once_off_packet.cpp
