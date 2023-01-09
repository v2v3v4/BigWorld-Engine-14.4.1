#include "time_keeper.hpp"

#include "bwconfig.hpp"

#include "cstdmf/debug.hpp"

#include "network/smart_bundles.hpp"
#include "network/event_dispatcher.hpp"
#include "network/nub_exception.hpp"
#include "network/udp_bundle.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: TimeKeeper
// -----------------------------------------------------------------------------

/**
 *	Constructor for master time keeper.
 */
TimeKeeper::TimeKeeper( Mercury::NetworkInterface & interface,
		TimerHandle trackingTimerHandle,
		GameTime & gameTime,
		int idealTickFrequency ) :
	interface_( interface ),
	trackingTimerHandle_( trackingTimerHandle ),
	gameTime_( gameTime ),
	idealTickFrequency_( double(idealTickFrequency) ),
	nominalIntervalStamps_(
		this->dispatcher().timerIntervalTime( trackingTimerHandle ) ),
	syncCheckTimerHandle_(),
	isMaster_( true ),
	masterAddress_( Mercury::Address::NONE ),
	pMasterRequest_( NULL ),
	lastSyncRequestStamps_( 0 ),
	staggerOffset_( 0.0 )
{
}


/**
 *	Constructor for slave time keepers.
 */
TimeKeeper::TimeKeeper( Mercury::NetworkInterface & interface,
		TimerHandle trackingTimerHandle,
		GameTime & gameTime,
		int idealTickFrequency,
		const Mercury::Address & masterAddress,
		const Mercury::InterfaceElement * pMasterRequest,
		int staggerIndex, float maxStagger ) :
	interface_( interface ),
	trackingTimerHandle_( trackingTimerHandle ),
	gameTime_( gameTime ),
	idealTickFrequency_( double(idealTickFrequency) ),
	nominalIntervalStamps_(
		this->dispatcher().timerIntervalTime( trackingTimerHandle ) ),
	syncCheckTimerHandle_(),
	isMaster_( false ),
	masterAddress_( masterAddress ),
	pMasterRequest_( pMasterRequest ),
	lastSyncRequestStamps_( 0 ),
	staggerOffset_( 0.0 )
{
	if (staggerIndex)
	{
		const int DEFAULT_PRIME = 37;
		const int DEFAULT_MULTIPLIER = 5;
		int staggerPrime = BWConfig::get( "timeStagger/prime", DEFAULT_PRIME );
		int staggerMultiplier =
			BWConfig::get( "timeStagger/multiplier", DEFAULT_MULTIPLIER );

		if ((staggerPrime != DEFAULT_PRIME) ||
			(staggerMultiplier != DEFAULT_MULTIPLIER))
		{
			NOTICE_MSG( "Time stagger settings:\n"
							"\ttimeStagger/prime      = %d (default: %d)\n"
							"\ttimeStagger/multiplier = %d (default: %d)\n"
							"\tmaxTimeStagger         = %.2f\n",
						staggerPrime, DEFAULT_PRIME,
						staggerMultiplier, DEFAULT_MULTIPLIER,
						maxStagger );
		}

		int index = (staggerIndex * staggerMultiplier) % staggerPrime;
		double staggerFraction = double( index )/double( staggerPrime ) - 0.5;
		staggerOffset_ = staggerFraction/idealTickFrequency * maxStagger;
		INFO_MSG( "Staggering time by %.3f seconds\n",
				staggerOffset_ );
	}
}


/**
 *	Destructor
 */
TimeKeeper::~TimeKeeper()
{
	syncCheckTimerHandle_.cancel();
}


/**
 *	Input a game time reading from a master TimeKeeper.
 *	Do not call from the timer expiry callback.
 *	If the reading was unusable or way out then false is returned.
 */
bool TimeKeeper::inputMasterReading( double reading )
{
	reading += staggerOffset_;

	uint64 now = timestamp();
	int64 roundTripStamps = now - lastSyncRequestStamps_;
	double roundTripTime = double( roundTripStamps ) / stampsPerSecondD();
	double readingNowValue = readingNow();

	// assume that half the round trip time ago, the master read from its
	// reading, and take the offset from that
	double offset = (reading + roundTripTime / 2) - readingNowValue;
	int64 offsetStamps = int64( offset * stampsPerSecond() );

	uint64 & intervalStamps =
		this->dispatcher().timerIntervalTime( trackingTimerHandle_ );
	uint64 increment = nominalIntervalStamps_ / 20;
/*
	TRACE_MSG( "TimeKeeper::inputMasterReading: \n"
			"reading = %.9f readingNowValue = %.9f\n "
			"offset = %.6fus, rtt=%.3fus\n",
		reading, readingNowValue,
		1000000 * offsetStamps / stampsPerSecondD(),
		1000000 * roundTripStamps / stampsPerSecondD() );
*/

	lastSyncRequestStamps_ = 0;

	int64 threshold = int64( nominalIntervalStamps_ / 20 +
		roundTripStamps / 2 );
	if (offsetStamps >  threshold )
	{
		// we are slow
		if (intervalStamps == nominalIntervalStamps_)
		{
			DEBUG_MSG( "TimeKeeper::inputMasterReading: "
				"shortening tick interval because we are slow "
				"by %.0fms\n",
				1000 * offsetStamps / stampsPerSecondD()
			);
			intervalStamps = nominalIntervalStamps_ - increment;
			this->scheduleSyncCheck();
		}
		else if (intervalStamps > nominalIntervalStamps_)
		{
			// we are now slow, and our interval was longer than normal
			DEBUG_MSG( "TimeKeeper:: we have slowed down enough, "
				"reverting to nominal tick interval\n" );
			intervalStamps = nominalIntervalStamps_;
		}
		else // if (intervalStamps < nominalIntervalStamps_)
		{
			// we are still slow, and our interval was shorter than normal
			// so we keep at it, and check after the next tick
			this->scheduleSyncCheck();
		}
	}
	else if (offsetStamps < -threshold)
	{
		// we are fast
		if (intervalStamps == nominalIntervalStamps_)
		{
			DEBUG_MSG( "TimeKeeper::inputMasterReading: "
				"lengthening tick interval because we are fast "
				"by %.0fms\n",
				-1000 * offsetStamps / stampsPerSecondD()
			);
			intervalStamps = nominalIntervalStamps_ + increment;
			this->scheduleSyncCheck();
		}
		else if (intervalStamps < nominalIntervalStamps_)
		{
			// we are now fast, and our interval was shorter than normal
			DEBUG_MSG( "TimeKeeper:: we sped up enough, "
					"reverting back to nominal tick interval\n" );
			intervalStamps = nominalIntervalStamps_;
		}
		else // if (intervalStamps > nominalIntervalStamps_)
		{
			// we are still fast, and our interval was longer than normal
			// so we keep at it, and check after the next tick
			this->scheduleSyncCheck();
		}
	}
	else
	{
		// we are within tolerance
		if (intervalStamps != nominalIntervalStamps_)
		{
			DEBUG_MSG( "TimeKeeper::inputMasterReading: "
				"reverting back to nominal tick interval\n" );
			intervalStamps = nominalIntervalStamps_;
		}
	}

	return true;
}

/**
 *	Schedules a synchronization check; a timer is set which calls
 *	synchronizeWithMaster().
 */
void TimeKeeper::scheduleSyncCheck()
{
	if (syncCheckTimerHandle_.isSet())
	{
		TRACE_MSG( "TimeKeeper::scheduleSyncCheck: existing timer ID\n" );
		return;
	}

	double deliveryIn = double(int64(this->dispatcher().timerDeliveryTime(
		trackingTimerHandle_ ) - timestamp()));
	double s = std::max( deliveryIn / stampsPerSecondD(), 0.001 );
	int callbackPeriod = int( s * 1000000.0) + 1000;
	syncCheckTimerHandle_ =
		this->dispatcher().addOnceOffTimer( callbackPeriod, this );
}


/**
 *	Return the reading of the time we are keeping at the last tick
 */
double TimeKeeper::readingAtLastTick() const
{
	return double(gameTime_) / idealTickFrequency_;
}

/**
 *	Return a reading of the time we are keeping right now
 */
double TimeKeeper::readingNow() const
{
	double ticksAtNext = double(gameTime_+1);
	double intervalStamps =
		double( this->dispatcher().timerIntervalTime( trackingTimerHandle_ ) );

	uint64 stampsAtNext =
		this->dispatcher().timerDeliveryTime( trackingTimerHandle_ );
	int64 stampsSinceNext = int64(timestamp() - stampsAtNext);
	// stampsSinceNext will hopefully be negative :)
	double ticksSinceNext = double(stampsSinceNext) / intervalStamps;

	return (ticksAtNext + ticksSinceNext) / idealTickFrequency_;
}

/**
 *	Return the reading of the time we are keeping at the next tick
 */
double TimeKeeper::readingAtNextTick() const
{
	return double(gameTime_+1) / idealTickFrequency_;
}


/**
 *	This method synchronises the time maintained by this time keeper
 *	with that maintained by the given peer. A reply is not expected.
 *	The message is not sent reliably.
 */
void TimeKeeper::synchroniseWithPeer( const Mercury::Address & address,
	const Mercury::InterfaceElement & request )
{
	Mercury::UDPBundle bundle;
	bundle.startMessage( request );
	bundle << this->readingNow();
	interface_.send( address, bundle );
}


/**
 *	This method initiates a query with the master time keeper provided
 *	on constructor, in order to synchronise with its clock
 */
void TimeKeeper::synchroniseWithMaster()
{
	MF_ASSERT( pMasterRequest_ != NULL );

	if (masterAddress_ == Mercury::Address::NONE)
	{
		WARNING_MSG( "TimeKeeper::synchroniseWithMaster: Skipping because "
				"master is not ready\n" );
		return;
	}

	if (lastSyncRequestStamps_)
	{
		WARNING_MSG( "TimeKeeper::synchroniseWithMaster: in progress\n" );
		return;
	}

	{
		Mercury::AutoSendBundle autoBundle( interface_, masterAddress_ );
		Mercury::Bundle & b = *autoBundle;

		b.startRequest( *pMasterRequest_, this );
		b << this->readingNow();
	}

	lastSyncRequestStamps_ = timestamp();
}


/**
 *	This private method finds the offset of the given reading which was
 *	made at the given number of stamps, from what our internal number
 *	of stamps would have been when we would have given that reading.
 *	The result is positive iff our stamps would be (or were) less than the
 *	given stamps, i.e. iff our clock is running faster than the given one.
 */
int64 TimeKeeper::offsetOfReading( double reading, uint64 stampsAtReceiptExt )
{
	// figure out what we think the corresponding timestamp
	// for that game time should be if we were there internally
	double readingAtNextTick = this->readingAtNextTick();

	Mercury::EventDispatcher & dispatcher = this->dispatcher();

	uint64 intervalTime = dispatcher.timerIntervalTime( trackingTimerHandle_ );
	uint64 deliveryTime = dispatcher.timerDeliveryTime( trackingTimerHandle_ );

	double stampsPerReadingUnit = double(intervalTime) * idealTickFrequency_;
	// above is like stamps per second, but scaled by adjusted intervalTime

	// At 'deliveryTime', we will be giving the reading 'readingAtNextTick'.
	// So now just subtract the difference in readings converted into
	// stamps, from that known deliveryTime (or add if we are very slow),
	// and we will know the stamps that we would have given 'reading' at.

	uint64 stampsAtReceiptInt = deliveryTime -
		uint64(int64((readingAtNextTick-reading) * stampsPerReadingUnit));
	return int64(stampsAtReceiptExt - stampsAtReceiptInt);
}


/**
 *	This is called when it is time to reset the interval of the tracking
 *	timer back to its nominal level.
 */
void TimeKeeper::handleTimeout( TimerHandle handle, void * arg )
{
	if (handle == syncCheckTimerHandle_)
	{
		syncCheckTimerHandle_.cancel();

		if (isMaster_)
		{
			// we're the master
			uint64 & intervalStamps = this->dispatcher().timerIntervalTime(
				trackingTimerHandle_ );
			intervalStamps = nominalIntervalStamps_;
		}
		else
		{
			this->synchroniseWithMaster();
		}
	}
}


/**
 *	This is called when someone has selected us to be a ReplyMessageHandler.
 *	We assume that it was for a game time sync message and that the
 *	reply stream has one 'double' on it.
 */
void TimeKeeper::handleMessage( const Mercury::Address & source,
	Mercury::UnpackedMessageHeader & header, BinaryIStream & data, void * )
{
	if (header.length != sizeof(double))
	{
		ERROR_MSG( "TimeKeeper::handleMessage: "
			"Reply from %s expected to be just one 'double' but length is %d\n",
			source.c_str(), header.length );
		return;
	}

	double reading;
	data >> reading;

	// we assume that this is a master reading
	if (!this->inputMasterReading( reading ))
	{
		// if it didn't like it then try again later
		if (!isMaster_)
		{
			//DEBUG_MSG( "TimeKeeper::handleMessage: "
			//	"Master reading unusable or way out, will try again\n" );

			this->scheduleSyncCheck();
		}
	}
}

/**
 *	This is called when someone has selected us to be a ReplyMessageHandler.
 *	We don't do anything here except print out a warning.
 */
void TimeKeeper::handleException( const Mercury::NubException & exception,
		void * )
{
	if (exception.reason() == Mercury::REASON_TIMER_EXPIRED)
	{
		WARNING_MSG( "TimeKeeper::handleException: "
			"Reply to game time sync request timed out\n" );
	}
	else
	{
		ERROR_MSG( "TimeKeeper::handleException: %s\n",
				Mercury::reasonToString( exception.reason() ) );
	}

	syncCheckTimerHandle_.cancel();
	lastSyncRequestStamps_ = 0;
}

BW_END_NAMESPACE

// time_keeper.cpp
