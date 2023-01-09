#include "pch.hpp"

#include "sending_stats.hpp"

#include "event_dispatcher.hpp"

#include "math/stat_watcher_creator.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

namespace
{
const float SAMPLE_RATE = 1.f;
}

/**
 *	Constructor.
 */
SendingStats::SendingStats() :
	timerHandle_(),
	pStats_( NULL ),
	numBytesSent_( pStats_ ),
	numBytesResent_( pStats_ ),
	numPacketsSent_( pStats_ ),
	numPacketsResent_( pStats_ ),
	numPiggybacks_( pStats_ ),
	numBytesPiggybacked_( pStats_ ),
	numBytesNotPiggybacked_( pStats_ ),
	numPacketsSentOffChannel_( pStats_ ),
	numBundlesSent_( pStats_ ),
	numMessagesSent_( pStats_ ),
	numReliableMessagesSent_( pStats_ ),
	numFailedPacketSend_( pStats_ ),
	numFailedBundleSend_( pStats_ )
#if ENABLE_WATCHERS
	, mercuryTimer_(),
	systemTimer_()
#endif // ENABLE_WATCHERS
{
#if ENABLE_WATCHERS
	StatWatcherCreator::initRatesOfChangeForStats( *pStats_ );
#endif // ENABLE_WATCHERS
}


/**
 *	This method initialises the timer associated with this instance.
 */
void SendingStats::init( EventDispatcher & dispatcher )
{
	MF_ASSERT( !timerHandle_.isSet() );

	timerHandle_ = dispatcher.addTimer( int( 1000000 * SAMPLE_RATE ), this,
		NULL, "SendingStats" );
}


/**
 *	This method cancels the timer associated with this instance.
 */
void SendingStats::fini()
{
	timerHandle_.cancel();
}


/**
 *	This method returns the number of bits per second sent.
 */
double SendingStats::bitsPerSecond() const
{
	return numBytesSent_.getRateOfChange() * 8;
}


/**
 *	This method returns the number of packets per second sent.
 */
double SendingStats::packetsPerSecond() const
{
	return numPacketsSent_.getRateOfChange();
}


/**
 *	This method returns the number of messages per second sent.
 */
double SendingStats::messagesPerSecond() const
{
	return numMessagesSent_.getRateOfChange();
}


#if ENABLE_WATCHERS
WatcherPtr SendingStats::pWatcher()
{
	WatcherPtr pWatcher = new DirectoryWatcher();
	SendingStats * pNull = NULL;

	StatWatcherCreator::addWatchers( pWatcher, "bytesSent",             pNull->numBytesSent_ );
	StatWatcherCreator::addWatchers( pWatcher, "bytesResent",           pNull->numBytesResent_ );
	StatWatcherCreator::addWatchers( pWatcher, "reliableBytesPiggybacked",      pNull->numBytesPiggybacked_ );
	StatWatcherCreator::addWatchers( pWatcher, "unreliableBytesDiscarded",   pNull->numBytesNotPiggybacked_ );
	StatWatcherCreator::addWatchers( pWatcher, "packetsSent",           pNull->numPacketsSent_ );
	StatWatcherCreator::addWatchers( pWatcher, "packetsResent",         pNull->numPacketsResent_ );
	StatWatcherCreator::addWatchers( pWatcher, "packetsPiggybacked",    pNull->numPiggybacks_ );
	StatWatcherCreator::addWatchers( pWatcher, "packetsSentOffChannel", pNull->numPacketsSentOffChannel_ );
	StatWatcherCreator::addWatchers( pWatcher, "bundlesSent",           pNull->numBundlesSent_ );
	StatWatcherCreator::addWatchers( pWatcher, "messagesSent",          pNull->numMessagesSent_ );
	StatWatcherCreator::addWatchers( pWatcher, "reliableMessagesSent",  pNull->numReliableMessagesSent_ );
	StatWatcherCreator::addWatchers( pWatcher, "failedPacketSends",     pNull->numFailedPacketSend_ );
	StatWatcherCreator::addWatchers( pWatcher, "failedBundleSends",     pNull->numFailedBundleSend_ );

	pWatcher->addChild( "timingInSeconds/mercury", ProfileVal::pWatcherSeconds(),
			&pNull->mercuryTimer_ );

	pWatcher->addChild( "timingInSeconds/system", ProfileVal::pWatcherSeconds(),
			&pNull->systemTimer_ );

	pWatcher->addChild( "timing/mercury", ProfileVal::pWatcherStamps(),
			&pNull->mercuryTimer_ );

	pWatcher->addChild( "timing/system", ProfileVal::pWatcherStamps(),
			&pNull->systemTimer_ );

	return pWatcher;
}
#endif // ENABLE_WATCHERS


/**
 *	This method is called once a second to update the moving averages.
 */
void SendingStats::handleTimeout( TimerHandle /* handle */, void * /* arg */ )
{
	if (pStats_)
	{
		Stat::Container::iterator iter = pStats_->begin();

		while (iter != pStats_->end())
		{
			(*iter)->tick( SAMPLE_RATE );

			++iter;
		}
	}
}

} // namespace Mercury

BW_END_NAMESPACE

// sending_stats.cpp
