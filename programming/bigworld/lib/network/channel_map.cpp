#include "pch.hpp"

#include "channel_map.hpp"

#include "udp_channel.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	This method destroys all channels owned by the NetworkInterface.
 */
void ChannelMap::destroyOwnedChannels()
{
	// Delete any channels this owns.
	Map::iterator iter = map_.begin();

	while (iter != map_.end())
	{
		UDPChannel * pChannel = iter->second;
		++iter;

		if (pChannel->isOwnedByInterface())
		{
			pChannel->destroy();
		}
		else
		{
			WARNING_MSG( "ChannelMap::destroyOwnedChannels: "
					"Channel to %s is still registered\n",
				pChannel->c_str() );
		}
	}
}


/**
 *	This method adds a channel to this map.
 */
bool ChannelMap::add( UDPChannel & channel )
{
	MF_ASSERT( channel.addr() != Address::NONE );

	std::pair< Map::iterator, bool > result = 
		map_.insert( Map::value_type( channel.addr(), &channel ) );

	// Should not register a channel twice.
	IF_NOT_MF_ASSERT_DEV( result.second )
	{
		return false;
	}

	return true;
}


/**
 *	This method removes a channel from this map.
 */
bool ChannelMap::remove( UDPChannel & channel )
{
	const Address & address = channel.addr();
	MF_ASSERT( address != Address::NONE );

	bool wasRemoved = (map_.erase( address ) != 0);

	if (!wasRemoved)
	{
		CRITICAL_MSG( "ChannelMap::remove: Channel not found %s!\n",
			address.c_str() );
	}

	return wasRemoved;
}


/**
 *	This method returns a channel associated with a given address. If a
 *	NetworkInterface is passed and the channel does not yet exist, it is
 *	created.
 */
UDPChannel * ChannelMap::find( const Address & address,
		NetworkInterface * pInterface ) const
{
	const bool createAnonymous = (pInterface != NULL);

	if (address.ip == 0)
	{
		MF_ASSERT( !createAnonymous );

		return NULL;
	}

	Map::const_iterator iter = map_.find( address );
	UDPChannel * pChannel = iter != map_.end() ? iter->second : NULL;

	// Indexed channels aren't supposed to be registered with the nub.
	MF_ASSERT( !pChannel || pChannel->id() == CHANNEL_ID_NULL );

	// Make a new anonymous channel if it didn't already exist and this is an
	// internal nub.
	if (!pChannel && pInterface)
	{
		INFO_MSG( "ChannelMap::find: Creating anonymous channel to %s\n",
			address.c_str() );

		pChannel = new UDPChannel( *pInterface, address, UDPChannel::INTERNAL );
		pChannel->isAnonymous( true );
	}

	return pChannel;
}


/**
 *	This method deletes an anonymous channel.
 */
void ChannelMap::delAnonymous( const Address & address )
{
	Map::iterator iter = map_.find( address );

	if (iter == map_.end())
	{
		ERROR_MSG( "ChannelMap::delAnonymous: "
			"Couldn't find channel for address %s\n",
			address.c_str() );
		return;
	}

	if (!iter->second->isAnonymous())
	{
		ERROR_MSG( "ChannelMap::delAnonymous: "
				"UDPChannel to %s is not anonymous!\n",
			address.c_str() );
		return;
	}

	iter->second->condemn();
}


/**
 *	This method returns whether any channel in this map has unacked packets.
 */
bool ChannelMap::hasUnackedPackets() const
{
	// Go through registered channels and check if any have unacked packets
	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		const UDPChannel & channel = *iter->second;

		if (channel.hasUnackedPackets())
		{
			return true;
		}

		++iter;
	}

	return false;
}

} // namespace Mercury

BW_END_NAMESPACE


// channel_map.cpp
