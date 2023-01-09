#include "pch.hpp"
#include "fixed_sized_allocator.hpp"

#include "allocator.hpp"
#include "dprintf.hpp"

// Enable this to place guards at the end of each block that get check on
// allocation and deallocation
#ifdef _DEBUG
#define ENABLE_FIXED_SIZE_POOL_BOUNDS_CHECKING 1
#else
#define ENABLE_FIXED_SIZE_POOL_BOUNDS_CHECKING 0
#endif

#if ENABLE_FIXED_SIZE_POOL_BOUNDS_CHECKING
namespace {
	const BW_NAMESPACE uint32 BoundsMagic = 0xdeadf00d;
}
#endif

// Enable this to fill new/deleted memory with marker values
#define ENABLE_FIXED_SIZED_POOL_MEMORY_FILLS _DEBUG

#if ENABLE_FIXED_SIZED_POOL_ALLOCATOR_DUMP
#define	DUMP_ALLOC( size_, ptr_ )			writeToDumpFile( Detail::FixedSizeAllocatorRecord::FSAR_AT_ALLOC, size_, ptr_ )
#define	DUMP_FREE( ptr_ )					writeToDumpFile( Detail::FixedSizeAllocatorRecord::FSAR_AT_FREE, 0, ptr_ )
#else
#define	DUMP_ALLOC( size_, ptr_ ) 
#define	DUMP_FREE( ptr_ )
#endif // ENABLE_FIXED_SIZED_POOL_ALLOCATOR_DUMP

namespace 
{

class ScopedLock
{
public:
	ScopedLock( BW_NAMESPACE SimpleMutex & sm ) :
		sm_( sm ),
		grabbed_(false)
	{
		sm_.grab();
		grabbed_ = true;
	}
	~ScopedLock()
	{ 
		this->release(); 
	}
	void release()
	{
		if (grabbed_)
		{
			grabbed_ = false;
			sm_.give();
		}
	}
private:
	BW_NAMESPACE SimpleMutex & sm_;
	bool grabbed_;
};

} // anonymous namespace

namespace BW
{


FixedSizedAllocator::FixedSizedAllocator( const size_t * sizes,
		size_t numSizes, size_t pool_size, const char * debugName, bool dumpAllocs ) :
	poolSize_( pool_size ),
	numPools_( numSizes ),
	lock_(),
	malloc_( NULL ),
	free_( NULL ),
	realloc_( NULL ),
	memsize_( NULL ),
	autoPools_( false ),
	autoPoolsMaxSize_( 0 ),
	poolSpans_( 0 ),
	poolSpanCount_( 0 ),
	poolSpanUsed_ ( 0 )
{
	MF_ASSERT_NOALLOC( poolSize_ > 0 );
	MF_ASSERT_NOALLOC( numPools_ > 0 && numPools_ < (size_t)MaxPools );

	memset( topLevelPools_, 0, sizeof( PoolHeader* ) * MaxPools );

	for (size_t i = 0; i < numSizes; ++i)
	{
		// allocation size must be greater or equal than size of a pointer
		MF_ASSERT_DEV( sizes[i] >= sizeof( void* ) );

		allocSizes_[i] = sizes[i];

#if ENABLE_FIXED_SIZED_POOL_STATISTICS
		debugPoolStatistics_[i].allocatorStats_ = &allocatorStatistics_;
		debugPoolStatistics_[i].allocSize_ = sizes[i];
		debugPoolStatistics_[i].poolSize_ = pool_size / sizes[i] * sizes[i];
#endif
	}

#if ENABLE_FIXED_SIZED_POOL_STATISTICS
	bw_zero_memory( debugName_, ARRAY_SIZE( debugName_ ) );
	strncpy( debugName_, debugName, ARRAY_SIZE( debugName_ ) );
#else
	(void)debugName;
#endif

#if ENABLE_FIXED_SIZED_POOL_ALLOCATOR_DUMP
	dumpFile_ = 0;
	if (dumpAllocs)
	{
		char dumpFileName[ MAX_PATH ] = { 0 };
		strcpy( dumpFileName, debugName );
		strcat( dumpFileName, ".dump" );
		dumpFile_ = fopen( dumpFileName, "w+b" );
		if (!dumpFile_)
		{
			dprintf( "Error : Failed to create FixedSizeAllocator dump file : %s\n", dumpFileName );
		}
	}
#endif
}


FixedSizedAllocator::FixedSizedAllocator( size_t maxPoolSize,
	size_t poolSize, const char * debugName ) :
	poolSize_( poolSize ),
	numPools_( 0 ),
	malloc_( NULL ),
	free_( NULL ),
	realloc_( NULL ),
	memsize_( NULL ),
	autoPools_( true ),
	autoPoolsMaxSize_( maxPoolSize ),
	poolSpans_( 0 ),
	poolSpanCount_( 0 ),
	poolSpanUsed_ ( 0 )
#if ENABLE_FIXED_SIZED_POOL_ALLOCATOR_DUMP
	, dumpFile_()
#endif
{
	memset( topLevelPools_, 0, sizeof( PoolHeader * ) * MaxPools );
	memset( allocSizes_, 0, sizeof( size_t ) * MaxPools );

#if ENABLE_FIXED_SIZED_POOL_STATISTICS
	bw_zero_memory( debugName_, ARRAY_SIZE( debugName_ ) );
	strncpy( debugName_, debugName, ARRAY_SIZE( debugName_ ) );
#else
	(void)debugName;
#endif
}


//-----------------------------------------------------------------------------

FixedSizedAllocator::~FixedSizedAllocator()
{
	// free all pools
	for (size_t i = 0; i < numPools_; ++i)
	{
		PoolHeader* hdr = topLevelPools_[i];
		while (hdr)
		{
			PoolHeader* next = hdr->nextPool_;
			(*free_)( hdr, Allocator::IF_INTERNAL_ALLOC );
			hdr = next;
		}
	}
	memset( topLevelPools_, 0, sizeof( PoolHeader* ) * MaxPools );

	// clear pool span array
	(*free_)( poolSpans_, Allocator::IF_INTERNAL_ALLOC );
	poolSpans_ = 0;
}


/**
 *	Set malloc and free hooks to use when allocation is too large for a pool.
 */
void FixedSizedAllocator::setLargeAllocationHooks(
	MallocFunc m,
	FreeFunc f,
	ReallocFunc r,
	MSizeFuncType ms )
{
	malloc_ = m;
	free_ = f;
    realloc_ = r;
	memsize_ = ms;
}


/**
 *	Allocate memory.
 */
void * FixedSizedAllocator::allocate( size_t size, uint32 heapAllocFlags )
{
	return this->internalAllocate( size, heapAllocFlags, NULL );
}

void * FixedSizedAllocator::internalAllocate( size_t size,
	uint32 heapAllocFlags, const int* poolHint )
{
	// take the global lock
	ScopedLock lock( lock_ );

	const int pool = poolHint ? *poolHint : this->findPool( size );

	if (pool == -1)
	{
		// too big so just grab from heap
		lock.release();
		return (*malloc_)( size, heapAllocFlags );
	}

	MF_ASSERT_NOALLOC( pool < static_cast<int>( numPools_ ) );

	// see if we have space in an existing pool
	PoolHeader* hdr = topLevelPools_[pool];
	while (hdr)
	{
		if (hdr->numFree_)
		{
			void * ptr = hdr->firstFree_;

			this->checkPointer( hdr, ptr );

			memcpy( &hdr->firstFree_, ptr, sizeof( void* ) );

			// can be null if last available slot
			if (hdr->firstFree_)
			{
				this->checkPointer( hdr, hdr->firstFree_ );
			}

			--hdr->numFree_;

#if ENABLE_FIXED_SIZED_POOL_STATISTICS
			this->debugGetStatistics( hdr->allocSize_ ).incAllocations();
#endif

#if ENABLE_FIXED_SIZED_POOL_MEMORY_FILLS
			memset( ptr, Allocator::CleanLandFill, hdr->allocSize_ );
#endif

			// link this pool at the head under the assumption that it has free space so subsequent
			// allocations will find a free pool at the head
			if (hdr->numFree_ && topLevelPools_[pool] != hdr)
			{
				// unlink this pool
				if (hdr->prevPool_)
				{
					hdr->prevPool_->nextPool_ = hdr->nextPool_;
				}
				if (hdr->nextPool_)
				{
					hdr->nextPool_->prevPool_ = hdr->prevPool_;
				}

				// relink at head 
				hdr->prevPool_ = 0;
				hdr->nextPool_ = topLevelPools_[pool];
				topLevelPools_[pool]->prevPool_ = hdr;
				topLevelPools_[pool] = hdr;
			}

			DUMP_ALLOC( static_cast<uint32>(size), ptr );

			return ptr;
		}
		hdr = hdr->nextPool_;
	}

	// none free so create a new one
	const size_t allocSize = allocSizes_[pool];

#if ENABLE_FIXED_SIZE_POOL_BOUNDS_CHECKING
	const size_t allocWithBoundSize = allocSize + sizeof( BoundsMagic );
#else
	const size_t allocWithBoundSize = allocSize;
#endif

	// round pool size to closest integer multiple of allocation size
	const size_t numPoolItems = static_cast< size_t >( poolSize_ ) / allocSize;
	MF_ASSERT_NOALLOC( numPoolItems > 0 );

	// allocate memory with embedded header and room for bounds checks
	size_t memSize = numPoolItems * allocWithBoundSize + sizeof( PoolHeader );

	void* newPoolMem = (*malloc_)( memSize, Allocator::IF_INTERNAL_ALLOC );

#if ENABLE_FIXED_SIZED_POOL_MEMORY_FILLS
	memset( newPoolMem, Allocator::NoMansLandFill, memSize );
#endif

	PoolHeader* newPool = reinterpret_cast< PoolHeader * >( newPoolMem );
	newPool->nextPool_ = 0;
	newPool->prevPool_ = 0;
	newPool->numFree_ = numPoolItems;
	newPool->numItems_ = numPoolItems;
	newPool->allocSize_ = allocSize;
	newPool->firstFree_ =
		reinterpret_cast< char * >( newPoolMem ) + sizeof( PoolHeader );

	this->checkPointer( newPool, newPool->firstFree_ );

#if ENABLE_FIXED_SIZED_POOL_STATISTICS
	this->debugGetStatistics( newPool->allocSize_ ).incPools();
#endif

	// link into the front of the existing pools
	if (topLevelPools_[pool])
	{
		newPool->nextPool_ = topLevelPools_[pool];
		topLevelPools_[pool]->prevPool_ = newPool;
		topLevelPools_[pool] = newPool;
	}
	else
	{
		topLevelPools_[pool] = newPool;
	}

	// link up free blocks
	char* p = reinterpret_cast<char *>( newPool->firstFree_ );

	for (size_t i = 0; i < numPoolItems - 1; ++i)
	{
		void* cur  = reinterpret_cast< void* >( p + allocWithBoundSize * i );
		void* next = reinterpret_cast< void* >( p + allocWithBoundSize * (i + 1) );
		memcpy( cur, &next, sizeof( void* ) );

#if ENABLE_FIXED_SIZE_POOL_BOUNDS_CHECKING
		char* pEntryEnd( reinterpret_cast< char * >( cur ) + allocSize );
		memcpy( pEntryEnd, &BoundsMagic, sizeof( BoundsMagic ) );
#endif
	}

	// last entry is 0.
	void* pLastEntry( p + allocWithBoundSize * (numPoolItems - 1) );
	memset( pLastEntry, 0, sizeof( void* ) );

#if ENABLE_FIXED_SIZE_POOL_BOUNDS_CHECKING
	char* pEntryEnd( reinterpret_cast< char * >( pLastEntry ) + allocSize );
	memcpy( pEntryEnd, &BoundsMagic, sizeof( BoundsMagic ) );
#endif

	// and allocate user memory
	void* ptr = newPool->firstFree_;

	this->checkPointer( newPool, ptr );

	memcpy( &newPool->firstFree_, ptr, sizeof( void* ) );
	--newPool->numFree_;

	// Can be null if last available slot
	// i.e. this pool is only fitting one item
	if (newPool->firstFree_ != NULL)
	{
		this->checkPointer( newPool, newPool->firstFree_ );
	}

	addPoolSpan( newPoolMem, (char*)newPoolMem + sizeof(PoolHeader) + numPoolItems * allocWithBoundSize);

#if ENABLE_FIXED_SIZED_POOL_STATISTICS
	this->debugGetStatistics( newPool->allocSize_ ).incAllocations();
#endif

#if ENABLE_FIXED_SIZED_POOL_MEMORY_FILLS
	memset( ptr, Allocator::CleanLandFill, newPool->allocSize_ );
#endif

	DUMP_ALLOC( static_cast<uint32>(size), ptr );
	return ptr;
}


/**
 *	Free memory.
 */
void FixedSizedAllocator::deallocate( void * ptr, uint32 heapAllocFlags )
{
	this->internalDeallocate( ptr, heapAllocFlags, NULL );
}

void FixedSizedAllocator::internalDeallocate( void * ptr,
	uint32 heapAllocFlags, PoolHeader * const * poolHint )
{
	if (ptr == 0)
	{
		return;
	}

	ScopedLock lock( lock_ );

	PoolHeader* pool = poolHint ? *poolHint : this->findAllocPool( ptr );

	if (!pool)
	{
		// came from the heap
		lock.release();
		(*free_)( ptr, heapAllocFlags );
		return;
	}

#if ENABLE_FIXED_SIZE_POOL_BOUNDS_CHECKING
	// check the magic is intact
	{
		const char* cPtr( reinterpret_cast< char* >( ptr ) );
		const uint32 magic =
			*(reinterpret_cast< const uint32 * >( cPtr + pool->allocSize_ ));
		MF_ASSERT_NOALLOC( magic == BoundsMagic );
	}
#endif

#if ENABLE_FIXED_SIZED_POOL_STATISTICS
	this->debugGetStatistics( pool->allocSize_ ).decAllocations();
#endif

	this->checkPointer( pool, ptr );

#if ENABLE_FIXED_SIZED_POOL_MEMORY_FILLS
	memset( static_cast< char * >( ptr ) + sizeof( void * ),
		Allocator::DeadLandFill, pool->allocSize_ - sizeof(void*) );
#endif

	// return this block to the pool
	memcpy( ptr, &pool->firstFree_, sizeof( void* ) );

	this->checkPointer( pool, ptr );

	pool->firstFree_ = ptr;

#if _DEBUG
	{
		void* tmp;
		memcpy( &tmp, ptr, sizeof( void* ) );
		if (tmp)
		{
			this->checkPointer( pool, tmp );
		}
	}
#endif


	++pool->numFree_;
	if (pool->numFree_ == pool->numItems_)
	{
		// Remove the pool from the linked list
		if (pool->prevPool_)
		{
			pool->prevPool_->nextPool_ = pool->nextPool_;
		}
		else
		{
			// If there is no previous pool,
			// this must be first in the linked list and so
			// this must be a top level pool
			// Get index into the top level pool array
			const int index = this->findPool( pool->allocSize_ );
			MF_ASSERT_NOALLOC( index != -1 );
			topLevelPools_[index] = pool->nextPool_;
		}

		if (pool->nextPool_)
		{
			pool->nextPool_->prevPool_ = pool->prevPool_;
		}

#if ENABLE_FIXED_SIZED_POOL_STATISTICS
		this->debugGetStatistics( pool->allocSize_ ).decPools();
#endif

		// Delete the pool
		(*free_)( pool, Allocator::IF_INTERNAL_ALLOC );
		delPoolSpan( pool );
		pool = NULL;
	}

	DUMP_FREE( ptr );
}


/**
 *	Reallocate memory.
 */
void* FixedSizedAllocator::reallocate( void * ptr, size_t size, uint32 heapAllocFlags )
{
	PoolHeader * const * deallocateHint = NULL;
	PoolHeader * currentPool = NULL;
	int * allocateHint = NULL;
	int newPool = -1;
	size_t copySize = 0;

	// see if we can reused existing block or if memory came from the heap
	if (ptr)
	{
		ScopedLock lock( lock_ );

		currentPool = this->findAllocPool( ptr );
		newPool = this->findPool( size );

		if (!currentPool)
		{
			if (newPool == -1)
			{
				// new size doesn't fit in pools
				lock.release();
				return (*realloc_)( ptr, size, heapAllocFlags );
			}

			copySize = std::min( allocSizes_[newPool], size );
		}
		else
		{
			if (newPool != -1 && allocSizes_[newPool] == currentPool->allocSize_)
			{
				// new size fits into current allocation size so we can just use
				// the same memory block
				return ptr;
			}

			copySize = std::min( currentPool->allocSize_, size );
		}

		// use these so they don't need to be looked up again
		allocateHint = &newPool;
		deallocateHint = &currentPool;
	}

	// doesn't not fit so allocate new block, copy old contents and free old 
	// block
	void* newPtr = this->internalAllocate( size, heapAllocFlags, allocateHint );
	memcpy( newPtr, ptr, copySize );
	this->internalDeallocate( ptr, heapAllocFlags, deallocateHint );

	return newPtr;
}


/**
 *	Determine the allocation size for the given memory address.
 */
size_t FixedSizedAllocator::memorySize( void* ptr )
{
	ScopedLock smh( lock_ );

	const PoolHeader* pool = this->findAllocPool( ptr );

	if (!pool)
	{
		// came from the heap
		return (*memsize_)( ptr );
	}
	return pool->allocSize_;
}

void FixedSizedAllocator::allocationInfo( void* ptr, AllocationInfo& info )
{
	ScopedLock smh( lock_ );
	if (const PoolHeader* pool = this->findAllocPool( ptr ))
	{
		info.allocFlags_ = Allocator::IF_POOL_ALLOC;
		info.allocSize_ = pool->allocSize_;
	}
	else
	{
		// came from the heap
		info.allocFlags_ = 0;
		info.allocSize_ = (*memsize_)( ptr );
	}
}

/**
 *	Check if there exists a memory pool that a given block of memory could fit
 *	into.
 *	@param requiredSize the size of the memory block that we want to find a
 *		pool for.
 *	@return true if the memory pool for a block of that size exists.
 *		false if there is no pool for memory of that size.
 */
bool FixedSizedAllocator::poolExistsForSize( size_t requiredSize ) const
{
	for (size_t pool = 0; pool < numPools_; ++pool)
	{
		if (requiredSize <= allocSizes_[pool])
		{
			return true;
		}
	}
	return false;
}


/**
 *	Check how many free blocks are left in the last memory pool in the linked
 *	list for a given size.
 *	If the number of free blocks is 0, then it will create a new pool on the
 *	next call to FixedSizedAllocator::allocate().
 *	@param requiredSize the size of the memory block that we want to find a
 *		pool for.
 *	@return the number of blocks remaining in the memory pool for a block of
 *		that size.
 *		0 if the pool is full or there is no pool for memory of that
 *		size.
 */
size_t FixedSizedAllocator::getFirstPoolNumFreeForSize( size_t requiredSize ) const
{
	for (size_t pool = 0; pool < numPools_; ++pool)
	{
		if (requiredSize <= allocSizes_[pool])
		{
			const PoolHeader* header = topLevelPools_[pool];
			// No allocations have happened yet - no header
			if (header == NULL)
			{
				return 0;
			}
			return header->numFree_;
		}
	}
	return 0;
}


/**
 *	Check how the maximum number of items that can go in a given memory pool.
 *	If the number of items is at its maximum, then it will create a new pool
 *	on the next FixedSizedAllocator::allocate.
 *	@param requiredSize the size of the memory block that we want to find a
 *		pool for.
 *	@return the number of items that can fit in the memory pool for a block of
 *		that size.
 *		0 if there is no pool for memory of that size.
 */
size_t FixedSizedAllocator::getNumPoolItemsForSize( size_t requiredSize )
{
	const int pool( this->findPool( requiredSize ) );

	// none free so create a new one
	size_t allocSize = allocSizes_[pool];

	// round pool size to closest integer multiple of allocation size
	const size_t numPoolItems = static_cast< size_t >( poolSize_ ) / allocSize;

	return numPoolItems;
}


/**
 *	Check if the linked list of memory pools of a given size is completely
 *	empty.
 *	@param requiredSize the size of the memory block that we want to find a
 *		pool for.
 *	@return true if the memory pool for a block of that size is empty.
 *		false if the pool is not empty or there is no pool for memory of that
 *		size.
 */
bool FixedSizedAllocator::isAllPoolsEmptyForSize( size_t requiredSize ) const
{
	for (size_t pool = 0; pool < numPools_; ++pool)
	{
		if (requiredSize <= allocSizes_[pool])
		{
			const PoolHeader* header = topLevelPools_[pool];
			// No allocations have happened yet - no header -> empty
			if (header == NULL)
			{
				return true;
			}
			return false;
		}
	}
	return false;
}


/**
 *	Check if there is space remaining in a memory pool for the given size.
 *	If this is true, then the next call to FixedSizedAllocator::allocate()
 *	will have to create a new pool and add it to the linked list.
 *	@param requiredSize the size of the memory block that we want to find a
 *		pool for.
 *	@return true if there is space in the memory pool for a block of that size.
 *		false if the pool is full or there is no pool for memory of that size.
 */
bool FixedSizedAllocator::isLastPoolFullForSize( size_t requiredSize ) const
{
	for (size_t pool = 0; pool < numPools_; ++pool)
	{
		if (requiredSize <= allocSizes_[pool])
		{
			const PoolHeader* header = topLevelPools_[pool];
			// No allocations have happened yet - no header
			if (header == NULL)
			{
				return true;
			}
			// Search linked list of pools to see if one is empty
			while (header->nextPool_ != NULL)
			{
				if (header->numFree_ > 0)
				{
					return false;
				}
				header = header->nextPool_;
			}
			return (header->numFree_ == 0);
		}
	}
	return false;
}


/**
 *	Get the size of the memory pools.
 *	@return the size of the memory pools.
 */
size_t FixedSizedAllocator::getPoolSize() const
{
	return poolSize_;
}


/**
 *	Returns the index of the smallest pool that satisfies the requested
 *  allocation size or -1 if request is too large for any pool.
 */
int FixedSizedAllocator::findPool( size_t reqSize )
{
	for (size_t i = 0; i < numPools_; ++i)
	{
		if (reqSize <= allocSizes_[i])
		{
			return static_cast<int>(i);
		}
	}

	// if using auto pools then create a new pool for this request size
	if (autoPools_ && reqSize <= autoPoolsMaxSize_)
	{
		MF_ASSERT_NOALLOC( numPools_ < (size_t)MaxPools );
		reqSize = std::max( sizeof(void*), reqSize );

		// pools must be increasing size order so add at end then shift into place
		allocSizes_[numPools_] = reqSize;
		size_t curSpot = numPools_;

#if ENABLE_FIXED_SIZED_POOL_STATISTICS
		debugPoolStatistics_[curSpot].allocatorStats_ = &allocatorStatistics_;
		debugPoolStatistics_[curSpot].allocSize_ = reqSize;
		debugPoolStatistics_[curSpot].poolSize_ = poolSize_ / reqSize * reqSize;
#endif

		while (curSpot > 0)
		{
			if (allocSizes_[curSpot] < allocSizes_[curSpot - 1])
			{
				// swap them
				std::swap( allocSizes_[curSpot], allocSizes_[curSpot - 1] );
				std::swap( topLevelPools_[curSpot], topLevelPools_[curSpot -1] );

#if ENABLE_FIXED_SIZED_POOL_STATISTICS
				std::swap( debugPoolStatistics_[curSpot], debugPoolStatistics_[curSpot - 1] );
#endif
				--curSpot;
			}
			else
			{
				break;
			}
		}
		return static_cast<int>(numPools_++);
	}

	return -1;
}


void FixedSizedAllocator::addPoolSpan( void* start, void * end )
{
	if (!poolSpans_)
	{
		poolSpanCount_ = 64;
		poolSpans_ = (PoolSpan*)(*malloc_)( sizeof(PoolSpan) * poolSpanCount_, Allocator::IF_INTERNAL_ALLOC );
		bw_zero_memory( poolSpans_, sizeof( PoolSpan ) * poolSpanCount_ );
	}
	else if (poolSpanCount_ == poolSpanUsed_)
	{
		size_t newPoolSpanCount = poolSpanCount_ *  2;
		PoolSpan * tmp = (PoolSpan*)(*malloc_)(sizeof(PoolSpan) * newPoolSpanCount, Allocator::IF_INTERNAL_ALLOC);
		memcpy( tmp, poolSpans_, sizeof(PoolSpan) * poolSpanCount_ );
		bw_zero_memory( (char*)tmp + sizeof(PoolSpan) * poolSpanCount_, sizeof(PoolSpan) * (newPoolSpanCount - poolSpanCount_) );
		(*free_)( poolSpans_, Allocator::IF_INTERNAL_ALLOC );
		poolSpans_ = tmp;
		poolSpanCount_ = newPoolSpanCount;
	}

	// find free spot
	for (size_t i = 0; i < poolSpanCount_; ++i)
	{
		if (poolSpans_[i].start_ == 0)
		{
			poolSpans_[i].start_ = start;
			poolSpans_[i].end_ = end;
			++poolSpanUsed_;
			return;
		}
	}
}

void FixedSizedAllocator::delPoolSpan( void* start )
{
	// find free spot
	for (size_t i = 0; i < poolSpanCount_; ++i)
	{
		if (poolSpans_[i].start_ == start)
		{
			poolSpans_[i].start_ = 0;
			poolSpans_[i].end_ = 0;
			--poolSpanUsed_;

			if (poolSpanUsed_ == 0)
			{
				(*free_)( poolSpans_, Allocator::IF_INTERNAL_ALLOC );
				poolSpans_ = NULL;
				poolSpanCount_ = 0;
			}

			return;
		}
	}
}

/**
 * Returns pointer to the pool the passed pointer was allocated from or
 * 0 if it did not come from any pool.
 */
FixedSizedAllocator::PoolHeader * FixedSizedAllocator::findAllocPool(
	const void * ptr ) const
{
	for (size_t i = 0; i < poolSpanCount_; ++i)
	{
		PoolSpan span = poolSpans_[i];
		if (ptr > span.start_ && ptr < span.end_)
		{
			return (PoolHeader*)span.start_;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Debugging
//-----------------------------------------------------------------------------

/**
 *	Asserts if the passed pointer is not within the pool.
 */
void FixedSizedAllocator::checkPointer(
	const PoolHeader * pool, const void * ptr ) const
{
	const size_t allocSize = pool->allocSize_;

#if ENABLE_FIXED_SIZE_POOL_BOUNDS_CHECKING
	const size_t allocWithBoundSize = allocSize + sizeof( BoundsMagic );
#else
	const size_t allocWithBoundSize = allocSize;
#endif

	const char* cPtr( reinterpret_cast< const char * >( ptr ) );
	const char* cPool( reinterpret_cast< const char * >( pool ) );

	// check pointer falls with this pool
	MF_ASSERT_NOALLOC( cPtr >= (cPool + sizeof( PoolHeader )) );
	MF_ASSERT_NOALLOC( cPtr <
		(cPool + sizeof( PoolHeader ) + pool->numItems_ * allocWithBoundSize ) );

	// check we are freeing a pointer that is correctly aligned
	const size_t offset = (cPtr - cPool) - sizeof( PoolHeader );
	MF_ASSERT_NOALLOC( (offset % allocWithBoundSize) == 0 );
}


#if ENABLE_FIXED_SIZED_POOL_STATISTICS

FixedSizedAllocator::PoolStatistics & FixedSizedAllocator::debugGetStatistics(
	size_t allocSize )
{
	const int p = this->findPool( allocSize );
	MF_ASSERT_NOALLOC( p >= 0 );
	MF_ASSERT_NOALLOC( p < static_cast<int>( numPools_ ) );
	return debugPoolStatistics_[p];
}


void FixedSizedAllocator::debugPrintStatistics() const
{
	dprintf( "----------------------------------------------------------------------------------------------------------\n" );
	dprintf( "| FixedSizeAllocator Statistics (%s)\n", debugName_[0] ? debugName_ : "Unnamed" );
	dprintf( "----------------------------------------------------------------------------------------------------------\n" );
	for (size_t i = 0; i < numPools_; ++i)
	{
		const PoolStatistics& stats = debugPoolStatistics_[i];
		dprintf("|  Pool %2d : Size =%5.d : PeakAllocs = %7.d : PeakPools = %3.d : PeakMemUsed = %.2fmb\n",
			i,
			stats.allocSize_,
			stats.peakAllocations_,
			stats.peakPools_,
			static_cast<float>( stats.peakPools_ ) * stats.poolSize_ / (1024 * 1024)
			);
	}
	dprintf( "----------------------------------------------------------------------------------------------------------\n" );
	dprintf( "|  Peak memory usage = %.2fmb                                                                             \n",
		static_cast<float>( allocatorStatistics_.peakMemUsage_ ) /  (1024 * 1024) );
	dprintf( "----------------------------------------------------------------------------------------------------------\n" );
}

#endif // ENABLE_FIXED_SIZED_POOL_STATISTICS

} // end namespace BW


// fixed_sized_allocator.cpp
