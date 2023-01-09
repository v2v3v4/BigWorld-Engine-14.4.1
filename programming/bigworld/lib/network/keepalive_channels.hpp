#ifndef KEEPALIVE_CHANNELS_HPP
#define KEEPALIVE_CHANNELS_HPP

#include "monitored_channels.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	This class is used to keep track of channels that send and receive so
 *	infrequently that we need to send keepalive traffic to ensure the peer apps
 *	haven't died.
 */
class KeepAliveChannels : public MonitoredChannels
{
public:
	KeepAliveChannels();
	void addIfNecessary( UDPChannel & channel );

	static float timeoutPeriod()		{ return s_timeoutPeriod; }
	static void setTimeoutPeriod( float timeoutPeriod )
	{
		s_timeoutPeriod = timeoutPeriod;
	}

protected:
	iterator & channelIter( UDPChannel & channel ) const;
	float defaultPeriod() const;

private:
	virtual void handleTimeout( TimerHandle, void * );

	uint64 lastTimeout_;
	uint64 lastInvalidTimeout_;

	static float s_timeoutPeriod;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // KEEPALIVE_CHANNELS_HPP
