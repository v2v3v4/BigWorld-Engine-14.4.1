#include "script/first_include.hpp"

#include "ack_cell_app_death_helper.hpp"

#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "entity.hpp"
#include "real_entity.hpp"
#include "../cellappmgr/cellappmgr_interface.hpp"

#include "network/condemned_channels.hpp"
#include "network/event_dispatcher.hpp"
#include "network/nub_exception.hpp"


BW_BEGIN_NAMESPACE

int AckCellAppDeathHelper::s_numInstances_ = 0;


AckCellAppDeathHelper::AckCellAppDeathHelper( CellApp & app,
		const Mercury::Address & deadAddr ) :
	app_( app ),
	deadAddr_( deadAddr ),
	numBadGhosts_( 0 ),
	timerHandle_()
{
	// Disable offloading for all cells until we're in a good state again.
	app_.shouldOffload( false );
	++s_numInstances_;
}


/**
 *  This method periodically checks to see if all entity channels on this
 *  app have been cleared of 'critical' status.
 */
void AckCellAppDeathHelper::handleTimeout( TimerHandle handle, void * arg )
{
	// Iterate through the recently onloaded entities and see which ones are
	// no longer in a channel-critical state.
	RecentOnloads::iterator iter = recentOnloads_.begin();

	while (iter != recentOnloads_.end())
	{
		RealEntity * pReal = (*iter)->pReal();
		RecentOnloads::iterator oldIter = iter++;

		// Hurry up the resends on channels that are still in flux.
		if (pReal && pReal->channel().hasUnackedCriticals())
		{
			pReal->channel().resendCriticals();
		}

		// Entities who are no longer critical are done, as well as entities
		// that are no longer real (i.e. destroyed in script or teleported).
		else
		{
			recentOnloads_.erase( oldIter );
		}
	}

	this->checkFinished();
}


/**
 *  This method handles replies to emergencySetCurrentCell messages.
 */
void AckCellAppDeathHelper::handleMessage( const Mercury::Address & srcAddr,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data,
	void * arg )
{
	--numBadGhosts_;
	this->checkFinished();
}


/**
 *  This method handles failed delivery of emergencySetCurrentCell messages.
 */
void AckCellAppDeathHelper::handleException( const Mercury::NubException & exc,
	void * arg )
{
	Mercury::Address addr;
	EntityID entityID = (EntityID)uintptr(arg);

	if (exc.getAddress( addr ))
	{
		ERROR_MSG( "AckCellAppDeathHelper::handleException( %s ): "
			"Got exception %s from %s\n",
			deadAddr_.c_str(), Mercury::reasonToString( exc.reason() ),
			addr.c_str() );
	}
	else
	{
		ERROR_MSG( "AckCellAppDeathHelper::handleException( %s ): "
			"Got exception %s (%u)\n",
			deadAddr_.c_str(), Mercury::reasonToString( exc.reason() ),
			entityID );
	}

	--numBadGhosts_;
}


/**
 *  This method should be called when we are ready to start checking for when
 *  it's safe to send the ACK back to the CellAppMgr.
 */
void AckCellAppDeathHelper::startTimer()
{
	// Calculate the average RTT of all the real channels.  We clamp each
	// RTT to the server tick rate since the initial RTT is quite large and
	// entities who haven't sent much may skew the calculations a bit.
	uint64 tickTime = stampsPerSecond() / CellAppConfig::updateHertz();
	uint64 avgRTT = tickTime;

	for (RecentOnloads::iterator iter = recentOnloads_.begin();
		 iter != recentOnloads_.end(); ++iter)
	{
		EntityPtr pEntity = *iter;
		avgRTT += std::min( pEntity->pReal()->channel().roundTripTime(),
			tickTime );
	}

	avgRTT /= recentOnloads_.size() + 1;

	// We use twice the RTT as the check-period to avoid generating too much
	// traffic.
	checkPeriod_ = int( (avgRTT * 1000000) / stampsPerSecond() ) * 2;
	timerHandle_ = app_.mainDispatcher().addTimer( checkPeriod_, this, NULL,
		"AckCellAppDeathHelper" );
	startTime_ = timestamp();
}


/**
 *  Checks to see if we're done and can ack back to the CellAppMgr.  This method
 *  may delete this object so it can only be called right at the end of another
 *  method call.
 */
void AckCellAppDeathHelper::checkFinished()
{
	// We need to check there are no condemned criticals because if an entity
	// was destroyed after the death notification arrived, it will have been
	// purged from recentOnloads_ in handleTimeout().  We need to make sure the
	// BaseApp knows it was here before it was destroyed.
	const Mercury::CondemnedChannels & condemned =
		app_.interface().condemnedChannels();

	if (recentOnloads_.empty() &&
		numBadGhosts_ == 0 &&
		condemned.numCriticalChannels() == 0)
	{
		// Send back the ACK to the CellAppMgr
		app_.cellAppMgr().ackCellAppDeath( deadAddr_ );

		timerHandle_.cancel();

		// If this is the last one of these, allow offloading again.
		if (s_numInstances_ == 1)
		{
			app_.shouldOffload( true );
		}

		DEBUG_MSG( "Acked death of %s after %.3fs (%dms period)\n",
			deadAddr_.c_str(),
			stampsToSeconds( timestamp() - startTime_ ),
			checkPeriod_ / 1000 );

		delete this;
	}
}

BW_END_NAMESPACE

// ack_cell_app_death_helper.cpp

