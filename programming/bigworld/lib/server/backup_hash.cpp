#include "server/backup_hash.hpp"
#include "cstdmf/debug.hpp"

#include "cstdmf/bw_map.hpp"

DECLARE_DEBUG_COMPONENT2( "Server", 0 );


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: MiniBackupHash
// -----------------------------------------------------------------------------

/**
 *	This method returns the hash associated with the input id.
 */
uint32 MiniBackupHash::hashFor( EntityID id ) const
{
	// The way that the hash works is to multiple the id by a large prime modulo
	// 2**32. The top 24 bits are then taken.
	//
	// This value then needs to be tranlated to a bucket. virtualSize_ is the
	// smallest power of 2 greater than or equal to the number of buckets. If
	// the modulo of the initial hash fits in the number of buckets, this is the
	// value that is returned. If not, the high-order bit is discarded to find
	// the actual bucket to use.
	//
	// This is done like this so that the number of buckets can grow and shrink
	// nicely without too much of the hash changing. If an extra bucket is
	// added, one of the original buckets is split into this new bucket.
	// Something similar occurs when a bucket is removed.

	// NOTE: Please remember to update MySqlDatabase::remapEntityMailboxes()
	// (in bigworld/src/server/dbapp/mysql_database.cpp) if you update this
	// hashing function. It uses some fancy SQL query to achieve the same
	// result as this hashing function.

	if (size_ > 0)
	{
		// The lower bits do not tend to be as randomised.
		uint32 hash = ((uint32(id) * prime_) >> 8) % virtualSize_;

		if (hash >= size_)
		{
			hash -= virtualSize_/2;
		}

		return hash;
	}

	return uint32( -1 );
}


/**
 *	This method returns the virtual size to use when considering a specific
 *	bucket. The virtual size is either the smallest power of 2 greater than or
 *	equal to the number of buckets or half this value.
 */
uint32 MiniBackupHash::virtualSizeFor( uint32 index ) const
{
	// An example:
	// If we have 6 buckets, we take modulo 8 of the original hash since this is
	// the smallest power of 2 greater than 6.
	// We then get the following mapping:
	// 0 -> 0
	// 1 -> 1
	// 2 -> 2
	// 3 -> 3
	// 4 -> 4
	// 5 -> 5
	// 6 -> 2
	// 7 -> 3

	// For buckets 2 and 3, an id is in the bucket if hash % 4 == bucketID.
	// For buckets 0, 1, 4 and 5, an id is in the bucket if hash % 8 == bucketID
	// So the virtual size for 2 and 3 is 4, for 0, 1, 4 and 5, it is 8.

	uint32 miniVirtualSize = virtualSize_/2;
	if ((index < size_ - miniVirtualSize) || (miniVirtualSize <= index))
	{
		return virtualSize_;
	}
	return miniVirtualSize;
}


/**
 *	This method populates a watcher with children suitable for inspecting this.
 */
void MiniBackupHash::populateWatcher( Watcher * pWatcher, bool isImplied )
{
	pWatcher->addChild( "prime", makeWatcher( &MiniBackupHash::prime_ ) );

	if (isImplied)
	{
		pWatcher->addChild( "impliedVirtualSize",
				makeWatcher( &MiniBackupHash::virtualSize_ ) );
	}
	else
	{
		pWatcher->addChild( "size", makeWatcher( &MiniBackupHash::size_ ) );
		pWatcher->addChild( "virtualSize",
				makeWatcher( &MiniBackupHash::virtualSize_ ) );
	}
}


/**
 *	This method returns a watcher for inspecting instances of this class.
 */
WatcherPtr MiniBackupHash::pWatcher()
{
	static WatcherPtr pWatcher;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();
		MiniBackupHash::populateWatcher( pWatcher.get(), /*isImplied*/ true );
	}

	return pWatcher;
}


// -----------------------------------------------------------------------------
// Section: BackupHash
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
BackupHash::BackupHash() : 
		MiniBackupHash( this->choosePrime() ),
		addrs_()
{
	// A random prime is chosen so that the hash functions are different for
	// different BaseApps. If we did not do this, we could get bad distribution
	// once a few BaseApps have died.
}


/**
 *	This method returns the address that the input id hashes to.
 */
Mercury::Address BackupHash::addressFor( EntityID id ) const
{
	if (!addrs_.empty())
	{
		return addrs_[ this->hashFor( id ) ];
	}

	return Mercury::Address( 0, 0 );
}


/**
 *	This method compares to BackupHashes and reports the differences. If a
 *	backup has not been affected by the hash change it does not need to be told
 *	that it needs to update.
 */
void BackupHash::diff( const BackupHash & other, DiffVisitor & visitor )
{
	bool primeChanged = ( other.prime() != this->prime() );

	BW::map< Mercury::Address, uint32 > srcHash;

	for (uint32 i = 0; i < addrs_.size(); ++i)
	{
		srcHash[ addrs_[i] ] = i;
	}

	for (uint32 i = 0; i < other.addrs_.size(); ++i)
	{
		const Mercury::Address & otherAddr = other.addrs_[ i ];
		BW::map< Mercury::Address, uint32 >::iterator findIter =
			srcHash.find( otherAddr );

		if (findIter != srcHash.end())
		{
			if ((i != findIter->second) ||
				(other.virtualSizeFor( i ) != this->virtualSizeFor( i )) ||
				primeChanged)
			{
				visitor.onChange( otherAddr, i,
						other.virtualSizeFor( i ), other.prime() );
			}
			srcHash.erase( findIter );
		}
		else
		{
			visitor.onAdd( otherAddr, i,
					other.virtualSizeFor( i ), other.prime() );
		}
	}

	BW::map< Mercury::Address, uint32 >::iterator mapIter = srcHash.begin();
	while (mapIter != srcHash.end())
	{
		visitor.onRemove( mapIter->first,
						mapIter->second,
						this->virtualSizeFor( mapIter->second ),
						this->prime() );
		++mapIter;
	}
}


/**
 *	This method adds an address to the hash.
 */
void BackupHash::push_back( const Mercury::Address & addr )
{
	addrs_.push_back( addr );
	this->handleSizeChange( addrs_.size() );
}


/**
 *	This method removes an address from the hash. It tries to do this in a way
 *	that has a small impact on the hash.
 */
bool BackupHash::erase( const Mercury::Address & addr )
{
	Container::iterator iter = std::find( addrs_.begin(), addrs_.end(), addr );
	if (iter != addrs_.end())
	{
		*iter = addrs_.back();
		addrs_.pop_back();

		this->handleSizeChange( addrs_.size() );

		return true;
	}

	return false;
}


/**
 *	This method clears an address from the hash.
 */
bool BackupHash::clearAddress( const Mercury::Address & addr )
{
	Container::iterator iter = std::find( addrs_.begin(), addrs_.end(), addr );
	if (iter != addrs_.end())
	{
		*iter = Mercury::Address::NONE;

		return true;
	}

	return false;
}


/**
 *	This method chooses a prime randomly. This is used to create a random hash
 *	function so that each BaseApp hash is not identical. If they were identical,
 *	degenerate allocations could occur to BaseApps after a few restores.
 */
uint32 BackupHash::choosePrime()
{
	static const uint32 primes[] =
	{
		// It's probably not good to have the primes all so close together but
		// we're not relying on this much.
		0x9e377e55, 0x9e377e53, 0x9e377e43, 0x9e377e41, 0x9e377e37,
		0x9e377e11, 0x9e377e07, 0x9e377de1, 0x9e377db7, 0x9e377da5,
		0x9e377d99, 0x9e377d8f, 0x9e377d8d, 0x9e377d81, 0x9e377d65,
		0x9e377d5f, 0x9e377d53, 0x9e377d47, 0x9e377d21, 0x9e377d03,
		0x9e377cff, 0x9e377cd3, 0x9e377c97, 0x9e377c75, 0x9e377c6d,
		0x9e377c3d, 0x9e377c33, 0x9e377c01, 0x9e377bcb, 0x9e377bbb,
		0x9e377b91, 0x9e377b85, 0x9e377b55, 0x9e377b0b, 0x9e377ae9,
		0x9e377ae3, 0x9e377aab, 0x9e377aa7, 0x9e377a9f, 0x9e377a6f,
		0x9e377a5d, 0x9e377a45, 0x9e377a0b, 0x9e3779f3, 0x9e3779ed,
		0x9e3779e5, 0x9e3779e1, 0x9e3779db, 0x9e3779cd, 0x9e3779b1,
	};

	return primes[ rand() % (sizeof( primes )/sizeof( primes[0])) ];
}


/**
 *	This method returns a watcher for inspecting instances of this class.
 */
WatcherPtr BackupHash::pWatcher()
{
	static WatcherPtr pWatcher;

	if (pWatcher == NULL)
	{
		BackupHash * pNull = NULL;

		pWatcher = new DirectoryWatcher();
		MiniBackupHash::populateWatcher( pWatcher.get(), /*isImplied*/ false );

		Watcher * pAddresses =
			new SequenceWatcher< Container >( pNull->addrs_ );
		pAddresses->addChild( "*", &Mercury::Address::watcher() );

		pWatcher->addChild( "addresses", pAddresses );
	}

	return pWatcher;
}

BW_END_NAMESPACE

// backup_hash.cpp
