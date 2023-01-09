#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "cell_app_channels.hpp"
#include "cell_app_channel.hpp"
#include "cellapp_interface.hpp"
#include "profile.hpp"
#include "cellapp.hpp"
#include "spaces.hpp"

#include "network/event_dispatcher.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( CellAppChannels )


/**
 *	Constructor.
 */
CellAppChannels::CellAppChannels( int microseconds,
			Mercury::EventDispatcher & dispatcher ) :
	map_(),
	recentlyDead_(),
	lastTimeOfDeath_( 0 ),
	timer_()
{
	this->startTimer( microseconds, dispatcher );
}


/**
 *	Destructor.
 */
CellAppChannels::~CellAppChannels()
{
	this->stopTimer();
}


/**
 *	This method calls send on all of the CellAppChannels.
 */
void CellAppChannels::sendAll()
{
	static ProfileVal localProfile( "deliverGhosts" );
	START_PROFILE( localProfile );

	// Send all our regular channels
	for (Map::iterator iter = map_.begin(); iter != map_.end(); iter++)
	{
		CellAppChannel * pChannel = iter->second;

		if (pChannel->isGood())
		{
			pChannel->send();
		}
	}

	STOP_PROFILE_WITH_DATA( localProfile, map_.size() );
}


/**
 *
 */
void CellAppChannels::handleTimeout( TimerHandle handle, void * arg )
{
	this->sendAll();
}


/**
 *	This method looks for a CellAppChannel to the given address.
 *	If found, it returns it, otherwise if 'shouldCreate' is true, a new one is
 *	created.
 */
CellAppChannel * CellAppChannels::get( const Mercury::Address & addr,
	bool shouldCreate )
{
	MF_ASSERT_DEV( addr.ip != 0 );

   	Map::iterator iter = map_.find( addr );

	if (iter != map_.end())
	{
		return iter->second;
	}
	else if (shouldCreate)
	{
		if ((lastTimeOfDeath_ +
				RECENTLY_DEAD_PERIOD * stampsPerSecond()) < timestamp())
		{
			recentlyDead_.clear();
		}

		if (recentlyDead_.find( addr ) != recentlyDead_.end())
		{
			return NULL;
		}

		CellAppChannel * pChannel = new CellAppChannel( addr );

		map_[ addr ] = pChannel;

		return pChannel;
	}
	else
	{
		return NULL;
	}
}


/**
 *	This method checks if an address is for a recently dead CellAppChannel.
 */
bool CellAppChannels::hasRecentlyDied( const Mercury::Address & addr ) const
{
	return recentlyDead_.find( addr ) != recentlyDead_.end();
}


/**
 *	This method is called when one (or all) remote processes fail.
 *	Indicate that whole machine has failed by setting port to 0.
 */
void CellAppChannels::remoteFailure( const Mercury::Address & addr )
{
	MF_ASSERT( addr.port != 0 );
	uint64 now = timestamp();

	if (lastTimeOfDeath_ + RECENTLY_DEAD_PERIOD * stampsPerSecond() < now)
	{
		recentlyDead_.clear();
	}

	recentlyDead_.insert( addr );
	lastTimeOfDeath_ = now;

	Map::iterator iter = map_.find( addr );

	if (iter != map_.end())
	{
		// Marking the Channel as remote failed before deleting the owner causes
		// it to be deleted instead of condemned.
		iter->second->channel().setRemoteFailed();

		delete iter->second;
		map_.erase( iter );
	}
}


/**
 *	This method starts or restarts the timer which is responsible for
 *	periodically flushing all the channels.
 */
void CellAppChannels::startTimer( int microseconds,
	Mercury::EventDispatcher & dispatcher )
{
	this->stopTimer();
	timer_ = dispatcher.addTimer( microseconds, this, NULL, "CellAppChannels" );
}


/**
 *	This method stops the timer which is responsible for periodically flushing
 *	all the channels.
 */
void CellAppChannels::stopTimer()
{
	timer_.cancel();
}


/**
 * This method sends gameTime to all other CellApps and flushes
 * all CellApp channels
 */
void CellAppChannels::sendTickCompleteToAll( GameTime gameTime )
{
	CellAppInterface::onRemoteTickCompleteArgs args;

	args.gameTime = gameTime;

	// Send all our regular channels
	for (Map::iterator iter = map_.begin(); iter != map_.end(); iter++)
	{
		CellAppChannel * pChannel = iter->second;

		// Only send to the CellApps who share cells in same spaces with ours
		if (pChannel->isGood() &&
			(CellApp::instance().spaces().haveOtherCellsIn( *pChannel )))
		{
			Mercury::Bundle & bundle = pChannel->bundle();

			bundle << args;
		}
	}
	
	this->sendAll();
}


/**
 * This method handles remote message on tick processing completion
 */
void CellAppChannels::onRemoteTickComplete( const Mercury::Address & srcAddr,
		GameTime remoteGameTime )
{
	CellAppChannel * pChannel = this->get( srcAddr,
			/* shouldCreate: */ false );

	if (pChannel == NULL || !pChannel->isGood())
	{
		return;
	}

	pChannel->lastReceivedTime( remoteGameTime );
}


/**
 * This method returns true if all good channels have received game time
 * not less than the given time. No remote channels is good too.
 */
bool CellAppChannels::haveAllChannelsReceivedTime( GameTime gameTime )
{
	for (Map::iterator iter = map_.begin(); iter != map_.end(); iter++)
	{
		CellAppChannel * pChannel = iter->second;

		if (pChannel->isGood() &&
			(CellApp::instance().spaces().haveOtherCellsIn( *pChannel )) &&
			(pChannel->lastReceivedTime() < gameTime))
		{
			return false;
		}
	}
	return true;
}


BW_END_NAMESPACE

// cell_app_channel.cpp
