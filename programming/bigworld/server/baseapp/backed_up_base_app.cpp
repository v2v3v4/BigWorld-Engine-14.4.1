#include "script/first_include.hpp"

#include "backed_up_base_app.hpp"

#include "base.hpp"
#include "baseapp.hpp"
#include "entity_type.hpp"
#include "offloaded_backups.hpp"

#include "cstdmf/memory_stream.hpp"

#include "network/bundle.hpp"
#include "network/bundle_sending_map.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This method handles the backup hash for a backed up BaseApp changing.
 */
void BackedUpBaseApp::startNewBackup( uint32 index,
		const MiniBackupHash & hash )
{
	if (!newBackup_.empty())
	{
		WARNING_MSG( "BackedUpBaseApp::startNewBackup: "
				"newBackup_ should've been empty.\n" );
		this->discardNewBackup();
	}

	newBackup_.init( index, hash, currentBackup_ );
	usingNew_ = true;
}


/**
 *	This method switches to a new backup set.
 */
void BackedUpBaseApp::switchToNewBackup()
{
	currentBackup_.swap( newBackup_ );

	MF_ASSERT( currentBackup_.numInvalidEntities() == 0 );

	this->discardNewBackup();

	hasHadCurrentBackup_ = true;
}


/**
 *	This method discards a new backup.
 */
void BackedUpBaseApp::discardNewBackup()
{
	newBackup_.clear();
	usingNew_ = false;
	canSwitchToNewBackup_ = false;
}


/**
 *	This method initialises a set of backed up entities. It populates itself
 *	with data from a previous backup set. Only entities whose hash has not
 *	changed are taken.
 *
 *	@param index 	The index of this backup. Only entities with a id that
 *					hashes to this index are backed up in this set.
 *	@param hash		The hash being used for this backup.
 *	@param current 	The current, up-to-date backup set. Backup data for
 *					entities that also back up to the new hash is copied into
 *					the new backup set from this parameter.
 */
void BackedUpEntitiesWithHash::init( uint32 index,
		const MiniBackupHash & hash,
		const BackedUpEntitiesWithHash & current )
{
	index_ = index;
	hash_ = hash;
	data_.clear(); // Just to be sure

	// We take what current data is useful.
	Container::const_iterator iter = current.data_.begin();

	while (iter != current.data_.end())
	{
		if (hash_.hashFor( iter->first ) == index_)
		{
			data_[ iter->first ] = iter->second;
		}

		++iter;
	}

	if (!data_.empty())
	{
		INFO_MSG( "BackedUpEntities::init: New backup started. "
					"Took %" PRIzu " of %" PRIzu " from current.\n",
				data_.size(), current.data_.size() );
	}
}


/**
 *	This returns the number of entities that are incorrectly mapped to this
 *	collection.
 */
int BackedUpEntitiesWithHash::numInvalidEntities() const
{
	int count = 0;

	Container::const_iterator iter = data_.begin();

	while (iter != data_.end())
	{
		if (hash_.hashFor( iter->first ) != index_)
		{
			++count;
		}

		++iter;
	}

	return count;
}


/**
 *	This static method returns a watcher for this class.
 */
WatcherPtr BackedUpEntitiesWithHash::pWatcher()
{
	static WatcherPtr pWatcher;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();

		BackedUpEntitiesWithHash * pNull = NULL;

		pWatcher->addChild( "numEntities",
				makeWatcher( &BackedUpEntitiesWithHash::size ) );
		pWatcher->addChild( "numInvalids",
				makeWatcher( &BackedUpEntitiesWithHash::numInvalidEntities ) );
		pWatcher->addChild( "hash",
				MiniBackupHash::pWatcher(),
				&pNull->hash_ );
		pWatcher->addChild( "index",
				makeWatcher( &BackedUpEntitiesWithHash::index_ ) );
	}

	return pWatcher;
}


/**
 *	This method restores on the base entities that this object has been backing
 *	up.
 */
void BackedUpBaseApp::restore()
{
	if (usingNew_)
	{
		WARNING_MSG( "BackedUpBaseApp::restore: Restoring while using new.\n" );
		this->discardNewBackup();
	}

	currentBackup_.restore();
}


/**
 *	This method restores base entities onto the local BaseApp that have been
 *	backed up from a remote BaseApp which has recently died.
 */
void BackedUpEntities::restore()
{
	INFO_MSG( "Restoring %" PRIzu " entities\n", data_.size() );

	Container::iterator iter = data_.begin();

	while (iter != data_.end())
	{
		MemoryIStream stream(
			const_cast< char * >( iter->second.data() ), iter->second.size() );

		BaseApp::instance().createBaseFromStream( iter->first, stream );

		++iter;
	}
}


/**
 *	This method handles a BaseApp death by offloading any backed up entity that
 *	we had previously offloaded to that BaseApp to that entity's backup BaseApp
 *	according to the given backup hash chain.
 *
 *	@param deadAddr 			The address of the BaseApp that has died that 
 *								we possibly offloaded to.
 *	@param backedUpHashChain 	The backup hash chain.
 */
void BackedUpEntities::restoreRemotely( const Mercury::Address & deadAddr, 
		const BackupHashChain & backedUpHashChain )
{
	INFO_MSG( "Offloading %" PRIzu " restored entities\n", data_.size() );

	Mercury::BundleSendingMap bundles( BaseApp::instance().intInterface() );

	Container::iterator iter = data_.begin();

	while (iter != data_.end())
	{
		const EntityID & id = iter->first;
		BW::string & backupData = iter->second;

		MemoryIStream stream( const_cast< char * >( backupData.data() ), 
			backupData.size() );
		Mercury::Address destBaseAppAddr = backedUpHashChain.addressFor( 
			deadAddr, id );

		Mercury::Bundle & bundle = bundles[destBaseAppAddr];
		bundle.startMessage( BaseAppIntInterface::backupBaseEntity );

		bundle << true; 		// isOffload
		bundle << id;
		bundle.transfer( stream, stream.remainingLength() );

		++iter;
	}
}


/**
 *	This method creates a watcher that can inspect instances of this class.
 */
/*static*/ WatcherPtr BackedUpBaseApp::pWatcher()
{
	DirectoryWatcherPtr pWatcher = new DirectoryWatcher();

	BackedUpBaseApp * pNull = NULL;

	pWatcher->addChild( "isUsingNew",
			makeWatcher( &BackedUpBaseApp::usingNew_ ) );

	pWatcher->addChild( "numInCurrentBackup",
			makeWatcher( &BackedUpBaseApp::numInCurrentBackup ) );

	pWatcher->addChild( "numInNewBackup",
			makeWatcher( &BackedUpBaseApp::numInNewBackup ) );

	pWatcher->addChild( "currentBackup",
			BackedUpEntitiesWithHash::pWatcher(),
			&pNull->currentBackup_ );

	pWatcher->addChild( "newBackup",
			BackedUpEntitiesWithHash::pWatcher(),
			&pNull->newBackup_ );

	return pWatcher;
}

BW_END_NAMESPACE

// backed_up_base_app.cpp
