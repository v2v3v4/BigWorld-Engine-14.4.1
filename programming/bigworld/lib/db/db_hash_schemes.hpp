#ifndef DB_HASH_SCHEMES_HPP
#define DB_HASH_SCHEMES_HPP

#include "network/basictypes.hpp"

#include "server/rendezvous_hash_scheme.hpp"

#include <string.h>


BW_BEGIN_NAMESPACE

namespace DBHashSchemes
{

/*
 * To get the hash scheme class, use e.g. 
 * 		typedef DBHashSchemes::DBAppIDBuckets< Mercury::Address >::HashScheme
 * 			MyHashScheme;
 * 
 * 		MyHashScheme hashScheme;
 * 		hashScheme.insert( 1, Mercury::Address( ... ) );
 * 		Mercury::Address addr = hashScheme[DBID];
 */

// Available schemes:
template< typename MAPPED_TYPE >
class DBAppIDBuckets;

template< typename MAPPED_TYPE >
class StringBuckets;


uint64 hashFunction( void * data, size_t size );


/**
 *	This function populates the given buffer with the bytes of the given
 *	integer value in big-endian order.
 */
// TODO: Scalable DB: Is there a macro that does this already somewhere?
template< typename INT >
inline void integerToBigEndian( unsigned char * buf, INT value )
{
	// Probably not a big deal, since for the Scalable DB work, we know we're
	// targeting Intel arch which supports unaligned memory accesses. But in
	// case that changes or this is used on clients, we make it
	// platform-neutral here.

	for (size_t i = 0; i < sizeof(INT); ++i)
	{
		const size_t shiftBits = (sizeof(INT) - 1 - i) * 8;
		*(buf++) = (value >> shiftBits) & 0xFF;
	}
}


/**
 *	This class is used to hash shard names and entity database IDs together for
 *	the Rendezvous Hash.
 */
template< typename MAPPED_TYPE >
class StringBuckets
{
public:
	// DB hash schemes that use this template class are of the following form.
	typedef RendezvousHashSchemeT< DatabaseID, BW::string, MAPPED_TYPE,
			StringBuckets< MAPPED_TYPE > >
		HashScheme;

	// This is used by RendezvousHashSchemeT.
	typedef uint64 Value;

	// This is the number of buffer space to pre-allocate. This will be
	// increased on-demand from the BW::vector implementation.
	static const size_t DEFAULT_BUFFER_SIZE = 128;

	/**
	 *	Constructor.
	 */
	StringBuckets() :
			hashBuffer_()
	{
		hashBuffer_.reserve( DEFAULT_BUFFER_SIZE );
	}


	/**
	 *	Copy constructor.
	 */
	StringBuckets( const StringBuckets & other ) :
			hashBuffer_()
	{
		hashBuffer_.reserve( DEFAULT_BUFFER_SIZE );

		// Don't bother copying the buffer contents.
		// hashBuffer_ = other.hashBuffer_;
	}


	/**
	 *	This implements the bucket and key hashing used with rendezvous
	 *	hashing.
	 *
	 *	@param name 		The shard name.
	 *	@param dbID 		The database ID.
	 */
	Value hash( const BW::string & name, DatabaseID dbID ) const
	{
		const size_t dataLength = sizeof( DatabaseID ) + name.size();

		if (hashBuffer_.size() < dataLength)
		{
			hashBuffer_.resize( dataLength );
		}

		unsigned char * buf = &(hashBuffer_.front());

		memcpy( buf, name.data(), name.size() );
		integerToBigEndian( buf + name.size(), dbID );

		Value value = hashFunction( &(hashBuffer_.front()), dataLength );

		return value;
	}

	// Pre-allocated to avoid allocations each time hash() is called.
	mutable BW::vector< uint8 > hashBuffer_; 	
};


/**
 *	This class is used to hash DBApp IDs and entity database IDs together for
 *	the Rendezvous Hash.
 */
template< typename MAPPED_TYPE >
class DBAppIDBuckets
{
public:
	// DB hash schemes that use this template class are of the following form.
	typedef RendezvousHashSchemeT< DatabaseID, DBAppID, MAPPED_TYPE,
			DBAppIDBuckets< MAPPED_TYPE > >
		HashScheme;
	// This is used by RendezvousHashSchemeT.
	typedef uint64 Value;

	/**
	 *	This implements the bucket and key hashing used with rendezvous
	 *	hashing.
	 *
	 *	@param name 		The shard name.
	 *	@param dbID 		The database ID.
	 */
	Value hash( DBAppID appID, DatabaseID dbID ) const
	{
		unsigned char buf[ sizeof( DBAppID ) + sizeof( DatabaseID ) ];

		integerToBigEndian( buf, appID );
		integerToBigEndian( buf + sizeof( DBAppID ), dbID );

		return hashFunction( buf, sizeof( buf ) );
	}
};


} // end namespace DBHashSchemes


BW_END_NAMESPACE


#endif // DB_HASH_SCHEMES_HPP
