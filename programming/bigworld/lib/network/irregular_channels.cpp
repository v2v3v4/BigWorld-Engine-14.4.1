#include "pch.hpp"

#include "irregular_channels.hpp"
#include "udp_channel.hpp"

DECLARE_DEBUG_COMPONENT2( "Network", 0 )

BW_BEGIN_NAMESPACE

namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: IrregularChannels
// -----------------------------------------------------------------------------

/**
 *	This method remembers this channel for resend checking if it is irregular
 *	and is not already stored.
 */
void IrregularChannels::addIfNecessary( UDPChannel & channel )
{
	if (!channel.isLocalRegular() &&
		channel.hasUnackedPackets())
	{
		MonitoredChannels::addIfNecessary( channel );
	}
}

IrregularChannels::iterator & IrregularChannels::channelIter(
	UDPChannel & channel ) const
{
	return channel.irregularIter_;
}


float IrregularChannels::defaultPeriod() const
{
	return 1.f;
}


/**
 *	This method handles the timer event. It checks whether irregular channels
 *	need to resend unacked packets.
 */
void IrregularChannels::handleTimeout( TimerHandle, void * )
{
	iterator iter = channels_.begin();

	while (iter != channels_.end())
	{
		UDPChannel & channel = **iter++;

		if (channel.hasUnackedPackets() && !channel.isLocalRegular())
		{
			channel.send();

			// checkResendTimers() sets remote failure status
			if (channel.hasRemoteFailed())
			{
				ERROR_MSG( "IrregularChannels::handleTimeout: "
					"Removing dead channel to %s\n",
					channel.c_str() );

				this->delIfNecessary( channel );
			}
		}
		else
		{
			this->delIfNecessary( channel );
		}
	}
}

} // namespace Mercury

BW_END_NAMESPACE

// irregular_channels.cpp
