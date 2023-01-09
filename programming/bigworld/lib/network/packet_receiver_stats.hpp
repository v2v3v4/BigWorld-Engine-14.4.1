#ifndef PACKET_RECEIVER_STATS_HPP
#define PACKET_RECEIVER_STATS_HPP

#include "cstdmf/profile.hpp"

#include "math/stat_with_rates_of_change.hpp"

BW_BEGIN_NAMESPACE

class Endpoint;

namespace Mercury
{

/**
 *	This class is used to collect statistics about a PacketReceiver.
 */
class PacketReceiverStats
{
public:
	PacketReceiverStats();

	void incCorruptedPackets();
	void incDuplicatePackets();

	unsigned int numPacketsReceived() const;
	unsigned int numBundlesReceived() const 
	{ return numBundlesReceived_.total(); }
	unsigned int numMessagesReceived() const;
	unsigned int numBytesReceived() const;
	unsigned int numOverheadBytesReceived() const;
	unsigned int numCorruptedPacketsReceived() const
	{ return numCorruptedPacketsReceived_.total(); }

	unsigned int numCorruptedBundlesReceived() const
	{ return numCorruptedBundlesReceived_.total(); }

	unsigned int numDuplicatePacketsReceived() const
	{ return numDuplicatePacketsReceived_.total(); }

	double bitsPerSecond() const;
	double packetsPerSecond() const;
	double messagesPerSecond() const;

	void updateSocketStats( const Endpoint & socket );

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif // ENABLE_WATCHERS

private:
	friend class PacketReceiver;
	friend class ProcessSocketStatsHelper;

	PacketReceiverStats( const PacketReceiverStats & );
	PacketReceiverStats & operator=( const PacketReceiverStats & );

	void updateStatAverages( double elapsedTime );

	typedef IntrusiveStatWithRatesOfChange< unsigned int > Stat;

#if ENABLE_WATCHERS
	ProfileVal	mercuryTimer_;
	ProfileVal	systemTimer_;
#endif // ENABLE_WATCHERS

	Stat::Container * pStats_;

	Stat numBytesReceived_;
	Stat numPacketsReceived_;
	Stat numDuplicatePacketsReceived_;
	Stat numPacketsReceivedOffChannel_;
	Stat numBundlesReceived_;
	Stat numMessagesReceived_;
	Stat numOverheadBytesReceived_;
	Stat numCorruptedPacketsReceived_;
	Stat numCorruptedBundlesReceived_;

	uint64	lastGatherTime_;
	int		lastTxQueueSize_;
	int		lastRxQueueSize_;
	int		maxTxQueueSize_;
	int		maxRxQueueSize_;
};

} // namespace Mercury

#ifdef CODE_INLINE
#include "packet_receiver_stats.ipp"
#endif

BW_END_NAMESPACE

#endif // PACKET_RECEIVER_STATS_HPP
