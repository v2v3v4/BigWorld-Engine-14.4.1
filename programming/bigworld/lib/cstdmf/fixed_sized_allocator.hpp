#ifndef FIXED_SIZED_ALLOCATOR_HPP
#define FIXED_SIZED_ALLOCATOR_HPP

#include "cstdmf/config.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/concurrency.hpp"

#include "cstdmf/bw_vector.hpp"

#if defined( WIN32 ) && !defined( _M_X64 )
#define FIXED_SIZE_ALLOCATOR_CALL_CONV __cdecl
#else // WIN32
#define FIXED_SIZE_ALLOCATOR_CALL_CONV
#endif // WIN32

namespace BW
{

/*
 * Implements an allocator using fixed size pools of user specified sizes.
 * Each pool will allocate a fixed amount of memory and allocate more pools as
 * necessary. Allocations over the max pool size will be allocated from and
 * returned to the heap. The primary intent of this is to reduce fragmentation
 * by allocation and destruction of small size objects. No memory is allocated
 * for any of the sizes until is attempted to be allocated and empty pools are
 * returned immediately to the heap.
 */
class FixedSizedAllocator
{
public:
	typedef void* (*MallocFunc)( size_t, uint32 );
	typedef void (*FreeFunc)( void*, uint32 );
	typedef void* (*ReallocFunc)( void*, size_t, uint32 );
	typedef size_t (FIXED_SIZE_ALLOCATOR_CALL_CONV *MSizeFuncType)( void* );

	struct AllocationInfo
	{
		size_t allocSize_;
		uint32 allocFlags_;
	};

public:
	FixedSizedAllocator( const size_t * sizes, size_t numSizes,
		size_t pool_size, const char * debugName, bool dumpAllocs = false );

	FixedSizedAllocator( size_t maxPoolSize, size_t poolSize, const char * debugName );

	~FixedSizedAllocator();

	void setLargeAllocationHooks(
		MallocFunc, FreeFunc, ReallocFunc, MSizeFuncType );

	void* allocate( size_t size, uint32 heapAllocflags = 0 );
	void  deallocate( void * ptr, uint32 heapAllocflags = 0 );
	void* reallocate( void * ptr, size_t size, uint32 heapAllocflags = 0 );

	size_t memorySize( void * ptr );

	void allocationInfo( void* ptr, AllocationInfo& info );

	/// Accessors used for testing
	bool poolExistsForSize( size_t requiredSize ) const;
	size_t getFirstPoolNumFreeForSize( size_t requiredSize ) const;
	size_t getNumPoolItemsForSize( size_t requiredSize );
	bool isAllPoolsEmptyForSize( size_t requiredSize ) const;
	bool isLastPoolFullForSize( size_t requiredSize ) const;
	size_t getPoolSize() const;

private:
	struct PoolHeader
	{
		PoolHeader* prevPool_;
		PoolHeader* nextPool_;  ///< pointer to next pool in linked list
		void* firstFree_; ///< pointer to first free block or 0 if full.
		size_t numFree_;
		size_t numItems_;
		size_t allocSize_;
	};

	struct PoolSpan
	{
		void* start_;
		void* end_;
	};

private:

	int findPool( size_t reqSize );
	void * internalAllocate( size_t size, uint32 heapAllocflags, const int * poolHint );
	void internalDeallocate( void * ptr, uint32 heapAllocflags, PoolHeader * const * poolHint );
	PoolHeader * findAllocPool( const void * ptr ) const;
	void checkPointer( const PoolHeader * pool, const void * ptr ) const;
	void addPoolSpan( void* start, void * end );
	void delPoolSpan( void* start );

private:
	static const int MaxPools = 16;

	/// Size of each pool
	size_t          poolSize_;

	/// Allocation size for each pool
	size_t          allocSizes_[MaxPools];

	/// Top level for pools of each size
	PoolHeader*     topLevelPools_[MaxPools];

	/// Number of top level pools
	size_t          numPools_;

	SimpleMutex     lock_;
	MallocFunc      malloc_;
	FreeFunc        free_;
    ReallocFunc     realloc_;
	MSizeFuncType   memsize_;
 	bool			autoPools_;
 	size_t			autoPoolsMaxSize_;
	PoolSpan*		poolSpans_;
	size_t			poolSpanCount_;
	size_t			poolSpanUsed_;

#if ENABLE_FIXED_SIZED_POOL_STATISTICS
public:

	struct AllocatorStatistics
	{
		AllocatorStatistics() :
			peakMemUsage_(0),
			curMemUsage_(0)
		{}

		size_t peakMemUsage_;
		size_t curMemUsage_;

		void incMemUsage( size_t bytes )
		{
			curMemUsage_ += bytes;
			if (curMemUsage_ > peakMemUsage_)
			{
				peakMemUsage_ = curMemUsage_;
			}
		}

		void decMemUsage( size_t bytes )
		{
			curMemUsage_ -= bytes;
		}
	};

	struct PoolStatistics
	{
		PoolStatistics() :
			allocatorStats_(),
			allocSize_( 0 ),
			peakPools_( 0 ),
			peakAllocations_( 0 ),
			numLiveAllocations_( 0 ),
			numLivePools_( 0 ),
			poolSize_( 0 )
		{}

		AllocatorStatistics* allocatorStats_;
		size_t allocSize_;			///< allocation size this pool is for
		size_t peakPools_;			///< peak number of allocated pools
		size_t peakAllocations_;	///< peak number of simultaneous allocations
		size_t numLiveAllocations_;	///< number of currently live allocations
		size_t numLivePools_;		///< number of currently live pools
		size_t poolSize_;			///< size of the pool in bytes

		void incPools()
		{
			++numLivePools_;
			if (numLivePools_ > peakPools_)
			{
				peakPools_ = numLivePools_;
			}
			allocatorStats_->incMemUsage( poolSize_ );
		}

		void decPools()
		{
			MF_ASSERT( numLivePools_ > 0 );
			--numLivePools_;
			allocatorStats_->decMemUsage( poolSize_ );
		}

		void incAllocations()
		{
			++numLiveAllocations_;
			if (numLiveAllocations_ > peakAllocations_)
			{
				peakAllocations_ = numLiveAllocations_;
			}
		}

		void decAllocations()
		{
			MF_ASSERT( numLiveAllocations_ > 0);
			--numLiveAllocations_;
		}
	};

	void debugPrintStatistics() const;

private:
	AllocatorStatistics allocatorStatistics_;
	PoolStatistics debugPoolStatistics_[ MaxPools ];
	PoolStatistics & debugGetStatistics( size_t allocSize );
	char debugName_[ 256 ];
#endif // ENABLE_FIXED_SIZED_POOL_STATISTICS
};

} // end namespace BW


#endif // FIXED_SIZED_ALLOCATOR_HPP

