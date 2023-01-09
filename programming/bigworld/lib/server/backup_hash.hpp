#ifndef BACKUP_HASH_HPP
#define BACKUP_HASH_HPP

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/watcher.hpp"

#include "network/basictypes.hpp"

#include <stdlib.h>


BW_BEGIN_NAMESPACE

/**
 *	This class is used to hash an object ID to an integer. The hash function
 *	handles changing hash sizes nicely. It attempts to keep most values hashing
 *	to the same bucket.
 */
class MiniBackupHash
{
public:
	MiniBackupHash( uint32 prime = 0, uint32 size = 0 ) :
		prime_( prime ),
		size_( size ),
		virtualSize_( 0 )
	{
		this->handleSizeChange( size_ );
	};
	~MiniBackupHash() {};

	uint32 prime() const		{ return prime_; }
	uint32 size() const			{ return size_; }
	uint32 virtualSize() const	{ return virtualSize_; }

	uint32 hashFor( EntityID id ) const;
	uint32 virtualSizeFor( uint32 index ) const;

	static WatcherPtr pWatcher();

protected:
	void handleSizeChange( uint32 newSize )
	{
		size_ = newSize;
		if (size_ > 0)
		{
			virtualSize_ = 1;
			while (virtualSize_ < size_)
			{
				virtualSize_ *= 2;
			}
		}
		else
		{
			virtualSize_ = 0;
		}
	}

protected:
	uint32 prime_;

	static void populateWatcher( Watcher * pWatcher, bool isImplied );

private:
	uint32 size_;
	uint32 virtualSize_;

	friend BinaryOStream &
		operator<<( BinaryOStream & b, const MiniBackupHash & hash );
	friend BinaryIStream & operator>>( BinaryIStream & b,
			MiniBackupHash & hash );
};


/**
 *	This class is used to hash an entity id to an address of a BaseApp.
 */
class BackupHash : public MiniBackupHash
{
public:
	// A random prime is chosen so that the hash functions are different for
	// different BaseApps. If we did not do this, we could get bad distribution
	// once a few BaseApps have died.
	BackupHash();

	Mercury::Address addressFor( EntityID id ) const;

	/**
	 * TODO: to be documented.
	 */
	class DiffVisitor
	{
	public:
		virtual ~DiffVisitor() {};
		virtual void onAdd( const Mercury::Address & addr,
				uint32 index, uint32 virtualSize, uint32 prime ) = 0;
		virtual void onChange( const Mercury::Address & addr,
				uint32 index, uint32 virtualSize, uint32 prime ) = 0;
		virtual void onRemove( const Mercury::Address & addr,
				uint32 index, uint32 virtualSize, uint32 prime ) = 0;
	};

	void diff( const BackupHash & other, DiffVisitor & visitor );

	void clear()
	{
		addrs_.clear();
		this->handleSizeChange( 0 );
	}

	bool empty() const	{ return addrs_.empty(); }
	const Mercury::Address & operator[]( const int index ) const
	{
		return addrs_[ index ];
	}

	void swap( BackupHash & other )
	{
		this->addrs_.swap( other.addrs_ );
		uint32 temp = prime_;
		prime_ = other.prime_;
		other.prime_ = temp;

		this->handleSizeChange( this->addrs_.size() );
		other.handleSizeChange( other.addrs_.size() );
	}

	void push_back( const Mercury::Address & addr );

	bool erase( const Mercury::Address & addr );
	bool clearAddress( const Mercury::Address & addr );

	static WatcherPtr pWatcher();

private:
	static uint32 choosePrime();

	typedef BW::vector< Mercury::Address > Container;
	Container addrs_;

	friend BinaryOStream &
		operator<<( BinaryOStream & b, const BackupHash & hash );
	friend BinaryIStream & operator>>( BinaryIStream & b, BackupHash & hash );
};


// -----------------------------------------------------------------------------
// Section: Streaming methods.
// -----------------------------------------------------------------------------

inline BinaryOStream & operator<<( BinaryOStream & b,
		const MiniBackupHash & hash )
{
	b << hash.size_ << hash.prime_;
	return b;
}

inline BinaryIStream & operator>>( BinaryIStream & b, MiniBackupHash & hash )
{
	b >> hash.size_ >> hash.prime_;
	hash.handleSizeChange( hash.size_ );
	return b;
}


inline BinaryOStream & operator<<( BinaryOStream & b, const BackupHash & hash )
{
	b << hash.prime_ << hash.addrs_;
	return b;
}

inline BinaryIStream & operator>>( BinaryIStream & b, BackupHash & hash )
{
	b >> hash.prime_ >> hash.addrs_;
	hash.handleSizeChange( hash.addrs_.size() );
	return b;
}

BW_END_NAMESPACE

#endif // BACKUP_HASH_HPP
