#ifndef CHANNEL_SENDER_HPP
#define CHANNEL_SENDER_HPP

#include "cstdmf/bw_namespace.hpp"

#include "udp_channel.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *  This class is a wrapper class for a Channel that automatically sends on
 *  destruct if the channel is irregular.  Recommended for use in app code when
 *  you don't want to have to keep figuring out if channels you get with
 *  findChannel() are regular or not.
 */
class ChannelSender
{
public:
	ChannelSender( UDPChannel & channel ) :
		channel_( channel )
	{}

	~ChannelSender()
	{
		if (!channel_.isLocalRegular())
		{
			channel_.delayedSend();
		}
	}

	Bundle & bundle() { return channel_.bundle(); }
	UDPChannel & channel() { return channel_; }

private:
	UDPChannel & channel_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // CHANNEL_SENDER_HPP
