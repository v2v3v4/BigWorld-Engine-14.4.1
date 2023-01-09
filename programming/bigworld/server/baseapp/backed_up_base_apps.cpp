#include "script/first_include.hpp"

#include "backed_up_base_apps.hpp"

#include "baseapp.hpp"

#include "network/unpacked_message_header.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This method handles a message informing this BaseApp that it should start
 *	being a backup for another BaseApp.
 */
void BackedUpBaseApps::startAppBackup( const Mercury::Address & realBaseAppAddr,
		uint32 index, uint32 hashSize, uint32 prime, bool isInitial )
{
	INFO_MSG( "BackedUpBaseApps::startAppBackup: %s backup on %s hash(id)%%%u = index %u\n",
			isInitial ? "Starting" : "Using",
			realBaseAppAddr.c_str(), hashSize, index );

	MiniBackupHash hash( prime, hashSize );

	bool appExists = (apps_.find( realBaseAppAddr ) != apps_.end());

	BackedUpBaseApp & newApp = apps_[ realBaseAppAddr ];

	if (isInitial)
	{
		newApp.startNewBackup( index, hash );

		if (newApp.canSwitchToNewBackup())
		{
			// We've already received the final message from BaseAppMgr.
			newApp.switchToNewBackup();
		}
	}
	else if (!appExists)
	{
		TRACE_MSG( "BackedUpBaseApps::startAppBackup: "
				"Received final message before initial for %s \n",
			realBaseAppAddr.c_str() );
		newApp.canSwitchToNewBackup( true );
	}
	else
	{
		newApp.switchToNewBackup();
	}
}


/**
 *	This method handles a message informing this BaseApp that it should stop
 *	being a backup for another BaseApp.
 */
void BackedUpBaseApps::stopAppBackup( const Mercury::Address & realBaseAppAddr,
		uint32 index, uint32 hashSize, uint32 prime, bool isPending )
{
	INFO_MSG( "BackedUpBaseApps::stopAppBackup: Stopping "
			"backup on %s hash(id)%%%u = index %u\n",
			realBaseAppAddr.c_str(), hashSize, index );

	Container::iterator iter = apps_.find( realBaseAppAddr );

	if (iter != apps_.end())
	{
		// MF_ASSERT( index == iter->second.index() );
		// MF_ASSERT( hashSize == iter->second.hashSize() );

		if (!isPending)
		{
			apps_.erase( iter );
		}
		else
		{
			if (iter->second.hasHadCurrentBackup())
			{
				iter->second.discardNewBackup();
			}
			else
			{
				INFO_MSG( "BackedUpBaseApps::stopAppBackup: was pending move to new backup, "
					"but current backup is empty, discarding whole app." );
				apps_.erase( iter );
			}
		}
	}
	else
	{
		ERROR_MSG( "BackedUpBaseApps::stopAppBackup: "
				"Not a backup for %s %u/%u\n",
			realBaseAppAddr.c_str(), index, hashSize );
	}
}


/**
 *	This method handles the message that backs up a base entity from another
 *	BaseApp.
 */
void BackedUpBaseApps::backUpEntity( const Mercury::Address & srcAddr,
		EntityID entityID, BinaryIStream & data )
{
	Container::iterator iter = apps_.find( srcAddr );
	if (iter == apps_.end())
	{
		ERROR_MSG( "BackedUpBaseApps::backUpEntity: Not a backup for %s "
				   "cannot back up entity %d\n",
				   srcAddr.c_str(), entityID );
		return;
	}

	BW::string & backupData = iter->second.getDataFor( entityID );
	int dataSize = data.remainingLength();
	backupData.assign( (char *)data.retrieve( dataSize ), dataSize );

}


/**
 *	This method handles the message that this BaseApp should stop being a backup
 *	for an entity.
 */
void BackedUpBaseApps::stopEntityBackup( const Mercury::Address & srcAddr,
		EntityID entityID )
{
	Container::iterator iter = apps_.find( srcAddr );

	if (iter != apps_.end())
	{
		iter->second.erase( entityID );
	}
	else
	{
		WARNING_MSG( "BackedUpBaseApps::stopEntityBackup: "
				"Have not yet got backup of %u from %s\n",
			entityID, srcAddr.c_str() );
	}
}


/**
 *	This method responds to a BaseApp dying. If this BaseApp was backing up
 *	any entities from that BaseApp, they are restored here.
 */
void BackedUpBaseApps::handleBaseAppDeath( const Mercury::Address & deadAddr )
{
	// Check if we were backing up the BaseApp
	Container::iterator iter = apps_.find( deadAddr );

	if (iter != apps_.end())
	{
		BackedUpBaseApp & backupApp = iter->second;

		backupApp.restore();

		apps_.erase( iter );
	}
}


/**
 *	This function is called when a base has been offloaded to this app.
 *	If another baseapp has given us a new entity, we don't want to try to
 *	restore it a second time if the source crashes.
 */
void BackedUpBaseApps::onloadedEntity( const Mercury::Address & srcAddr, 
									   EntityID entityID )
{
	Container::iterator iter = apps_.find( srcAddr );

	if (iter != apps_.end())
	{
		iter->second.erase( entityID );
	}
}


/**
 *	This method returns the total number of base entities being backed up.
 */
size_t BackedUpBaseApps::numEntitiesBackedUp() const
{
	size_t count = 0;
	Container::const_iterator iter = apps_.begin();

	while (iter != apps_.end())
	{
		count += iter->second.numInCurrentBackup();
		++iter;
	}

	return count;
}


/**
 *	This method creates a watcher that can inspect instances of this object.
 */
/*static*/ WatcherPtr BackedUpBaseApps::pWatcher()
{
	DirectoryWatcherPtr pWatcher = new DirectoryWatcher();

	BackedUpBaseApps * pNULL = NULL;

	Watcher * pMapWatcher =
		new MapWatcher< Container >( pNULL->apps_ );
	pMapWatcher->addChild( "*", BackedUpBaseApp::pWatcher() );
	pWatcher->addChild( "apps", pMapWatcher );

	pWatcher->addChild( "numEntitiesBackedUp",
			makeWatcher( &BackedUpBaseApps::numEntitiesBackedUp ) );

	return pWatcher;
}

BW_END_NAMESPACE

// backed_up_base_apps.cpp
