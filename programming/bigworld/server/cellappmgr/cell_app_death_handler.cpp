#include "cell_app_death_handler.hpp"

#include "cellappmgr.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"

#include "network/bundle.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
CellAppDeathHandler::CellAppDeathHandler( const Mercury::Address & deadAddr,
	   int ticksToWait ) :
	deadAddr_( deadAddr ),
	ticksToWait_( ticksToWait )
{
}


/**
 *	This method informs this handler to wait for a response from the given
 *	address.
 */
void CellAppDeathHandler::addWaiting( const Mercury::Address & addr )
{
	if (!waitingFor_.insert( addr ).second)
	{
		ERROR_MSG( "CellAppDeathHandler::addWaiting: "
				"Trying to wait for %s twice\n", addr.c_str() );
	}
}


/**
 *
 */
void CellAppDeathHandler::clearWaiting( const Mercury::Address & waitingAddr,
		const Mercury::Address & deadAddr )
{
	if (deadAddr != deadAddr_)
	{
		BW::string gotAddr = deadAddr.c_str();
		BW::string expectedAddr = deadAddr_.c_str();
		WARNING_MSG( "CellAppDeathHandler::clearWaiting: "
				"Got dead addr %s from %s but expecting %s\n",
			gotAddr.c_str(), waitingAddr.c_str(), expectedAddr.c_str() );
		return;
	}

	if (!waitingFor_.erase( waitingAddr ))
	{
		ERROR_MSG( "CellAppDeathHandler::clearWaiting: "
						"Unable to clear %s\n", waitingAddr.c_str() );
	}

	if (waitingFor_.empty())
	{
		// WARNING: This call will delete this object.
		this->finish();
	}
}


/**
 *
 */
void CellAppDeathHandler::tick()
{
	if (--ticksToWait_ <= 0)
	{
		WARNING_MSG( "CellAppDeathHandler::tick: "
					"Did not receive responses from %" PRIzu " CellApps\n",
				waitingFor_.size() );
		WaitingForSet::const_iterator iter = waitingFor_.begin();
		while (iter != waitingFor_.end())
		{
			WARNING_MSG( "\tWaiting for: %s\n", iter->c_str() );
			++iter;
		}

		// WARNING: This call deletes this object.
		this->finish();
	}
}


/**
 *
 */
void CellAppDeathHandler::finish()
{
	CellAppMgr & cellAppMgr = CellAppMgr::instance();
	BaseAppMgr & baseAppMgr = cellAppMgr.baseAppMgr();
	Mercury::Bundle & bundle = baseAppMgr.bundle();
	bundle.startMessage( BaseAppMgrInterface::handleCellAppDeath );
	bundle << deadAddr_;
	bundle.transfer( stream_, stream_.remainingLength() );
	baseAppMgr.send();

	cellAppMgr.clearCellAppDeathHandler( deadAddr_ );
	delete this;
}

BW_END_NAMESPACE

// cell_app_death_handler.cpp
