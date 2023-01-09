#include "pch.hpp"

#include "delayed_channels.hpp"

#include "udp_channel.hpp"
#include "event_dispatcher.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

DelayedChannels::DelayedChannels() :
	FrequentTask( "DelayedChannels" )
{
}

/**
 *	This method initialises this object.
 */
void DelayedChannels::init( EventDispatcher & dispatcher )
{
	dispatcher.addFrequentTask( this );
}


/**
 *	This method initialises this object.
 */
void DelayedChannels::fini( EventDispatcher & dispatcher )
{
	dispatcher.cancelFrequentTask( this );
}


/**
 *	This method registers a channel for delayed sending. This is used by
 *	irregular channels so that they still have an opportunity to aggregate
 *	messages. Instead of sending a packet for each method call, these are
 *	aggregated.
 */
void DelayedChannels::add( UDPChannel & channel )
{
	if (channel.addr().ip == 0)
	{
		ERROR_MSG( "DelayedChannels::add: Channel with index "
				"%d has been reset. hasUnsentData = %d\n",
			channel.id(), channel.hasUnsentData() );
		ERROR_MSG( "Please send following callstack to "
						"suppport@bigworldtech.com.\n" );
		ERROR_BACKTRACE();
	}

	channels_.insert( &channel );
}


/**
 *	This method performs a send on the given channel if that channel has been
 *	registered as a delayed channel. After sending the channel is removed from
 *	the collection of delayed channels.
 */
void DelayedChannels::sendIfDelayed( UDPChannel & channel )
{
	if (channels_.erase( &channel ) > 0)
	{
		channel.send();
	}
}


/**
 *	This method overrides the FrequentTask method to perform the delayed
 *	sending.
 */
void DelayedChannels::doTask()
{
	Channels::iterator iter = channels_.begin();

	while (iter != channels_.end())
	{
		UDPChannel * pChannel = iter->get();

		if (!pChannel->isDead())
		{
			if (pChannel->addr().ip == 0)
			{
				ERROR_MSG( "DelayedChannels::doTask: Channel with index "
						"%d has been reset. hasUnsentData = %d\n",
					pChannel->id(), pChannel->hasUnsentData() );
			}
			else
			{
				pChannel->send();
			}
		}

		++iter;
	}

	channels_.clear();
}

} // namespace Mercury

BW_END_NAMESPACE

// delayed_channels.cpp
