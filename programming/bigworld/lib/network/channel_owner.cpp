#include "pch.hpp"

#include "channel_owner.hpp"

#include "cstdmf/watcher.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: ChannelOwner
// -----------------------------------------------------------------------------

/**
 *  This method switches this ChannelOwner to a different address.  We can't
 *  simply call through to Channel::address() because there might already be an
 *  anonymous channel to that address.  We need to look it up and claim the
 *  anonymous one if it already exists.
 */
void ChannelOwner::addr( const Address & address )
{
	MF_ASSERT( pChannel_ );

	// Don't do anything if it's already on the right address
	if (this->addr() == address)
	{
		return;
	}

	// Get a new channel to the right address.
	UDPChannel * pNewChannel =
		UDPChannel::get( pChannel_->networkInterface(), address );

	// Configure the new channel like the old one, and then get rid of it.
	pNewChannel->configureFrom( *pChannel_ );

	// Put the new channel in its place.
	UDPChannel * pOldChannel = pChannel_;
	pChannel_ = pNewChannel;

	// Condemning the old channel is done after the new channel is in place
	// to allow any reliance on channel existance in handleException to work.
	pOldChannel->condemn();
}


#if ENABLE_WATCHERS
/**
 *	This static method returns the watcher associated with this class.
 */
WatcherPtr ChannelOwner::pWatcher()
{
	static WatcherPtr pWatcher =
		new PolymorphicWatcher( "ChannelOwner" );

	return pWatcher;
}
#endif // ENABLE_WATCHERS

} // namespace Mercury

BW_END_NAMESPACE

// channel_owner.cpp
