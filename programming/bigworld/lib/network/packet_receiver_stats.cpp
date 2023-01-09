#include "pch.hpp"

#include "cstdmf/timestamp.hpp"

#include "packet_receiver_stats.hpp"

#ifndef CODE_INLINE

BW_BEGIN_NAMESPACE
#include "packet_receiver_stats.ipp"
BW_END_NAMESPACE

#endif

#include "endpoint.hpp"

#include "math/stat_watcher_creator.hpp"

DECLARE_DEBUG_COMPONENT2( "Network", 0 );


BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	Constructor.
 */
PacketReceiverStats::PacketReceiverStats() :
#if ENABLE_WATCHERS	
	mercuryTimer_(),
	systemTimer_(),
#endif // ENABLE_WATCHERS
	pStats_( NULL ),
	numBytesReceived_(             pStats_ ),
	numPacketsReceived_(           pStats_ ),
	numDuplicatePacketsReceived_(  pStats_ ),
	numPacketsReceivedOffChannel_( pStats_ ),
	numBundlesReceived_(           pStats_ ),
	numMessagesReceived_(          pStats_ ),
	numOverheadBytesReceived_(     pStats_ ),
	numCorruptedPacketsReceived_(  pStats_ ),
	numCorruptedBundlesReceived_(  pStats_ ),
	lastGatherTime_( 0 ),
	lastTxQueueSize_( 0 ),
	lastRxQueueSize_( 0 ),
	maxTxQueueSize_( 0 ),
	maxRxQueueSize_( 0 )
{
#if ENABLE_WATCHERS
	StatWatcherCreator::initRatesOfChangeForStats( *pStats_ );
#endif // ENABLE_WATCHERS
}


/**
 *	This method updates the statics associated with the PacketReceiver's socket.
 */
void PacketReceiverStats::updateSocketStats( const Endpoint & socket )
{
	uint64 currTime = timestamp();

	// Wait at least a second.
	if (currTime < lastGatherTime_ + stampsPerSecond())
	{
		return;
	}

	// Update stat averages
	{
		double elapsedTime = (currTime - lastGatherTime_)/stampsPerSecondD();
		this->updateStatAverages( elapsedTime );
	}

	lastGatherTime_ = currTime;

	socket.getQueueSizes( lastTxQueueSize_, lastRxQueueSize_ );

	// Warn if the buffers are getting fuller
	if ((lastTxQueueSize_ > maxTxQueueSize_) && (lastTxQueueSize_ > 128*1024))
	{
		WARNING_MSG( "Transmit queue peaked at new max (%d bytes)\n",
			lastTxQueueSize_ );
	}

	if ((lastRxQueueSize_ > maxRxQueueSize_) && (lastRxQueueSize_ > 1024*1024))
	{
		WARNING_MSG( "Receive queue peaked at new max (%d bytes)\n",
			lastRxQueueSize_ );
	}

	maxTxQueueSize_ = std::max( lastTxQueueSize_, maxTxQueueSize_ );
	maxRxQueueSize_ = std::max( lastRxQueueSize_, maxRxQueueSize_ );
}


/**
 *	This method updates the moving averages of the collected stats.
 */
void PacketReceiverStats::updateStatAverages( double elapsedTime )
{
	if (pStats_)
	{
		Stat::Container::iterator iter = pStats_->begin();

		while (iter != pStats_->end())
		{
			(*iter)->tick( elapsedTime );

			++iter;
		}
	}
}


/**
 *	This method returns the current bits per second received.
 */
double PacketReceiverStats::bitsPerSecond() const
{
	return numBytesReceived_.getRateOfChange() * 8;
}


/**
 *	This method returns the current packets per second received.
 */
double PacketReceiverStats::packetsPerSecond() const
{
	return numPacketsReceived_.getRateOfChange();
}


/**
 *	This method returns the current messages per second received.
 */
double PacketReceiverStats::messagesPerSecond() const
{
	return numMessagesReceived_.getRateOfChange();
}


#if ENABLE_WATCHERS
/**
 *	This method returns a Watcher that can be used to inspect
 *	PacketReceiverStats instances.
 */
WatcherPtr PacketReceiverStats::pWatcher()
{
	WatcherPtr pWatcher = new DirectoryWatcher();
	PacketReceiverStats * pNull = NULL;

	StatWatcherCreator::addWatchers( pWatcher, "bytesReceived", pNull->numBytesReceived_ );
	StatWatcherCreator::addWatchers( pWatcher, "packetsReceived", pNull->numPacketsReceived_ );
	StatWatcherCreator::addWatchers( pWatcher, "duplicatePacketsReceived", pNull->numDuplicatePacketsReceived_ );
	StatWatcherCreator::addWatchers( pWatcher, "packetsReceivedOffChannel", pNull->numPacketsReceivedOffChannel_ );
	StatWatcherCreator::addWatchers( pWatcher, "bundlesReceived", pNull->numBundlesReceived_ );
	StatWatcherCreator::addWatchers( pWatcher, "messagesReceived", pNull->numMessagesReceived_ );
	StatWatcherCreator::addWatchers( pWatcher, "overheadBytesReceived", pNull->numOverheadBytesReceived_ );
	StatWatcherCreator::addWatchers( pWatcher, "corruptedPacketsReceived", pNull->numCorruptedPacketsReceived_ );
	StatWatcherCreator::addWatchers( pWatcher, "corruptedBundlesReceived", pNull->numCorruptedBundlesReceived_ );

#ifdef __unix__
	pWatcher->addChild( "socket/transmitQueue",
		makeWatcher( pNull->lastTxQueueSize_ ) );

	pWatcher->addChild( "socket/receiveQueue",
		makeWatcher( pNull->lastRxQueueSize_ ) );

	pWatcher->addChild( "socket/maxTransmitQueue",
		makeWatcher( pNull->maxTxQueueSize_ ) );

	pWatcher->addChild( "socket/maxReceiveQueue",
		makeWatcher( pNull->maxRxQueueSize_ ) );
#endif // __unix__

	pWatcher->addChild( "timing/mercury", ProfileVal::pWatcherStamps(),
			&pNull->mercuryTimer_ );
	pWatcher->addChild( "timing/system", ProfileVal::pWatcherStamps(),
			&pNull->systemTimer_ );

	pWatcher->addChild( "timingInSeconds/mercury", ProfileVal::pWatcherSeconds(),
			&pNull->mercuryTimer_ );
	pWatcher->addChild( "timingInSeconds/system", ProfileVal::pWatcherSeconds(),
			&pNull->systemTimer_ );

	pWatcher->addChild( "timingSummary/mercury", ProfileVal::pSummaryWatcher(),
			&pNull->mercuryTimer_ );
	pWatcher->addChild( "timingSummary/system", ProfileVal::pSummaryWatcher(),
			&pNull->systemTimer_ );

	return pWatcher;
}
#endif // ENABLE_WATCHERS

} // namespace Mercury

BW_END_NAMESPACE

// packet_receiver_stats.cpp
