#include "pch.hpp"

#include "bundle_sending_map.hpp"
#include "udp_channel.hpp"

DECLARE_DEBUG_COMPONENT2( "Network", 0 )

// -----------------------------------------------------------------------------
// Section: BundleSendingMap
// -----------------------------------------------------------------------------

#include "network_interface.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *  This method returns the bundle for the given address, mapping the
 *  channel in if necessary.
 */
Bundle & BundleSendingMap::operator[]( const Address & addr )
{
	Channels::iterator iter = channels_.find( addr );

	if (iter != channels_.end())
	{
		return iter->second->bundle();
	}
	else
	{
		UDPChannel * pChannel = networkInterface_.findChannel( addr, true );
		channels_[ addr ] = pChannel;
		return pChannel->bundle();
	}
}


/**
 *  This method sends all the pending bundles on channels in this map.
 */
void BundleSendingMap::sendAll()
{
	for (Channels::iterator iter = channels_.begin();
		 iter != channels_.end(); ++iter)
	{
		iter->second->send();
	}

	channels_.clear();
}

} // namespace Mercury

BW_END_NAMESPACE

// bundle_sending_map.cpp
