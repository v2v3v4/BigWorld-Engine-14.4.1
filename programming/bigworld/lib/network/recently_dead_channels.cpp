#include "pch.hpp"

#include "recently_dead_channels.hpp"

#include "event_dispatcher.hpp"


BW_BEGIN_NAMESPACE


namespace Mercury
{

/**
 *	Constructor.
 */
RecentlyDeadChannels::RecentlyDeadChannels( EventDispatcher & dispatcher ) :
	dispatcher_( dispatcher )
{
}


/**
 *	Destructor.
 */
RecentlyDeadChannels::~RecentlyDeadChannels()
{
	Channels::iterator iter = channels_.begin();

	while (iter != channels_.end())
	{
		iter->second.cancel();

		++iter;
	}

	channels_.clear();
}


/**
 *	This method adds an address to the recently dead collection.
 */
void RecentlyDeadChannels::add( const Address & addr )
{
	Channels::iterator iter = channels_.begin();

	if (iter != channels_.end())
	{
		iter->second.cancel();
	}

	TimerHandle timeoutHandle = dispatcher_.addOnceOffTimer( 60 * 1000000,
			this, NULL );

	channels_[ addr ] = timeoutHandle;
}


/**
 *	This method handles timer events.
 */
void RecentlyDeadChannels::handleTimeout( TimerHandle handle, void * arg )
{
	// Find the dead channel in the map
	Channels::iterator iter = channels_.begin();

	while (iter != channels_.end())
	{
		if (iter->second == handle)
		{
			channels_.erase( iter );
			break;
		}

		++iter;
	}
}


/**
 *	This method returns whether an address is associated with a channel that
 *	was recently destroyed.
 */
bool RecentlyDeadChannels::isDead( const Address & addr ) const
{
	return channels_.find( addr ) != channels_.end();
}

} // namespace Mercury


BW_END_NAMESPACE


// recently_dead_channels.cpp
