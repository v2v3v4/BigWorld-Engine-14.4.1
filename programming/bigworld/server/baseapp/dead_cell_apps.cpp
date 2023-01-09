#include "script/first_include.hpp"

#include "dead_cell_apps.hpp"

#include "base.hpp"
#include "bases.hpp"
#include "baseapp.hpp" // For nextTickPending
#include "proxy.hpp"

#include "cstdmf/timestamp.hpp"

#include "network/basictypes.hpp"
#include "network/udp_channel.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: DyingCellApp
// -----------------------------------------------------------------------------

/**
 *	This class handles a dying CellApp. That is, a CellApp that has died but
 *	all of the local base entities with their cell part on this app has yet to
 *	be restored to a backup cellapp.
 */
class DyingCellApp
{
public:
	DyingCellApp( const Mercury::Address & addr, BinaryIStream & data );

	bool tick( const Bases & bases );
	void onCompleted( Mercury::NetworkInterface & intInterface );

private:
	Mercury::Address destinationFor( const Base * pBase );

	Mercury::Address addr_;

	typedef BW::map< SpaceID, Mercury::Address > NewDestinations;
	NewDestinations newDestinations_;

	int numDiscarded_;
};


// -----------------------------------------------------------------------------
// Section: DeadCellApp
// -----------------------------------------------------------------------------

/**
 *  A class for keeping track of CellApps that have died recently.
 */
class DeadCellApp
{
public:
	DeadCellApp( const Mercury::Address & addr ) :
		addr_( addr ),
		timestamp_( timestamp() )
	{}

	const Mercury::Address & addr() const	{ return addr_; }
	bool isOlderThan( uint64 cutoff ) const	{ return timestamp_ < cutoff; }

	static uint64 cutoffTime()				
	{
		return timestamp() - (MAX_AGE * stampsPerSecond());
	}

private:
	/// How long we keep these things around for.  Basically the purpose of
	/// them is to ensure that emergencySetCurrentCell messages that are
	/// received after the handleCellAppDeath message cause an immediate
	/// restore.
	static const int MAX_AGE = 10; // seconds

	/// The address of the dead app
	Mercury::Address addr_;

	/// The time at which we found out about the app death
	uint64 timestamp_;
};


// -----------------------------------------------------------------------------
// Section: DeadCellApps
// -----------------------------------------------------------------------------

/**
 *	This method returns whether or not an address is of a recently dead CellApp.
 */
bool DeadCellApps::isRecentlyDead( const Mercury::Address & addr ) const
{
	for (Container::const_iterator iter = apps_.begin();
			iter != apps_.end();
			++iter)
	{
		if ((*iter)->addr() == addr)
		{
			return true;
		}
	}

	return false;
}


/**
 *	This method adds the address of a recently dead CellApp.
 */
void DeadCellApps::addApp( const Mercury::Address & addr, BinaryIStream & data )
{
	this->removeOldApps();

	// Remember this CellApp as having died recently.
	apps_.push_back( shared_ptr< DeadCellApp>( new DeadCellApp( addr ) ) );

	// The dying cellapps are actually dead but still have some cell entities
	// that need to be restore elsewhere.
	dyingApps_.push_back(
			shared_ptr< DyingCellApp >( new DyingCellApp( addr, data ) ) );
}


/**
 *	This method removes the apps that have been dead for a long time.
 */
void DeadCellApps::removeOldApps()
{
	uint64 cutoff = DeadCellApp::cutoffTime();

	Container::iterator iter = apps_.begin();

	while (iter != apps_.end())
	{
		if ((*iter)->isOlderThan( cutoff ))
		{
			iter = apps_.erase( iter );
		}
		else
		{
			++iter;
		}
	}
}


/**
 *	This method process any dying cellapps. It should be called once per tick.
 */
void DeadCellApps::tick( const Bases & bases,
		Mercury::NetworkInterface & intInterface )
{
	while (!dyingApps_.empty())
	{
		if (dyingApps_.front()->tick( bases ))
		{
			dyingApps_.front()->onCompleted( intInterface );
			dyingApps_.pop_front();
		}
		else
		{
			// Timed out. Wait for next tick to continue
			return;
		}
	}
}


// -----------------------------------------------------------------------------
// Section: DyingCellApp
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param addr The address of the recently dead CellApp.
 *	@param data A stream containing information about where to restore entities.
 */
DyingCellApp::DyingCellApp( const Mercury::Address & addr,
		BinaryIStream & data ) :
	addr_( addr ),
	newDestinations_(),
	numDiscarded_( 0 )
{
	const int ENTRY_SIZE = sizeof( SpaceID ) + sizeof( Mercury::Address );

	if ((data.remainingLength() % ENTRY_SIZE) != 0)
	{
		CRITICAL_MSG( "BaseApp::handleCellAppDeath: Bad message length. "
				"Addr = %s. Len = %d. entrySize = %d\n",
			addr.c_str(),
			data.remainingLength(),
			ENTRY_SIZE );
	}

	int count = data.remainingLength() / ENTRY_SIZE;

	SpaceID spaceID = 0;
	Mercury::Address newDest;

	for (int i = 0; i < count; ++i)
	{
		data >> spaceID >> newDest;
		newDestinations_[ spaceID ] = newDest;

		INFO_MSG( "DyingCellApp::DyingCellApp( %s ): "
				"space id = %u.  newCellApp = %s\n",
			addr_.c_str(), spaceID, newDest.c_str() );
	}
}


/**
 *	This method returns the address of the CellApp where the given entity
 *	should be restored to.
 */
Mercury::Address DyingCellApp::destinationFor( const Base * pBase )
{
	NewDestinations::iterator destIter =
		newDestinations_.find( pBase->spaceID() );

	return destIter != newDestinations_.end() ?
				destIter->second : Mercury::Address::NONE;
}


/**
 *	This method will restore as many base entities as possible away from this
 *	dead cellapp. It stops when the next game tick is pending.
 *
 *	@return true if all entities have been restored, otherwise false.
 */
bool DyingCellApp::tick( const Bases & bases )
{
	int numRestored = 0;

	Bases::const_iterator iter = bases.begin();

	while (iter != bases.end())
	{
		Base * pBase = iter->second;
		++iter; // Increment early as we may discard this entity.

		if (pBase->cellAddr() == addr_)
		{
			++numRestored;

			// Check if this entity is on one of the dead cells.
			Mercury::Address newDest = this->destinationFor( pBase );

			if (newDest == Mercury::Address::NONE)
			{
				ERROR_MSG( "BaseApp::handleCellAppDeath: "
						"No alternate CellApp for %u (on space %u)\n",
					pBase->id(), pBase->spaceID() );
			}

			if (BaseApp::instance().isRecentlyDeadCellApp( newDest ))
			{
				newDest = Mercury::Address::NONE;
			}

			if (newDestinations_.empty())
			{
				++numDiscarded_;
				pBase->discard();
			}
			else if (!pBase->restoreTo( pBase->spaceID(), newDest ))
			{
				CRITICAL_MSG( "BaseApp::handleCellAppDeath: "
							  "%u could not be restored to space %u\n",
							  pBase->id(), pBase->spaceID() );
			}
			else if (pBase->isProxy())
			{
				// we also tell the client about this now
				((Proxy*)pBase)->restoreClient();
			}

			if (BaseApp::instance().nextTickPending())
			{
				WARNING_MSG( "DyingCellApp::tick: "
								"Next tick pending after restoring %d\n",
							numRestored );
				return false;
			}
		}
	}

	return true;
}


/**
 *	This method is called when all base entities that were on this dying cellapp
 *	have been restored correctly.
 */
void DyingCellApp::onCompleted( Mercury::NetworkInterface & intInterface )
{
	Mercury::UDPChannel * pDeadChannel = intInterface.findChannel( addr_ );

	if (pDeadChannel)
	{
		pDeadChannel->condemn();
	}

	if (numDiscarded_ > 0)
	{
		MF_ASSERT( newDestinations_.empty() );
		NOTICE_MSG( "BaseApp::handleCellAppDeath: "
				"No restore locations. Num discarded = %d\n",
			numDiscarded_ );
	}

	BaseApp::instance().scriptEvents().triggerEvent( "onCellAppDeath",
			Py_BuildValue( "(s)", addr_.c_str() ) );
}

BW_END_NAMESPACE

// dead_cell_apps.cpp
