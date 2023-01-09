#include "pch.hpp"

#include "keepalive_channels.hpp"

#include "udp_bundle.hpp"
#include "udp_channel.hpp"

BW_BEGIN_NAMESPACE

// This value is the period at which handleTimeout() will be called, which
// is actually half the interval at which we want to send keepalives (see
// the initialisation of 'pingInactivityPeriod' below).
static const float KEEP_ALIVE_PING_PERIOD = 2.5f; 		// seconds

namespace Mercury
{

// This is the length of time where if the channel is not used, we will 
// destroy it.
float KeepAliveChannels::s_timeoutPeriod = 60.f; 	// seconds

/**
 *	Constructor.
 */
KeepAliveChannels::KeepAliveChannels() :
	lastTimeout_( 0 ),
	lastInvalidTimeout_( 0 )
{
}


/**
 *	This method remembers this channel for resend checking if it is irregular
 *	and is not already stored.
 */
void KeepAliveChannels::addIfNecessary( UDPChannel & channel )
{
	// At the moment, the only channels that should be getting automatic
	// keepalive checking are anonymous channels.
	if (channel.isAnonymous())
	{
		MonitoredChannels::addIfNecessary( channel );
	}
}


KeepAliveChannels::iterator & KeepAliveChannels::channelIter(
	UDPChannel & channel ) const
{
	return channel.keepAliveIter_;
}


/**
 *  This method returns the interval for timeouts on this object.
 */
float KeepAliveChannels::defaultPeriod() const
{
	return KEEP_ALIVE_PING_PERIOD;
}


/**
 *  This method checks for dead channels and sends keepalives as necessary.
 */
void KeepAliveChannels::handleTimeout( TimerHandle, void * )
{
	const uint64 pingInactivityPeriod =
		uint64( 2 * period_ * stampsPerSecond() );
	const uint64 now = timestamp();
	const uint64 timeoutPeriod =
		uint64( s_timeoutPeriod * stampsPerSecond() );
	bool shouldDoTimeoutTest = true;

	// If handleTimeout has not been called for a while, do not time out other
	// channels because it is probably our fault.
	if ((lastTimeout_ != 0) && 2 * (now - lastTimeout_) > timeoutPeriod)
	{
		WARNING_MSG( "KeepAliveChannels::handleTimeout: Not called for %.2f "
						"seconds. Temporarily suspending timeouts.\n",
					(now - lastTimeout_) / stampsPerSecondD() );
		lastInvalidTimeout_ = now;
	}

	if (lastInvalidTimeout_ != 0)
	{
		if (now - lastInvalidTimeout_ < timeoutPeriod)
		{
			// Do not kill any if we paused recently
			shouldDoTimeoutTest = false;
		}
		else
		{
			INFO_MSG( "KeepAliveChannels::handleTimeout: "
					"Timeout now restored.\n" );
			lastInvalidTimeout_ = 0;
		}
	}

	if (UDPChannel::allowInteractiveDebugging())
	{
		shouldDoTimeoutTest = false;
	}

	lastTimeout_ = now;

	iterator iter = channels_.begin();

	while (iter != channels_.end())
	{
		UDPChannel & channel = **iter++;

		if (shouldDoTimeoutTest &&
				(now - channel.lastReceivedTime() > timeoutPeriod))
		{
			ERROR_MSG( "KeepAliveChannels::check: "
				"Channel to %s has timed out (%.3fs)\n",
				channel.c_str(),
				(now - channel.lastReceivedTime()) / stampsPerSecondD() );

			this->delIfNecessary( channel );

			// Set hasRemoteFailed to true for dead channels.
			channel.setRemoteFailed();
		}

		else if (now - channel.lastReceivedTime() > pingInactivityPeriod)
		{
			UDPBundle emptyBundle;

			// Send pings to channels that have been inactive for too long.
			emptyBundle.reliable( RELIABLE_DRIVER );
			channel.send( &emptyBundle );
		}
	}
}

} // namespace Mercury

BW_END_NAMESPACE

// keepalive_channels.cpp
