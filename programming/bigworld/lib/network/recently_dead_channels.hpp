#ifndef RECENTLY_DEAD_CHANNELS_HPP
#define RECENTLY_DEAD_CHANNELS_HPP

#include "basictypes.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/timer_handler.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class EventDispatcher;

/**
 *	This class is used by external interfaces to remember recently deregistered
 *	channels for a little while. It drops incoming packets from those addresses.
 *	This is to avoid processing packets from recently disconnected clients,
 *	especially ones using channel filters. Attempting to process these packets
 *	generates spurious corruption warnings because they are processed raw once
 *	the channel is gone.
 */
class RecentlyDeadChannels : private TimerHandler
{
public:
	RecentlyDeadChannels( EventDispatcher & dispatcher );
	~RecentlyDeadChannels();

	void add( const Address & addr );

	bool isDead( const Address & addr ) const;

private:
	virtual void handleTimeout( TimerHandle handle, void * arg );

	/// The mapping is addresses of recently deregistered channels, mapped to
	/// the timer ID for when they will time out.
	typedef BW::map< Address, TimerHandle > Channels;
	Channels channels_;

	EventDispatcher & dispatcher_;
};

} // end namespace Mercury

BW_END_NAMESPACE

#endif // RECENTLY_DEAD_CHANNELS_HPP
