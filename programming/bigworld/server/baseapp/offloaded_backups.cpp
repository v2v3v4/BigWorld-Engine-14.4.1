#include "script/first_include.hpp"

#include "offloaded_backups.hpp"
#include "baseapp.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: OffloadedBackups
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
OffloadedBackups::OffloadedBackups():
		apps_()
{}


/**
 *	This method stores entity backup data for entities that have been offloaded
 *	to another BaseApp. 
 */
void OffloadedBackups::backUpEntity( const Mercury::Address & srcAddr,
									 BinaryIStream & data )
{
	EntityID entityID;
	data >> entityID;

	BW::string & backupData = apps_[ srcAddr ].getDataFor( entityID );
	int dataSize = data.remainingLength();
	backupData.assign( (char *)data.retrieve( dataSize ), dataSize );
}


/**
 *	This method handles a BaseApp death by restoring entities that were
 *	offloaded to the dead BaseApp onto a live BaseApp according to the backup
 *	hash.
 */
void OffloadedBackups::handleBaseAppDeath( const Mercury::Address & deadAddr,
	   const BackupHashChain & backedUpHash )
{
	apps_[ deadAddr ].restoreRemotely( deadAddr, backedUpHash );

	// Once the cluster has recovered from one death, it cannot recover from
	// another. Thus, clear the backups completely to tie up any loose ends
	// regarding offload confirmation.
	apps_.clear();
}


/**
 *	Returns true if the given entity ID has been registered as an offloaded
 *	entity.
 */
bool OffloadedBackups::wasOffloaded( EntityID entityID ) const
{
	Container::const_iterator iApp = apps_.begin();
	while (iApp != apps_.end())
	{
		if (iApp->second.contains( entityID ))
		{
			return true;
		}
		iApp++;
	}
	return false;
}

/**
 *  Another BaseApp has taken responsibility for backing up this entity
 *  we may cease to back it up ourselves.
 */ 
void OffloadedBackups::stopBackingUpEntity( EntityID entityID )
{
	Container::iterator iApp = apps_.begin();
	Container::iterator iLastApp;
	while (iApp != apps_.end())
	{
		iLastApp = iApp;
		iApp++;
		iLastApp->second.erase( entityID );
		if (iLastApp->second.empty())
		{
			apps_.erase( iLastApp );
		}
	}
}

BW_END_NAMESPACE

// offloaded_backups.cpp

