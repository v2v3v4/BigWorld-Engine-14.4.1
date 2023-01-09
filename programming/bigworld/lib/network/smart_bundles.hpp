#ifndef SMART_BUNDLES_HPP
#define SMART_BUNDLES_HPP

#include "channel_sender.hpp"
#include "bundle.hpp"
#include "udp_bundle.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	This class helps sending a bundle that is on a channel.
 */
class OnChannelBundle
{
public:
	OnChannelBundle( NetworkInterface & networkInterface,
			const Address & dstAddr ) :
		sender_( networkInterface.findOrCreateChannel( dstAddr ) )
	{
	}

	Bundle & operator*()	{ return sender_.channel().bundle(); }
	Bundle * operator->()	{ return &sender_.channel().bundle(); }
	operator Bundle &()		{ return sender_.channel().bundle(); }

private:
	ChannelSender sender_;
};


/**
 *
 */
class AutoSendBundle
{
public:
	AutoSendBundle( NetworkInterface & networkInterface,
			const Address & dstAddr ) :
		pChannel_( networkInterface.findChannel( dstAddr ) ),
		bundle_(),
		interface_( networkInterface ),
		addr_( dstAddr )
	{
	}

	~AutoSendBundle()
	{
		if (pChannel_)
		{
			pChannel_->send();
		}
		else
		{
			interface_.send( addr_, bundle_ );
		}
	}

	UDPBundle & getBundle()
	{
		return pChannel_ ? 
			static_cast< Mercury::UDPBundle & >( pChannel_->bundle() ) :
			bundle_;
	}

	UDPBundle & operator*()		{ return this->getBundle(); }
	UDPBundle * operator->()	{ return &this->getBundle(); }
	operator Bundle &()			{ return this->getBundle(); }

private:
	UDPChannel * 		pChannel_;
	UDPBundle 			bundle_;
	NetworkInterface & 	interface_;
	const Address 		addr_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // SMART_BUNDLES_HPP
