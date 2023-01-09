#ifndef CELL_APP_CHANNELS_HPP
#define CELL_APP_CHANNELS_HPP

#include "cstdmf/singleton.hpp"
#include "cstdmf/timer_handler.hpp"

#include "network/basictypes.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

class CellAppChannel;

namespace Mercury
{
class EventDispatcher;
}


/**
 *	This class is the collection of CellAppChannel instances.
 */
class CellAppChannels : public Singleton< CellAppChannels >,
	public TimerHandler
{
public:
	CellAppChannels( int microseconds, Mercury::EventDispatcher & dispatcher );
	~CellAppChannels();

	CellAppChannel * get( const Mercury::Address & addr,
		bool shouldCreate = true );

	bool hasRecentlyDied( const Mercury::Address & addr ) const;

	void remoteFailure( const Mercury::Address & addr );

	void startTimer( int microseconds, Mercury::EventDispatcher & dispatcher );

	void stopTimer();

	void sendAll();

	void sendTickCompleteToAll( GameTime gameTime );

	void onRemoteTickComplete( const Mercury::Address & srcAddr, GameTime remoteGameTime );

	bool haveAllChannelsReceivedTime( GameTime gameTime );

private:
	virtual void handleTimeout( TimerHandle handle, void * arg );

	typedef BW::map< Mercury::Address, CellAppChannel * > Map;
	typedef Map::iterator iterator;

	Map map_;

	static const int RECENTLY_DEAD_PERIOD = 60;

	typedef BW::set< Mercury::Address > RecentlyDead;
	RecentlyDead recentlyDead_;

	uint64 lastTimeOfDeath_;
	TimerHandle timer_;
};

BW_END_NAMESPACE

#endif // CELL_APP_CHANNELS_HPP
