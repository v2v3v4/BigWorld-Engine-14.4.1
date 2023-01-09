#ifndef BACKED_UP_BASE_APP_HPP
#define BACKED_UP_BASE_APP_HPP

#include "cstdmf/stdmf.hpp"
#include "server/backup_hash_chain.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/watcher.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used for storing information about BaseApps that are being
 *	backed up by this BaseApp.
 */

class BackedUpEntities
{
public:
	BW::string & getDataFor( EntityID entityID )
	{
		return data_[ entityID ];
	}

	bool contains( EntityID entityID ) const
	{
		return data_.count( entityID ) != 0;
	}

	bool erase( EntityID entityID )
	{
		return data_.erase( entityID );
	}

	void clear()
	{
		data_.clear();
	}

	void restore();

	bool empty() const	{ return data_.empty(); }

	size_t size() const	{ return data_.size(); }

	void restoreRemotely( const Mercury::Address & deadAddr, 
		  const BackupHashChain & backedUpHashChain );

protected:
	typedef	BW::map< EntityID, BW::string > Container;
	Container data_;
};


class BackedUpEntitiesWithHash : public BackedUpEntities
{
public:
	void init( uint32 index, const MiniBackupHash & hash,
			   const BackedUpEntitiesWithHash & current );

	void swap( BackedUpEntitiesWithHash & other )
	{
		uint32 tempInt = index_;
		index_ = other.index_;
		other.index_ = tempInt;

		MiniBackupHash tempHash = hash_;
		hash_ = other.hash_;
		other.hash_ = tempHash;

		data_.swap( other.data_ );
	}

	int numInvalidEntities() const;

	static WatcherPtr pWatcher();

private:
	uint32 index_;
	MiniBackupHash hash_;
};


class BackedUpBaseApp
{
public:
	BackedUpBaseApp() : 
		usingNew_( false ),
		canSwitchToNewBackup_( false ),
		hasHadCurrentBackup_( false )
	{}

	void startNewBackup( uint32 index, const MiniBackupHash & hash );

	BW::string & getDataFor( EntityID entityID )
	{
		if (usingNew_)
			return newBackup_.getDataFor( entityID );
		else
			return currentBackup_.getDataFor( entityID );
	}

	bool hasHadCurrentBackup() const
	{
		return hasHadCurrentBackup_;
	}

	bool erase( EntityID entityID )
	{
		if (usingNew_)
			return newBackup_.erase( entityID );
		else
			return currentBackup_.erase( entityID );
	}

	void switchToNewBackup();
	void discardNewBackup();

	bool canSwitchToNewBackup() const 
		{ return canSwitchToNewBackup_; }

	void canSwitchToNewBackup( bool value ) 
		{ canSwitchToNewBackup_ = value; }

	void restore();

	static WatcherPtr pWatcher();

	size_t numInCurrentBackup() const { return currentBackup_.size(); }
	size_t numInNewBackup() const { return newBackup_.size(); }

private:
	BackedUpEntitiesWithHash currentBackup_; // Up-to-date backup.
	BackedUpEntitiesWithHash newBackup_;	// Backup we're trying to switch to.
	bool usingNew_;
	bool canSwitchToNewBackup_;
	bool hasHadCurrentBackup_;				// whether at least one entity 
											// has been backed up to current 
											// since our creation
};

BW_END_NAMESPACE

#endif // BACKED_UP_BASE_APP_HPP
