#ifndef BW_MEMORY_DEBUG_HPP
#define BW_MEMORY_DEBUG_HPP

#include "cstdmf/config.hpp"

#if ENABLE_MEMORY_DEBUG

#include "cstdmf/stdmf.hpp"
#include "cstdmf/grow_only_pod_allocator.hpp"
#include "cstdmf/allocator.hpp"
#include "cstdmf/fixed_sized_allocator.hpp"
#include "cstdmf/stl_fixed_sized_allocator.hpp"
#include "cstdmf/callstack.hpp"
#include "cstdmf/stack_tracker.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/bw_unordered_map.hpp"
#include "cstdmf/bw_unordered_set.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_hash.hpp"

namespace BW
{

// Internal allocator for use with memory debug
namespace detail
{
template<class T>
class DebugStlAllocator
{
public:
	typedef T				    value_type;

	typedef value_type          * pointer;
	typedef value_type          & reference;
	typedef const value_type    * const_pointer;
	typedef const value_type    & const_reference;

	typedef size_t      size_type;
	typedef ptrdiff_t   difference_type;

	template <class Other>
	struct rebind
	{
		typedef DebugStlAllocator<Other> other;
	};


	DebugStlAllocator( FixedSizedAllocator* baseAllocator ) :
		baseAllocator_( baseAllocator )
	{
	}


	DebugStlAllocator( const DebugStlAllocator<T>& rhs ) :
		baseAllocator_( rhs.baseAllocator_ )
	{}

	template<class Other>
	DebugStlAllocator( const DebugStlAllocator<Other>& rhs ) :
		baseAllocator_( rhs.baseAllocator_ )
	{
	}

	typename std::allocator<T>::pointer allocate( typename std::allocator<T>::size_type n, typename std::allocator<void>::const_pointer = 0 )
	{
		return (typename std::allocator<T>::pointer)baseAllocator_->allocate( n * sizeof(T), Allocator::IF_DEBUG_ALLOC);
	}

	void deallocate( typename std::allocator<T>::pointer p, typename std::allocator<T>::size_type n )
	{
		baseAllocator_->deallocate( p, Allocator::IF_DEBUG_ALLOC );
	}

	void construct( pointer p, const T & val ) const
	{
		new ((void*)p) T( val );
	}
	void destroy( pointer p ) const
	{
		p->~T();
	}
	size_type max_size() const
	{
		size_type _Count = (size_type)(-1) / sizeof (T);
		return (0 < _Count ? _Count : 1);
	}

//private:
	FixedSizedAllocator* baseAllocator_;
};


template< class T >
bool operator==( const DebugStlAllocator< T > & left,
	const DebugStlAllocator< T > & right )
{
	return left.baseAllocator_ == right.baseAllocator_;
}


template< class T >
bool operator!=( const DebugStlAllocator< T > & left,
	const DebugStlAllocator< T > & right )
{
	return !(left == right);
}

} // namespace detail


class MemoryDebug
{
public:

	enum AllocationFlags
	{
		AF_ALLOCATION_PREMAIN    = 1, ///< Allocation occured before main() called
		AF_ALLOCATION_POOL       = 2, ///< Allocation came from a fixed sized pool
		AF_ALLOCATION_INTERNAL   = 4, ///< Allocation is an internal heap allocation (e.g. fixed sized pool memory)
		AF_ALLOCATION_IGNORELEAK = 8, ///< Allocation has been flagged as allowed to leak (should only be used for thirdparty allocations)
	};

	enum CallstackFlags
	{
		CS_NONE = 0,
		CS_ALLOCATION_HAS_COMPLETE = 1,
		CS_SMARTPOINTER = 2
	};

	enum CallstackMode
	{
		CM_BW_GUARD,
		CM_CALLSTACKS_FAST,
		CM_CALLSTACKS_COMPLETE
	};

	struct ProcessMemoryInfo
	{
		ProcessMemoryInfo() :
			imageBytes(0),
			privateBytes(0),
			mappedBytes(0)
		{}

		size_t imageBytes;
		size_t privateBytes;
		size_t mappedBytes;
		size_t virtualAddressAvailable;
		size_t virtualAddressUsed;
	};

	/// header that precedes callstack
	struct CallstackHeader
	{			
		int32 depth;		///< depth of the callstack
		int32 guardDepth;   ///< depth of the BW_GUARD callstack
		uint8 flags;
		size_t totalSize() const;
		static CallstackAddressType* callstack( CallstackHeader* hdr );
		static const CallstackAddressType* callstack( const CallstackHeader* hdr );
		static StackTracker::StackItem* guardstack( CallstackHeader* hdr );
		static const StackTracker::StackItem* guardstack( const CallstackHeader* hdr );
	};

	struct CallstackStats
	{
		int32 allocCount;	///< number of allocations that came from this callstack
#ifdef MF_SERVER
		timeval firstTime;	///< first time the allocation was performed
#endif // MF_SERVER
	};

public:
	/// constructor
	MemoryDebug( CallstackMode csMode, bool reportHotspots,
		bool reportIgnoredLeaks , bool reportCacheGrind );

	/// destructor
	~MemoryDebug();

	/// inform the memory debugger of the stage of initialisation
	void setSystemStage( Allocator::SystemStage stage );

	/// register an allocation 
	/// \param ptr Pointer to the new allocation
	/// \param sizeUsed The size used by the allocation as returned by memsize
	/// \param sizeRequested The size requested for the allocation
	/// \param flags Allocator::InfoFlags describing the allocation
	void allocate( const void * ptr, size_t sizeUsed, size_t sizeRequested,
		uint32 flags );

	/// register a deallocation
	/// \param ptr Pointer to the memory being deallocated
	void deallocate( const void * ptr );

	/// prints out list of current live allocations
	/// \returns The number of current main allocations
	size_t printLiveAllocations();

	/// prints out recorded allocation statistics
	void printAllocationStatistics();

	/// Saves allocation logs to file (stack trace stats)
	void saveAllocationsToFile( const char* filename );

	/// Save allocation logs to cachegrind format
	void saveAllocationsToCacheGrindFile( const char * filename );

	/// Saves high-level allocation stats to file (slot based stats in CSV format)
	void saveStatsToFile( const char* filename );

	/// show detailed callstacks for all BW_GUARD stacks that contain the 
	/// passed string
	void showDetailedStackForGuard( const char * str );

	/// Read totals from live allocations for unit testing.
	void readLiveAllocationStats( Allocator::AllocationStats& stats ) const;

	/// Read the smart pointer tracking stats for unit testing.
	void readSmartPointerStats( Allocator::SmartPointerStats& stats ) const;

#if defined( _WINDOWS_ )
	/// \returns Structure with information about process memory usage
	static ProcessMemoryInfo getProcessMemoryInfo();
#endif // defined( _WINDOWS_ )

	/// Begin flagging all allocations by the current thread as AF_ALLOCATION_IGNORELEAK
	void allocTrackingIgnoreBegin();

	/// Stop flagging allocations by the current thread as AF_ALLOCATION_IGNORELEAK
	void allocTrackingIgnoreEnd();

	/// Record the assignment of a smart pointer to a given value
	void trackSmartPointerAssignment( const void * smartPointer, const void * object );
	
	/// Print smart pointers holding given object
	void printSmartPointerTracking( const void * object );

	/// \returns True if should generate cachegrind file
	bool shouldReportCacheGrind() { return reportCacheGrind_; }

#if ENABLE_WATCHERS
	void addWatchers();
#endif

private:

	class AllocationStats;

	struct AllocationInfo
	{
		int16 flags;
		int16 slot;
		int32 sizeUsed;
		int32 id;
		const CallstackHeader* callstack;
	};
	
	struct SmartPointerInfo
	{
		const void * object;
		const CallstackHeader * callstack;
	};

	struct AddressLookupCacheEntry
	{
		AddressLookupCacheEntry() : 
			funcname( NULL ), filename( NULL ),
			lineno( -1 ), validAddress( false ) { }
		const char * funcname;
		const char * filename;
		int lineno;
		bool validAddress;
	};
	
	struct HashCallstackHeaderPtr : public std::unary_function< 
		const CallstackHeader*, std::size_t >
	{
		std::size_t operator()( const CallstackHeader* ) const;
	};

	struct EqualsCallstackHeaderPtr : public std::binary_function< 
		const CallstackHeader*, const CallstackHeader*, bool >
	{
		bool operator()( const CallstackHeader*, 
			const CallstackHeader* ) const;
	};

	struct HashCStr : public std::unary_function< const char*, std::size_t >
	{
		std::size_t operator()( const char* ) const;
	};

	struct EqualsCStr : public std::binary_function< const char*, const char*, 
		bool >
	{
		bool operator()( const char*, const char* ) const;
	};

	typedef BW::unordered_map< const void *, SmartPointerInfo, BW::hash< const void* >, std::equal_to< const void* >, detail::DebugStlAllocator< std::pair<const void * const, SmartPointerInfo > > > SmartPointerMap;
	typedef BW::unordered_map< const CallstackHeader*, CallstackStats, HashCallstackHeaderPtr, EqualsCallstackHeaderPtr, detail::DebugStlAllocator< std::pair< const CallstackHeader* const, CallstackStats > > > CallstackMap;
	typedef BW::unordered_map< const void*, AllocationInfo, BW::hash< const void* >, std::equal_to< const void* >, detail::DebugStlAllocator< std::pair< const void* const, AllocationInfo > > > LiveAllocationMap;
	typedef BW::unordered_map< const void*, AddressLookupCacheEntry, BW::hash< const void* >, std::equal_to< const void* >, detail::DebugStlAllocator< std::pair< const void* const, AddressLookupCacheEntry > > > LookupCacheMap;
	typedef BW::unordered_set< const char*, HashCStr, EqualsCStr, detail::DebugStlAllocator< const char* > > HashStringMap;

	GrowOnlyPodAllocator	allocator_;
	CallstackMap*			callstacks_;
	LiveAllocationMap*		liveAllocations_;
	HashStringMap*			stringTable_;
	LookupCacheMap*			lookupCache_;
	FixedSizedAllocator*	debugAllocator_;
	AllocationStats*		allocationStats_;
	SmartPointerMap*		smartPointers_;
	Allocator::SystemStage	systemStage_;
	mutable SimpleMutex		lock_;

	/// emit the N callstacks that emit the highest number of allocations
	bool					reportHottestCallstacks_;
	bool					internalInit_;
	bool					showDetailedCallstackForGuard_;
	bool					reportIgnoredLeaks_;
	bool					reportCacheGrind_;

	/// callstack generated when we are initialised, used to decided whether we need to do 
	/// a more complete stack trace
#if BW_ENABLE_STACKTRACE && defined( _WINDOWS_ )
	CallstackAddressType	callstackTop_;
#endif
	uint32					curAllocId_;

	uint64					totalStacksGenerated_;
	uint64					totalStacksGeneratedSlow_;
	CallstackMode			callstackMode_;
	char detailedCallstackForGuardString_[256];
	ProcessMemoryInfo		initialProcessMemoryInfo_;
	uint64					totalSmartPointerAssigns_;

	// static memory areas to allocate internal structures to avoid hitting the heap
	char callstacksMem_[sizeof(CallstackMap)];
	char liveAllocationsMem_[sizeof(LiveAllocationMap)];
	char debugAllocatorMem_[sizeof(FixedSizedAllocator)];
	char stringTableMem_[sizeof(HashStringMap)];
	char lookupCacheMem_[sizeof(LookupCacheMap)];
	char smartPointersMem_[sizeof(SmartPointerMap)];

	static THREADLOCAL( bool ) internalAlloc_;
	static THREADLOCAL( uint ) ignoreAllocs_;

	void init();

	uint recordGuardCallstack( StackTracker::StackItem * pBuffer, uint length, bool * requestDetailedCallstack );

	bool matchIgnoredFunction( const char * functionName );

	std::pair< const CallstackHeader * const, CallstackStats > & 
		getCallstackEntry( uint8 flags, size_t skipFrames );

	const char* getStringTableEntry( const char* );
	void formatShortGuardStack( const StackTracker::StackItem* items, size_t numItems, BW::StringBuilder & builder );
	void formatLongGuardStack( const StackTracker::StackItem * items, size_t numItems, BW::StringBuilder & builder );

#if BW_ENABLE_STACKTRACE
	bool cachedResolve( const CallstackAddressType address,
		const char ** ppFuncname, const char ** ppFilename = NULL, int * ppLineno = NULL );
	void formatShortCallstack( const CallstackAddressType* items, size_t numItems, BW::StringBuilder& builder );
	void formatLongCallstack( const CallstackAddressType* callstack, int depth, BW::StringBuilder & builder );
#endif

	friend 	bool sortCallstacksByAllocCount( const CallstackHeader * lhs_, const CallstackHeader * rhs_ );
};

} // namespace BW

#endif // ENABLE_MEMORY_DEBUG

#endif // BW_MEMORY_DEBUG_HPP
