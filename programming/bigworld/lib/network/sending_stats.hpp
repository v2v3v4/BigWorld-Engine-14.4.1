#ifndef SENDING_STATS_HPP
#define SENDING_STATS_HPP

#include "cstdmf/profile.hpp"
#include "cstdmf/timer_handler.hpp"

#include "math/stat_with_rates_of_change.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class EventDispatcher;

/**
 *	This class maintains the statistics associated with network sending.
 */
class SendingStats : public TimerHandler
{
public:
	SendingStats();

	void init( EventDispatcher & dispatcher );
	void fini();

	double bitsPerSecond() const;
	double packetsPerSecond() const;
	double messagesPerSecond() const;

	uint numPacketsSent() const { return numPacketsSent_.total(); }
	uint numBundlesSent() const { return numBundlesSent_.total(); }
	uint numPacketsResent() const { return numPacketsResent_.total(); }
	uint numPiggybacks() const { return numPiggybacks_.total(); }

	uint numFailedBundleSend() const{ return numFailedBundleSend_.total(); }
	uint numFailedPacketSend() const { return numFailedPacketSend_.total(); }

#if ENABLE_WATCHERS
	ProfileVal & mercuryTimer()	{ return mercuryTimer_; }
	ProfileVal & systemTimer()	{ return systemTimer_; }

	static WatcherPtr pWatcher();
#endif // ENABLE_WATCHERS

private:
	virtual void handleTimeout( TimerHandle handle, void * arg );

	TimerHandle	timerHandle_;

	// TODO: Get rid of this friend
	friend class UDPBundle;
	friend class NetworkInterface;
	friend class PacketSender;

	typedef IntrusiveStatWithRatesOfChange< unsigned int > Stat;

	Stat::Container * pStats_;

	Stat numBytesSent_;
	Stat numBytesResent_;
	Stat numPacketsSent_;
	Stat numPacketsResent_;
	Stat numPiggybacks_;
	Stat numBytesPiggybacked_;
	Stat numBytesNotPiggybacked_;
	Stat numPacketsSentOffChannel_;
	Stat numBundlesSent_;
	Stat numMessagesSent_;
	Stat numReliableMessagesSent_;
	Stat numFailedPacketSend_;
	Stat numFailedBundleSend_;

#if ENABLE_WATCHERS
	ProfileVal	mercuryTimer_;
	ProfileVal	systemTimer_;
#endif // ENABLE_WATCHERS

};

}

BW_END_NAMESPACE

#endif // SENDING_STATS_HPP
