#ifndef CHANNEL_OWNER_HPP
#define CHANNEL_OWNER_HPP

#include "udp_channel.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	This class is a simple base class for classes that want to own a channel.
 */
class ChannelOwner
{
public:
	ChannelOwner( NetworkInterface & networkInterface,
			const Address & address = Address::NONE,
			UDPChannel::Traits traits = UDPChannel::INTERNAL ) :
		pChannel_( traits == UDPChannel::INTERNAL ?
			UDPChannel::get( networkInterface, address ) :
			new UDPChannel( networkInterface, address, traits ) )
	{
	}

	~ChannelOwner()
	{
		pChannel_->condemn();
		pChannel_ = NULL;
	}

	Bundle & bundle()				{ return pChannel_->bundle(); }
	const Address & addr() const	{ return pChannel_->addr(); }
	const char * c_str() const		{ return pChannel_->c_str(); }
	void send( Bundle * pBundle = NULL ) { pChannel_->send( pBundle ); }

	UDPChannel & channel()					{ return *pChannel_; }
	const UDPChannel & channel() const		{ return *pChannel_; }

	void addr( const Address & addr );

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif

private:
	UDPChannel * pChannel_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // CHANNEL_OWNER_HPP
