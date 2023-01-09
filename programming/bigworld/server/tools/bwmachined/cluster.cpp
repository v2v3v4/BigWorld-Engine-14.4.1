#include <algorithm>
#include "cluster.hpp"
#include "bwmachined.hpp"
#include "cstdmf/memory_stream.hpp"
#include "network/portmap.hpp"
#include <syslog.h>


BW_BEGIN_NAMESPACE

/**
 * Constructor.
 */
Cluster::Cluster( BWMachined &machined ) :
	machined_( machined ),
	buddyAddr_( 0 ),
	floodTriggerHandler_( *this ),
	pFloodReplyHandler_( NULL ),
	birthHandler_( *this )
{}


/**
 * Work out the correct value for buddyAddr based on our current knowledge of
 * the network. We handle machine death by keeping track of the machine with the
 * next highest IP address (i.e. machines are in a ring).  Each machine also
 * periodically sends out broadcasts to handle network re-connection of forests.
 */
void Cluster::chooseBuddy()
{
	uint32 oldBuddy = buddyAddr_;
	buddyAddr_ = 0;

	uint32 lowest = 0xFFFFFFFF;
	for (Addresses::iterator it = machines_.begin(); it != machines_.end(); ++it)
	{
		if (*it > ownAddr_ && (buddyAddr_ == 0 || *it < buddyAddr_))
			buddyAddr_ = *it;
		if (*it < lowest && *it != ownAddr_)
			lowest = *it;
	}

	if (buddyAddr_ == 0 && machines_.size() > 1)
		buddyAddr_ = lowest;

	MGMPacket::setBuddy( buddyAddr_ );

	if (buddyAddr_ != oldBuddy)
	{
		if (buddyAddr_)
			syslog( LOG_INFO, "Buddy is %s", inet_ntoa( (in_addr&)buddyAddr_ ) );
		else
			syslog( LOG_INFO, "I have no buddy" );
	}
}


// -----------------------------------------------------------------------------
// Section: ClusterTimeoutHandler
// -----------------------------------------------------------------------------

/**
 *
 */
void Cluster::ClusterTimeoutHandler::addTimer()
{
	handle_ = cluster_.machined_.callbacks().add(
		cluster_.machined_.timeStamp() + this->delay(),
		this->interval(), this, NULL, "ClusterTimeoutHandler" );
}


/**
 *
 */
void Cluster::ClusterTimeoutHandler::cancel()
{
	if (!handle_.isSet())
	{
		syslog( LOG_ALERT, "Tried to cancel() a CTH before doing addTimer()" );
		return;
	}

	handle_.cancel();
}

// -----------------------------------------------------------------------------
// Section: FloodTriggerHandler
// -----------------------------------------------------------------------------


/**
 *
 */
void Cluster::FloodTriggerHandler::handleTimeout( TimerHandle handle,
		void *pUser )
{
	FloodReplyHandler *& rpHandler = cluster_.pFloodReplyHandler_;

	// Check on the previous timer
	if (rpHandler != NULL)
	{
		if (rpHandler->handle().isSet())
		{
			syslog( LOG_WARNING,
				"Old FloodReplyHandler seems to be taking a long time" );
			return;
		}
		else
			delete rpHandler;
	}

	// Trigger new flood keepalive
	rpHandler = new FloodReplyHandler( cluster_ );

	// Re-schedule yourself (since this isn't a repeating timer in the TimeQueue
	// sense of the word)
	this->addTimer();
}


/**
 *
 */
TimeQueue::TimeStamp Cluster::FloodTriggerHandler::delay() const
{
	TimeQueue::TimeStamp min = AVERAGE_INTERVAL;
	TimeQueue::TimeStamp max =
		std::max( min+1,
				TimeQueue::TimeStamp(AVERAGE_INTERVAL *
					cluster_.machines_.size() * 2) );

	TimeQueue::TimeStamp interval = min + rand() % (max-min);
	return interval;
}


// -----------------------------------------------------------------------------
// Section: FloodReplyHandler
// -----------------------------------------------------------------------------

/**
 *
 */
Cluster::FloodReplyHandler::FloodReplyHandler( Cluster &cluster ) :
	ClusterTimeoutHandler( cluster ), tries_( MAX_RETRIES )
{
	this->sendBroadcast();
	this->addTimer();
}


/**
 *
 */
void Cluster::FloodReplyHandler::handleTimeout( TimerHandle handle,
		void *pUser )
{
	// Figure out if anyone new is on the network
	bool births = false;
	for (Addresses::iterator it = replied_.begin();
		 it != replied_.end(); ++it)
	{
		if (cluster_.machines_.find( *it ) == cluster_.machines_.end())
		{
			births = true;
			cluster_.machines_.insert( *it );
			syslog( LOG_INFO, "Discovered new machine %s",
				inet_ntoa( (in_addr&)*it ) );
		}
	}

	if (births)
		cluster_.chooseBuddy();

	// Figure out if anyone's gone away
	BW::vector< uint32 > deaths;
	for (Addresses::iterator it = cluster_.machines_.begin();
		 it != cluster_.machines_.end(); ++it)
	{
		if (replied_.find( *it ) == replied_.end())
			deaths.push_back( *it );
	}

	tries_--;

	// If we got what we were expecting, terminate this timer
	if (!births && deaths.empty())
	{
		this->cancel();
	}

	// If machines appear to have died but we still have tries left, send out
	// the broadcast again
	else if (deaths.size() && tries_ > 0)
	{
		this->sendBroadcast();
	}

	// If no more tries or there are only new machines about, announce findings
	else
	{
		MGMPacket packet;

		// Announce deaths
		if (deaths.size())
		{
			syslog( LOG_INFO, "Machines have died or become unreachable" );
			for (BW::vector< uint32 >::iterator it = deaths.begin();
				 it != deaths.end(); ++it)
			{
				MachinedAnnounceMessage *pMam = new MachinedAnnounceMessage();
				pMam->addr_ = *it;
				pMam->type_ = pMam->ANNOUNCE_DEATH;
				packet.append( *pMam, true );
			}
		}

		// If there are any births, we must announce all known machines
		if (births)
		{
			for (Addresses::iterator it = cluster_.machines_.begin();
				 it != cluster_.machines_.end(); ++it)
			{
				MachinedAnnounceMessage *pMam = new MachinedAnnounceMessage();
				pMam->addr_ = *it;
				pMam->type_ = pMam->ANNOUNCE_EXISTS;
				packet.append( *pMam, true );
			}
		}

		MemoryOStream os;
		packet.write( os );
		cluster_.machined_.endpoint().sendto(
			os.data(), os.size(), htons( PORT_MACHINED ) );

		this->cancel();
	}
}


/**
 *
 */
void Cluster::FloodReplyHandler::sendBroadcast()
{
	if (!wmm_.sendto( cluster_.machined_.endpoint(), htons( PORT_MACHINED ),
			BROADCAST, MGMPacket::PACKET_STAGGER_REPLIES ))
	{
		syslog( LOG_ERR, "Couldn't send broadcast keepalive poll!" );
	}
}


/**
 *
 */
TimeQueue::TimeStamp Cluster::FloodReplyHandler::delay() const
{
	// Floods should finish in less than the time between trigger timeouts
	return Cluster::FloodTriggerHandler::AVERAGE_INTERVAL / MAX_RETRIES;
}


// -----------------------------------------------------------------------------
// Section: BirthReplyHandler
// -----------------------------------------------------------------------------

/**
 *
 */
void Cluster::BirthReplyHandler::addTimer()
{
	MachinedAnnounceMessage mam;
	mam.type_ = mam.ANNOUNCE_BIRTH;
	mam.count_ = 0;
	mam.sendto( cluster_.machined_.endpoint(), htons( PORT_MACHINED ),
		BROADCAST, MGMPacket::PACKET_STAGGER_REPLIES );

	toldSize_ = 1;
	cluster_.machines_.clear();
	cluster_.buddyAddr_ = 0;
	ClusterTimeoutHandler::addTimer();
}


/**
 *
 */
void Cluster::BirthReplyHandler::handleTimeout( TimerHandle handle,
		void *pUser )
{
	if (cluster_.machines_.size() == toldSize_)
		this->cancel();
	else
		this->addTimer();
}

// TODO: Better name?
/**
 *
 */
void Cluster::BirthReplyHandler::markReceived( uint32 addr, uint32 count )
{
	cluster_.machines_.insert( addr );

	// We believe the highest value for the size of the cluster
	toldSize_ = std::max( toldSize_, count );

	if (cluster_.machines_.size() == toldSize_)
	{
		syslog( LOG_INFO,
			"Bootstrap complete; %" PRIzu " machines on network",
			cluster_.machines_.size() );
		cluster_.chooseBuddy();
	}
}


/**
 *
 */
TimeQueue::TimeStamp Cluster::BirthReplyHandler::delay() const
{
	return 2 * cluster_.machined_.maxPacketDelay();
}

BW_END_NAMESPACE

// cluster.cpp
