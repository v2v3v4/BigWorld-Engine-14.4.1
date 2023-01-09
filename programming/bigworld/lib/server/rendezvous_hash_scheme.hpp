#ifndef RENDEZVOUS_HASH_SCHEME_HPP
#define RENDEZVOUS_HASH_SCHEME_HPP

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This template class implements the Rendezvous Hashing scheme.
 *
 *	Refer to http://www.eecs.umich.edu/techreports/cse/96/CSE-TR-316-96.pdf for
 *	details.
 *
 *	The basic idea is that additional to a given key, an identifier for each
 *	bucket is also used as the input to the hash function that outputting the
 *	hash value for each bucket. The bucket with the highest value is chosen as
 *	the bucket for the given key.
 *
 *	An important property of this scheme is when a bucket is removed, only the
 *	minimum set of keys need to be remapped to the other surviving buckets.
 *
 *	This template class is to be used in conjunction with a hash function
 *	implementation class supplied as the HASH_ALGORITHM template argument.
 *
 *	With a good hash function implementation, each bucket should have an equal
 *	share of the keyspace.
 *
 *	Since the hash function needs to be evaluated for each bucket to determine
 *	the highest hash function output value, this is most suited for
 *	applications where the number of buckets is relatively low.
 *
 *	The requirements for the HASH_ALGORITHM class are:
 *
 *	1. 	It needs to be defaultly-constructible.
 *	
 *	2. 	It needs to implement:
 *
 *			Value hash( const Bucket & bucket, const Object & object ) const;
 *
 *	3. 	The hash function output type is required to be typedef'd as
 *		HASH_ALGORITHM::Value.
 *
 *	4.	They should be more or less stateless, and two instances of the same
 *		algorithm class should be functionally equivalent. Data can be cached
 *		for each instance (e.g. allocating some scratch memory to avoid
 *		allocations each time it is invoked), but shouldn't be expected to be
 *		part of the hash's data with respect to equality and copying.
 */
template< typename KEY, typename BUCKET, typename MAPPED_TYPE,
		typename HASH_ALGORITHM, 
		typename BUCKET_MAP = BW::map< BUCKET, MAPPED_TYPE > >
class RendezvousHashSchemeT
{
public:
	typedef KEY Key;
	typedef BUCKET Bucket;
	typedef MAPPED_TYPE MappedType;
	typedef typename BUCKET_MAP::value_type ValueType;
	typedef typename HASH_ALGORITHM::Value HashValue;
	typedef HASH_ALGORITHM HashAlgorithm;
	typedef BUCKET_MAP BucketMap;
	typedef typename BucketMap::iterator iterator;
	typedef typename BucketMap::const_iterator const_iterator;

	// For MapWatcher
	typedef typename BucketMap::key_type key_type;
	typedef typename BucketMap::mapped_type mapped_type;

	/**
	 *	This template structure holds static methods for streaming maps.
	 */
	struct MapStreaming
	{
		/**
		 *	This static method adds the given map of the bucket mapped-value
		 *	pairs to a given stream.
		 *
		 *	This can be specialised to provide special streaming, e.g. if the
		 *	map is a map with ChannelOwner mapped values, and you want to
		 *	stream just the address.
		 *
		 *	This is called to stream out the RendezvousHashSchemeT::buckets_
		 *	member.
		 *
		 *	@param os 	The output stream.
		 *	@param map	The map object to stream.
		 */
		static void addToStream( BinaryOStream & os, const BUCKET_MAP & map )
		{
			os.writePackedInt( static_cast< int >( map.size() ) );
			for (typename BUCKET_MAP::const_iterator iter = map.begin();
					iter != map.end();
					++iter)
			{
				os << iter->first << iter->second;
			}
		}


		/**
		 *	This static method reads the given map of the bucket mapped-value
		 *	pairs from a given stream.
		 *
		 *	This is called to stream in the RendezvousHashSchemeT::buckets_
		 *	member.
		 *
		 *	@param is 	The input stream.
		 *	@param map	The map object to stream.
		 *
		 *	@return 	true if successful, false if a stream error occurred.
		 */
		static bool readFromStream( BinaryIStream & is, BUCKET_MAP & map )
		{
			map.clear();

			int size = is.readPackedInt();
			while ((size-- > 0) && !is.error())
			{
				typename BUCKET_MAP::key_type key;
				typename BUCKET_MAP::mapped_type mappedValue;
				is >> key >> mappedValue;
				map.insert( 
					typename BUCKET_MAP::value_type( key, mappedValue ) );
			}

			return !is.error();
		}
	};


	/**
	 *	Constructor.
	 *
	 *	@param hash 	A specific hash algorithm instance to use, otherwise,
	 *					one is created using the default constructor.
	 */
	RendezvousHashSchemeT( const HashAlgorithm & hash = HashAlgorithm() ) :
		buckets_(),
		hash_( hash )
	{}

	
	/**
	 *	Constructor taking a range.
	 *
	 *	@param begin 	The start of the range.
	 *	@param end 		The end of the range.
	 *	@param hash 	A specific hash algorithm instance to use, otherwise,
	 *					one is created using the default constructor.
	 */
	template< typename INPUT_ITERATOR >
	RendezvousHashSchemeT( INPUT_ITERATOR begin, INPUT_ITERATOR end,
			const HashAlgorithm & hash = HashAlgorithm() ) :
		buckets_(),
		hash_( hash )
	{
		buckets_.insert( begin, end );
	}


	// Default copy and assignment constructor suffices for this class.
	// RendezvousHashSchemeT( const RendezvousHashSchemeT & other );
	// RendezvousHashSchemeT & operator=( const RendezvousHashSchemeT & other );


	/**
	 *	Destructor.
	 */
	~RendezvousHashSchemeT() {}


	/** This method clears the collection. */
	void clear() { buckets_.clear(); }


	/** This method returns true if the collection is empty, false otherwise. */
	bool empty() const { return buckets_.empty(); }


	/**
	 *	This method adds a new bucket-value pair.
	 *
	 *	@param value 	The new bucket-value pair.
	 *
	 *	@return 		A pair with an iterator pointing to the inserted
	 *					element and a bool to show whether a new element
	 *					is inserted or not.
	 */
	std::pair< iterator, bool > insert( const ValueType & value )
	{
		return buckets_.insert( value );
	}


	/**
	 *	This method adds a new bucket-value pair.
	 *
	 *	@param position The position hint where the element can be inserted.
	 *	@param value 	The new bucket-value pair.
	 *
	 *	@return 		An iterator pointing to the inserted element.
	 */
	iterator insert( iterator position, const ValueType & value )
	{
		return buckets_.insert( position, value );
	}


	/**
	 *	This method adds an iterator range of new bucket-value pairs.
	 *
	 *	@param begin 	The start of the range.
	 *	@param end 		The end of the range.
	 *
	 *	@return 		The number of bucket-value pairs added.
	 */
	template< typename INPUT_ITERATOR >
	size_t insert( INPUT_ITERATOR begin, INPUT_ITERATOR end )
	{
		size_t oldSize = buckets_.size();
		buckets_.insert( begin, end );
		return buckets_.size() - oldSize;
	}


	/**
	 *	This method removes a bucket-value pair.
	 *
	 *	@param bucket 	The bucket to remove.
	 *
	 *	@return 		The number of elements erased.
	 */
	size_t erase( const Bucket & bucket )
	{
		return buckets_.erase( bucket );
	}


	/**
	 *	This method retrieves the bucket-value pair based on the input key. 
	 * 
	 *	@param key		The key to search.
	 *
	 *	@return 		The bucket-value pair.
	 */
	ValueType & valueForKey( const Key & key )
	{
		HashValue max = 0;
		iterator ret = buckets_.begin();

		for (iterator iter = buckets_.begin(); 
				iter != buckets_.end(); 
				++iter)
		{
			HashValue value = hash_.hash( iter->first, key );
			if (value > max)
			{
				max = value;
				ret = iter;
			}
		}

		return *ret;
	}


	/**
	 *	This method retrieves the bucket-value pair based on the input key
	 *	(const version).
	 * 
	 *	@param key		The key to search.
	 *
	 *	@return 		The bucket-value pair.
	 */
	const ValueType & valueForKey( const Key & key ) const
	{
		RendezvousHashSchemeT * pNonConstThis = 
			const_cast< RendezvousHashSchemeT * >( this );
		return pNonConstThis->valueForKey( key );
	}


	/**
	 *	This method calculates the bucket based on the input key. 
	 * 
	 *	@param key		The key to search.
	 *
	 *	@return 		The bucket.
	 */
	const Bucket & bucketForKey( const Key & key ) const
	{
		return this->valueForKey( key ).first;
	}


	/**
	 *	This method retrieves the mapped value for a bucket.
	 *
	 *	@param bucket 	The bucket.
	 *
	 *	@return 		The mapped value for that bucket.
	 */
	MappedType & mappedValueForBucket( const Bucket & bucket )
	{
		return buckets_.find( bucket )->second;
	}


	/**
	 *	This method retrieves the mapped value for a bucket (const version).
	 *
	 *	@param bucket 	The bucket.
	 *
	 *	@return 		The mapped value for that bucket.
	 */
	const MappedType & mappedValueForBucket( const Bucket & bucket ) const
	{
		RendezvousHashSchemeT * pNonConstThis = 
			const_cast< RendezvousHashSchemeT * >( this );
		return pNonConstThis->mappedValueForBucket( bucket );
	}


	/**
	 *	This method retrieves the bucket's mapped value, calculated for a given
	 *	key.
	 * 
	 *	@param key		The key to search.
	 *
	 *	@return 		The mapped value.
	 */
	MappedType & mappedValueForKey( const Key & key )
	{
		return this->valueForKey( key ).second;
	}


	/**
	 *	This method retrieves the bucket's mapped value, calculated for a given
	 *	key (const version).
	 * 
	 *	@param key		The key to search.
	 *
	 *	@return 		The mapped value.
	 */
	const MappedType & mappedValueForKey( const Key & key ) const
	{
		return this->valueForKey( key ).second;
	}


	/**
	 *	This method retrieves the bucket's mapped value, calculated for a given
	 *	key.
	 * 
	 *	@param key		The key to search.
	 *
	 *	@return 		The mapped value.
	 */
	MappedType & operator[]( const Key & key ) 
	{
		return this->mappedValueForKey( key );
	}


	/**
	 *	This method retrieves the bucket's mapped value, calculated for a given
	 *	key (const version).
	 * 
	 *	@param key		The key to search.
	 *
	 *	@return 		The mapped value.
	 */
	const MappedType & operator[]( const Key & key ) const
	{
		return this->mappedValueForKey( key );
	}


	/** Equality operator. */
	bool operator==( const RendezvousHashSchemeT & other ) const
	{
		// Hash algorithm objects should be identical within the same class,
		// and should hold no other state. So no need to compare hash_.
		return (buckets_ == other.buckets_);
	}


	/** Inequality operator. */
	bool operator!=( const RendezvousHashSchemeT & other ) const
	{
		// Hash algorithm objects should be identical within the same class,
		// and should hold no other state. So no need to compare hash_.
		return (buckets_ != other.buckets_);
	}


	/**
	 * This method returns the start of the bucket collection range
	 * (const-version).
	 */
	const_iterator begin() const 	{ return buckets_.begin(); }


	/**
	 *	This method returns the end of the bucket collection range
	 *	(const-version). 
	 */
	const_iterator end() const 		{ return buckets_.end(); }


	/** This method returns the start of the bucket collection range. */
	iterator begin() { return buckets_.begin(); }


	/** This method returns the end of the bucket collection range. */
	iterator end() { return buckets_.end(); }


	/**
	 *	This method locates the given bucket in the collection, or returns
	 *	end() if not found.
	 */
	const_iterator find( const Bucket & bucket ) const
	{ 
		return buckets_.find( bucket );
	}


	/**
	 *	This method locates the given bucket in the collection, or returns
	 *	end() if not found.
	 */
	iterator find( const Bucket & bucket )
	{ 
		return buckets_.find( bucket );
	}


	/** This method returns the count of the given bucket in the collection. */
	size_t count( const Bucket & bucket ) const 
	{
		return buckets_.count( bucket );
	}


	/** This method returns the number of buckets in the collection. */
	size_t size() const { return buckets_.size(); }


	/**
	 *	This method writes the hash data to the given output stream.
	 */
	void addToStream( BinaryOStream & os ) const
	{
		MapStreaming::addToStream( os, buckets_ );
	}


	/**
	 *	This method reads the hash data from the given input stream.
	 */
	void readFromStream( BinaryIStream & is )
	{
		MapStreaming::readFromStream( is, buckets_ );
	}


	/**
	 *	This method returns the bucket-value pair with the least value of 
	 *	bucket in the collection (with respect to BUCKET_MAP::key_comp).
	 */
	const ValueType & smallest() const { return *(buckets_.begin()); }

	/**
	 *	This method returns the bucket-value pair with the largest value of 
	 *	bucket in the collection (with respect to BUCKET_MAP::key_comp).
	 */
	const ValueType & largest() const { return *(buckets_.rbegin()); }


private:
	BucketMap 		buckets_;
	HashAlgorithm 	hash_;
};


// For MapWatcher.
template< typename KEY, typename BUCKET, typename MAPPED_TYPE,
		typename HASH_ALGORITHM, typename BUCKET_MAP >
struct MapTypes< RendezvousHashSchemeT< KEY, BUCKET, MAPPED_TYPE,
		HASH_ALGORITHM, BUCKET_MAP > >
{
	typedef MAPPED_TYPE & _Tref;
};


/**
 *	Out-streaming operator for RendezvousHash objects.
 */
template< typename KEY, typename BUCKET, typename MAPPED_TYPE,
		typename HASH_ALGORITHM, typename BUCKET_MAP >
BinaryOStream & operator<<( BinaryOStream & os, 
		const RendezvousHashSchemeT< KEY, BUCKET, MAPPED_TYPE, HASH_ALGORITHM,
			BUCKET_MAP > & hash )
{
	hash.addToStream( os );

	return os;
}


/**
 *	In-streaming operator for RendezvousHash objects.
 */
template< typename KEY, typename BUCKET, typename MAPPED_TYPE,
		typename HASH_ALGORITHM, typename BUCKET_MAP >
BinaryIStream & operator>>( BinaryIStream & is, 
		RendezvousHashSchemeT< KEY, BUCKET, MAPPED_TYPE, HASH_ALGORITHM,
			BUCKET_MAP > & hash )
{
	hash.readFromStream( is );
	return is;
}


BW_END_NAMESPACE


#endif // RENDEZVOUS_HASH_SCHEME_HPP
