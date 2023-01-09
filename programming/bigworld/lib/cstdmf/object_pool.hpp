#ifndef OBJECT_POOL_HPP
#define OBJECT_POOL_HPP

#include "stdmf.hpp"
#include "bw_vector.hpp"
#include "debug.hpp"
#include "cstdmf/lookup_table.hpp"

BW_BEGIN_NAMESPACE

/**
 * This class is used to define a Handle that can be used to 
 * assign access to a resource. It has template parameters
 * to control the maximum number of handles that can be
 * assigned at any one time (indexBits) and the the number
 * of times a handle can be reused before wrapping the
 * handle ids back around. 
 *
 * Generation numbers allow the same resource to be 
 * assigned and released and assigned again without
 * giving away the same handleID each time. This
 * allows previously released handles to be detected when
 * they are used to access a resource and assert.
 */
template <size_t indexBits, size_t generationBits = (32 - indexBits), 
	typename baseType = uint32>
struct Handle
{
	static const baseType INVALID_INDEX = ((1 << indexBits) - 1);
	typedef baseType base;
	typedef Handle<indexBits, generationBits, baseType> SelfType;

	static const baseType INDEX_MASK = ((1 << indexBits) - 1);
	static const baseType GENERATION_SHIFT = (indexBits);
	static const baseType GENERATION_BITS = (generationBits);

	Handle()
		: index_( INVALID_INDEX)
		, generation_( 0 )
	{

	}

	Handle( baseType raw )
		: index_( raw & INDEX_MASK )
		, generation_( raw >> GENERATION_SHIFT )
	{

	}

	Handle( baseType index, baseType generation )
		: index_( index )
		, generation_( generation )
	{

	}

	baseType index_ : indexBits;
	baseType generation_ : generationBits;

	bool isValid()
	{
		return index_ != INVALID_INDEX;
	}

	bool operator == (const SelfType& other) const
	{
		// Check if the indexes match
		if (index_ != other.index_)
			return false;
		
		// If they do match, and they're invalid then same
		if (index_ == INVALID_INDEX)
			return true;
			
		// Otherwise we need to check generation too
		if (generation_ != other.generation_)
			return false;

		return true;
	}

	bool operator != (const SelfType& other) const
	{
		return !(*this == other);
	}

	operator baseType()
	{
		return index_ | (generation_ << GENERATION_SHIFT);
	}

	void invalidate()
	{
		*this = INVALID();
	}

	static SelfType INVALID()
	{
		return SelfType();
	}
};

/// The default handle size allows 2^16 simultaneous handles to be 
/// allocated.
typedef Handle<16, 16> DefaultHandle;

/**
 * This class is used to define a pool of handles and manage the 
 * set of data that they are currently being used to represent.
 * Upon create() a handle is returned that was unused so that
 * it may be assigned a resource and accessed externally.
 * Handles can be updated with set() and retreived with get().
 * 
 * Handles must be castable to uintptr_t. This allows ObjectTypes
 * of integral types and pointer types.
 */
template <typename HandleType = DefaultHandle>
class HandleTable
{
public:
	typedef HandleType Handle;
	typedef HandleTable<HandleType> SelfType;

	static const size_t INVALID_INDEX = Handle::INVALID_INDEX;
	static const size_t DEFAULT_GROW_BY = 50;

	HandleTable()
		: firstFree_( INVALID_INDEX )
		, lastFree_( INVALID_INDEX )
		, growBy_( DEFAULT_GROW_BY )
	{
	}

	HandleTable( size_t initialSize, size_t growBy = DEFAULT_GROW_BY )
		: firstFree_( INVALID_INDEX )
		, lastFree_( INVALID_INDEX )
		, growBy_( growBy )
	{
		reserve( initialSize );
	}

	size_t capacity() const
	{
		return entries.capacity();
	}

	size_t size() const
	{
		return entries_.size();
	}

	void reserve( size_t numObjects )
	{
		if (numObjects < capacity())
		{
			// Involves lowering our capacity
			return;
		}

		// Then grow by this amount
		grow( numObjects - size() );
	}

	void grow( size_t numObjects )
	{
		// Add on N more objects to the pool and initialize them
		size_t oldSize = entries_.size();
		size_t newSize = oldSize + numObjects;

		// Make sure the new size is still indexable
		// If this assertion is hit, then the number of entries
		// indexable by the handle type is too low. Increase its
		// number of bits assigned to the index to allow more 
		// concurrent objects alive at a time.
		MF_ASSERT( newSize < INVALID_INDEX );

		// Resize
		entries_.resize( newSize );

		// Go through all the new entries and push them onto the 
		// free list
		for (size_t index = oldSize; index < newSize; ++index)
		{
			freeListPush( index );
		}
	}

	HandleType create()
	{
		// Check if we have any more entries to give away?
		if (firstFree_ == INVALID_INDEX)
		{
			grow( growBy_ ); 
		}
		// Unable to grow!
		MF_ASSERT( firstFree_ != INVALID_INDEX );

		size_t entryIndex = freeListPop();
		PoolEntry& entry = entries_[entryIndex];

		// Return the free entry
		return HandleType(
			(typename HandleType::base)entryIndex,
			(typename HandleType::base)entry.generation_ );
	}

	void release( const HandleType& handle )
	{
		MF_ASSERT( isValid( handle ) );
		
		// Increment the object's generation
		PoolEntry& entry = entries_[handle.index_];
		// Wrap the generation back around if it exceeds the amount available
		// in the handle type
		const HandleType::base GENERATION_MASK = 
			(1 << HandleType::GENERATION_BITS) - 1;
		entry.generation_ = (entry.generation_ + 1) & GENERATION_MASK;

		// Push the entry onto the freelist
		freeListPush( handle.index_ );
	}

	bool isValid( const HandleType& handle ) const
	{
		// Check that the index in the handle is valid
		if (handle.index_ >= entries_.size())
			return false;

		// Check that the generation in that position matches
		if (handle.generation_ != entries_[handle.index_].generation_)
			return false;

		return true;
	}

protected:

	void freeListPush( size_t index )
	{
		// FIFO queue, add onto the back
		if (lastFree_ != INVALID_INDEX)
		{
			// Update the current last index:
			entries_[lastFree_].nextFree_ = index;
		}
		lastFree_ = index;
		entries_[lastFree_].nextFree_ = INVALID_INDEX;
		
		// Did we have any free previously?
		if (firstFree_ == INVALID_INDEX)
		{
			firstFree_ = lastFree_;
		}
	}

	size_t freeListPop()
	{
		MF_ASSERT( firstFree_ != INVALID_INDEX );

		// FIFO queue, remove from front
		size_t result = firstFree_;

		// Update the first free entry
		firstFree_ = entries_[result].nextFree_;

		// We just removed the last free entry
		if (firstFree_ == INVALID_INDEX)
		{
			lastFree_ = INVALID_INDEX;
		}

		return result;
	}

	struct PoolEntry
	{
		PoolEntry()
			: generation_( 0 )
			, nextFree_( INVALID_INDEX )
		{
		}

		size_t generation_;
		size_t nextFree_;
	};
	typedef BW::vector<PoolEntry> EntryContainer;

	EntryContainer entries_;
	size_t firstFree_;
	size_t lastFree_;
	size_t growBy_;
};

/**
 * This class is used to store a contiguous set of POD objects that 
 * can be iterated over. Updating specific objects is done through
 * a handle interface that allows the handles to be stored externally
 * and their corresponding data to be moved around in memory by the
 * packed pool.
 */
template <typename ObjectType, typename HandleType = DefaultHandle, 
	class ObjectContainer = BW::vector<ObjectType> >
class PackedObjectPool
{
public:
	static const size_t DEFAULT_GROW_BY = 5;

	typedef HandleType Handle;
	typedef ObjectType Object;
	typedef ObjectContainer Container;
	typedef PackedObjectPool<ObjectType, HandleType, ObjectContainer> SelfType;
	typedef typename ObjectContainer::iterator iterator;
	typedef typename ObjectContainer::const_iterator const_iterator;

	PackedObjectPool()
	{
	}

	PackedObjectPool( size_t initialSize, size_t growBy = DEFAULT_GROW_BY )
		: handles_( initialSize, growBy )
		, outerLookup_( initialSize )
		, innerLookup_( initialSize )
		, innerToOuter_( initialSize )
	{
	}
	
	size_t capacity() const
	{
		return innerLookup_.capacity();
	}

	size_t size() const
	{
		return innerLookup_.size();
	}

	void reserve( size_t numObjects )
	{
		handles_.reserve( numObjects );
		innerToOuter_.reserve( numObjects );
		innerLookup_.reserve( numObjects );
		outerLookup_.reserve( numObjects );
	}

	HandleType create()
	{
		innerLookup_.push_back( ObjectType() );
		HandleType handle = handles_.create();
		outerLookup_[handle.index_] = innerLookup_.size() - 1;
		
		// Assign the new index of this handle
		innerToOuter_.push_back( handle.index_ );

		// Make sure the inner arrays stay in sync
		MF_ASSERT( innerLookup_.size() == innerToOuter_.size() );

		return handle;
	}

	void release( const HandleType& handle )
	{
		MF_ASSERT( isValid( handle ) );
		size_t objIndex = outerLookup_[handle.index_];

		// Swap this entry with the back
		innerLookup_[objIndex] = innerLookup_.back();
				
		// Swap the outer indexes too (because the back entry has moved)
		Handle backHandle( 
			(typename Handle::base)innerToOuter_[innerToOuter_.size() - 1],
			(typename Handle::base)0 );
		outerLookup_[backHandle.index_] = objIndex;
		innerToOuter_[objIndex] = backHandle.index_;
		
		// Now shrink the two inner arrays
		innerLookup_.pop_back();
		innerToOuter_.pop_back();

		// Make sure the inner arrays stay in sync
		MF_ASSERT( innerLookup_.size() == innerToOuter_.size() );

		handles_.release( handle );
	}

	ObjectType& lookup( const HandleType& handle )
	{
		MF_ASSERT( isValid( handle ) );
		
		size_t objIndex = outerLookup_[handle.index_];
		return innerLookup_[objIndex];
	}

	const ObjectType& lookup( const HandleType& handle ) const
	{
		return (*const_cast<SelfType*>(this)).lookup(handle);
	}

	ObjectType& operator[] (const HandleType& handle)
	{
		return lookup( handle );
	}

	const ObjectType& operator[] (const HandleType& handle) const
	{
		return lookup( handle );
	}

	bool isValid( const HandleType& handle ) const
	{
		return handles_.isValid( handle );
	}

	iterator begin()
	{
		return innerLookup_.begin();
	}

	iterator end()
	{
		return innerLookup_.end();
	}

	const_iterator begin() const
	{
		return innerLookup_.begin();
	}

	const_iterator end() const
	{
		return innerLookup_.end();
	}

	ObjectContainer& container()
	{
		return innerLookup_;
	}

protected:
	HandleTable<HandleType> handles_;
	BW::LookUpTable<size_t> outerLookup_;
	BW::vector<size_t> innerToOuter_;
	ObjectContainer innerLookup_;
};

BW_END_NAMESPACE

#endif // OBJECT_POOL_HPP
