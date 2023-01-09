#include "pch.hpp"

#include "config.hpp"

#if ENABLE_MEMORY_DEBUG

#include "bw_namespace.hpp"
#include "bw_string_ref.hpp"
#include "callstack.hpp"
#include "dprintf.hpp"
#include "fixed_sized_allocator.hpp"
#include "memory_debug.hpp"
#include "scoped_bool.hpp"
#include "slot_tracker.hpp"
#include "stack_tracker.hpp"
#include "static_array.hpp"
#include "string_builder.hpp"
#include "watcher.hpp"

#include <limits.h>
#include <time.h>

#ifdef MF_SERVER
#include <sys/time.h>
#endif // MF_SERVER

namespace BW
{

namespace
{
const size_t SYMBOL_STRING_MAX_LENGTH = 1024;
const size_t MAX_GUARD_DEPTH = 512;
const size_t MAX_CALLSTACK_DEPTH = 512;
const size_t CALLSTACK_HEADER_SIZE = sizeof(MemoryDebug::CallstackHeader);
const size_t GUARD_ITEM_SIZE = sizeof(StackTracker::StackItem);
const size_t CALLSTACK_ITEM_SIZE = sizeof(CallstackAddressType);
const size_t MAX_CALLSTACK_SIZE = CALLSTACK_HEADER_SIZE + 
	(GUARD_ITEM_SIZE * MAX_GUARD_DEPTH) +
	(CALLSTACK_ITEM_SIZE * MAX_CALLSTACK_DEPTH);

// used for reporting leaked callstacks.
typedef std::pair< const BW::MemoryDebug::CallstackHeader*, 
	BW::MemoryDebug::CallstackStats > CallstackStatPair;
BW_NAMESPACE StaticArray< CallstackStatPair, 256 * 1024 > s_sortedCallstacks;

class CacheGrindEventInfo
{
public:
	CacheGrindEventInfo()
		: allocCount(0)
		, liveCount(0)
		, liveSize(0)
		, ignoredCount(0)
		, ignoredSize(0)
		, internalCount(0)
		, internalSize(0)
	{
	}

	size_t allocCount;
	size_t liveCount;
	size_t liveSize;
	size_t ignoredCount;
	size_t ignoredSize;
	size_t internalCount;
	size_t internalSize;

	void addAllocation( size_t sizeUsed, uint32 flags )
	{
		if (flags & MemoryDebug::AF_ALLOCATION_INTERNAL)
		{
			++internalCount;
			internalSize += sizeUsed;
		}
		else
		{
			++liveCount;
			liveSize += sizeUsed;

			if (flags & MemoryDebug::AF_ALLOCATION_IGNORELEAK)
			{
				++ignoredCount;
				ignoredSize += sizeUsed;
			}
		}
	}

};
typedef BW::map<
	CallstackAddressType, 
	CacheGrindEventInfo, 
	std::less<CallstackAddressType>,
	detail::DebugStlAllocator< std::pair<CallstackAddressType, 
		CacheGrindEventInfo > > > CacheGrindEventMap;
typedef BW::map<const char *, 
	CacheGrindEventInfo, 
	std::less<const char *>, 
	detail::DebugStlAllocator< std::pair<const char *, 
		CacheGrindEventInfo > > > CacheGrindCalleeEventMap;

class CacheGrindCalleeEventInfo
{
public:
	CacheGrindCalleeEventInfo( FixedSizedAllocator * allocator ) : 
		events( CacheGrindCalleeEventMap::key_compare(),
			detail::DebugStlAllocator< CacheGrindCalleeEventMap::value_type >( 
				allocator ))
	{}

	const void* iterationToken_;
	CacheGrindCalleeEventMap events;
};
typedef BW::map<
	CallstackAddressType, 
	CacheGrindCalleeEventInfo, 
	std::less<CallstackAddressType>, 
	detail::DebugStlAllocator< std::pair<CallstackAddressType, 
		CacheGrindCalleeEventInfo > > > CacheGrindCalleeMap;

class CacheGrindFrameInfo
{
public:
	CacheGrindFrameInfo( FixedSizedAllocator * allocator ) : 
		callees( CacheGrindCalleeMap::key_compare(), 
			detail::DebugStlAllocator< CacheGrindCalleeMap::value_type >( 
				allocator ) ),
		events( CacheGrindEventMap::key_compare(),
			detail::DebugStlAllocator< CacheGrindEventMap::value_type >( 
				allocator ) )
	{}

	CacheGrindCalleeMap callees;
	CacheGrindEventMap events;
	CallstackAddressType address;
};
typedef BW::map<const char *, 
	CacheGrindFrameInfo, 
	std::less<const char *>, 
	detail::DebugStlAllocator< 
		std::pair<const char *, CacheGrindFrameInfo > > > CacheGrindFrameMap;

}

//-----------------------------------------------------------------------------

CallstackAddressType* MemoryDebug::CallstackHeader::callstack( 
	MemoryDebug::CallstackHeader* hdr )
{
	return const_cast< CallstackAddressType* >( callstack( 
		const_cast< const CallstackHeader* >( hdr ) ) );
}

//-----------------------------------------------------------------------------

const CallstackAddressType* MemoryDebug::CallstackHeader::callstack( 
	const MemoryDebug::CallstackHeader* hdr )
{
	return reinterpret_cast< const CallstackAddressType* >( 
		reinterpret_cast< const uint8* >( hdr ) + CALLSTACK_HEADER_SIZE + 
		(hdr->guardDepth * GUARD_ITEM_SIZE) );
}

//-----------------------------------------------------------------------------

StackTracker::StackItem* MemoryDebug::CallstackHeader::guardstack( 
	MemoryDebug::CallstackHeader* hdr )
{
	return const_cast< StackTracker::StackItem* >( guardstack( 
		const_cast< const MemoryDebug::CallstackHeader* >( hdr ) ) );
}

//-----------------------------------------------------------------------------

const StackTracker::StackItem* MemoryDebug::CallstackHeader::guardstack( 
	const MemoryDebug::CallstackHeader* hdr )
{
	return reinterpret_cast< const StackTracker::StackItem* >( 
		reinterpret_cast< const uint8* >( hdr ) + CALLSTACK_HEADER_SIZE );
}

//-----------------------------------------------------------------------------

std::size_t MemoryDebug::CallstackHeader::totalSize() const
{
	return CALLSTACK_HEADER_SIZE + (GUARD_ITEM_SIZE * guardDepth) +
		(CALLSTACK_ITEM_SIZE * depth);
}

//-----------------------------------------------------------------------------

std::size_t MemoryDebug::HashCallstackHeaderPtr::operator()( 
	const MemoryDebug::CallstackHeader* hdr ) const
{
	return hash_string( hdr, hdr->totalSize() );
}

//-----------------------------------------------------------------------------

bool MemoryDebug::EqualsCallstackHeaderPtr::operator()( 
	const MemoryDebug::CallstackHeader* lhs, 
	const MemoryDebug::CallstackHeader* rhs ) const
{
	return (lhs->totalSize() == rhs->totalSize()) && 
		(memcmp( lhs, rhs, lhs->totalSize() ) == 0);
}

//-----------------------------------------------------------------------------

std::size_t MemoryDebug::HashCStr::operator()( const char* str ) const
{
	return hash_string( str );
}

//-----------------------------------------------------------------------------

bool MemoryDebug::EqualsCStr::operator()( const char* lhs, 
	const char* rhs ) const
{
	return strcmp( lhs, rhs ) == 0;
}

//-----------------------------------------------------------------------------

class MemoryDebug::AllocationStats
{
public:

	struct SlotStat
	{
		size_t liveAllocationCount_;
		size_t liveAllocationBytes_;
		size_t ignoreLeaksAllocationCount_;
		size_t ignoreLeaksAllocationBytes_;
		size_t peakAllocationBytes_;
		size_t totalAllocations_;
		const char* name_;
	};

	typedef BW_NAMESPACE StaticArray< SlotStat, 
		BW::ThreadSlotTracker::MAX_SLOTS > SlotStats;

	SlotStat heapStats_; // Heap only stats (Not including allocations from fixed sized pools)
	SlotStat globalStats_; // Global allocation stats (via Allocator::allocate calls, not including internal fixed sized pool memory)
	SlotStat poolStats_;  // Pool allocation stats
	SlotStat internalStats_;  // Internal allocation stats
	SlotStats slotStats_; // Slot allocation stats (via Allocator::allocate calls, not including internal fixed sized pool memory)

	void incBytesSlot( SlotStat& slot, size_t size, bool ignoreLeaks )
	{
		++slot.liveAllocationCount_;
		slot.liveAllocationBytes_ += size;

		if (ignoreLeaks)
		{
			++slot.ignoreLeaksAllocationCount_;
			slot.ignoreLeaksAllocationBytes_ += size;
		}

		++slot.totalAllocations_;
		if (slot.liveAllocationBytes_ > slot.peakAllocationBytes_)
		{
			slot.peakAllocationBytes_ = slot.liveAllocationBytes_;
		}
	}

	void decBytesSlot( SlotStat& slot, size_t size, bool ignoreLeaks )
	{
		--slot.liveAllocationCount_;
		slot.liveAllocationBytes_ -= size;

		if (ignoreLeaks)
		{
			--slot.ignoreLeaksAllocationCount_;
			slot.ignoreLeaksAllocationBytes_ -= size;
		}
	}

	void debugReportSlot( const SlotStat& slot )
	{
		BW_NAMESPACE dprintf( " %s\n", slot.name_ );
		BW_NAMESPACE dprintf( " Peak memory usage  = %ld MB\n", 
			slot.peakAllocationBytes_ / (1024 * 1024) );
		BW_NAMESPACE dprintf( " Total allocations  = %ld\n", 
			slot.totalAllocations_ );
	}

	void reportSlotHeader(FILE* f)
	{
		fprintf( f, "Slot name, Peak allocation (bytes), Total allocation count, "
			"Live allocation (bytes), Live allocation count, "
			"Ignored leaks (bytes), Ignored leak count, "
			"Memory leaked (bytes), Memory leak count \n" );
	}

	void reportSlotToFile(FILE* f, const SlotStat& slot)
	{
		const char* name = slot.name_;
		if (!name)
			name = "Unknown Slot";

		size_t leakedBytes = slot.liveAllocationBytes_ - 
			slot.ignoreLeaksAllocationBytes_;
		size_t leakedCount = slot.liveAllocationCount_ - 
			slot.ignoreLeaksAllocationCount_;

		// Output slot columns
		fprintf( f, "\"%s\", %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu \n", 
			name,
			(unsigned long) slot.peakAllocationBytes_,
			(unsigned long) slot.totalAllocations_,
			(unsigned long) slot.liveAllocationBytes_,
			(unsigned long) slot.liveAllocationCount_,
			(unsigned long) slot.ignoreLeaksAllocationBytes_,
			(unsigned long) slot.ignoreLeaksAllocationCount_,
			(unsigned long) leakedBytes,
			(unsigned long) leakedCount );
	}

	inline void initSlots( int slot )
	{
		const size_t startSize = slotStats_.size();
		const size_t endSize = slot + 1;
		if ( startSize < endSize )
		{
			slotStats_.resize( endSize );
			for ( size_t i = startSize; i < endSize; ++i )
			{
				SlotStat& newSlot = slotStats_[i];
				memset( &newSlot, 0, sizeof(SlotStat) );
				newSlot.name_ = 
					BW::ThreadSlotTracker::instance().slotName( (uint)i );
			}
		}
	}

public:

	AllocationStats()
	{
		memset( &globalStats_, 0, sizeof(globalStats_) );
		globalStats_.name_ = "Total allocations (Heap + Pool + Internal)";

		memset( &heapStats_, 0, sizeof(heapStats_) );
		heapStats_.name_ = "Heap allocations (Pool misses)";

		memset( &poolStats_, 0, sizeof(poolStats_) );
		poolStats_.name_ = "Pool allocations";

		memset( &internalStats_, 0, sizeof(internalStats_) );
		internalStats_.name_ = "Internal allocations (Pool buffers)";

		// This means:
		// Heap+Pool+Internal = Total. 
	}

	void incBytes( size_t size, int slot, uint32 flags )
	{
		bool ignoreLeak = (flags & AF_ALLOCATION_IGNORELEAK) != 0;

		this->incBytesSlot( globalStats_, size, ignoreLeak );

		if ((flags & AF_ALLOCATION_INTERNAL))
		{
			this->incBytesSlot( internalStats_, size, ignoreLeak );
		}
		else 
		{
			if ((flags & AF_ALLOCATION_POOL))
			{
				this->incBytesSlot( poolStats_, size, ignoreLeak );
			}
			else
			{
				this->incBytesSlot( heapStats_, size, ignoreLeak );
			}

			this->initSlots( slot );
			this->incBytesSlot( slotStats_[slot], size, ignoreLeak );
		}
	}

	void decBytes( size_t size, int slot, uint32 flags )
	{
		bool ignoreLeak = (flags & AF_ALLOCATION_IGNORELEAK) != 0;

		this->decBytesSlot( globalStats_, size, ignoreLeak );

		if ((flags & AF_ALLOCATION_INTERNAL))
		{
			this->decBytesSlot( internalStats_, size, ignoreLeak );
		}
		else 
		{
			if ((flags & AF_ALLOCATION_POOL))
			{
				this->decBytesSlot( poolStats_, size, ignoreLeak );
			}
			else
			{
				this->decBytesSlot( heapStats_, size, ignoreLeak );
			}

			this->decBytesSlot( slotStats_[slot], size, ignoreLeak );
		}
	}

	void debugReport()
	{
		BW_NAMESPACE dprintf( "----------------------------------------------------------------------------------\n" );
		this->debugReportSlot( globalStats_ );
		BW_NAMESPACE dprintf( "----------------------------------------------------------------------------------\n" );
		this->debugReportSlot( heapStats_ );
		BW_NAMESPACE dprintf( "----------------------------------------------------------------------------------\n" );
		this->debugReportSlot( poolStats_ );
		BW_NAMESPACE dprintf( "----------------------------------------------------------------------------------\n" );
		this->debugReportSlot( internalStats_ );

		BW_NAMESPACE dprintf( "----------------------------------------------------------------------------------\n" );
		for (size_t i = 0; i < slotStats_.size(); ++i)
		{
			this->debugReportSlot( slotStats_[i] );
			BW_NAMESPACE dprintf( "----------------------------------------------------------------------------------\n" );
		}
	}

	void readAllocationStats(  BW::Allocator::AllocationStats& stats )
	{
		stats.currentBytes_ = globalStats_.liveAllocationBytes_ - 
			internalStats_.liveAllocationBytes_;
		stats.currentAllocs_ = globalStats_.liveAllocationCount_ - 
			internalStats_.liveAllocationCount_;
	}

	const SlotStat& getHeapStats() const { return heapStats_; }
	const SlotStat& getGlobalStats() const { return globalStats_; }
	const SlotStats& getSlotStats() const { return slotStats_; }
};

/*static*/ THREADLOCAL( bool ) MemoryDebug::internalAlloc_;
/*static*/ THREADLOCAL( uint ) MemoryDebug::ignoreAllocs_;

//-----------------------------------------------------------------------------

MemoryDebug::MemoryDebug( CallstackMode csMode, bool reportHotspots, 
	bool reportIgnoredLeaks, bool reportCacheGrind ) :
	allocator_( 256 * 1024 ),
	callstacks_( NULL ),
	liveAllocations_( NULL ),
	stringTable_( NULL ),
	lookupCache_( NULL ),
	allocationStats_( NULL ),
	smartPointers_(NULL),
	systemStage_( Allocator::SS_PRE_MAIN ),
	reportHottestCallstacks_( reportHotspots ),
	internalInit_( true ),
	showDetailedCallstackForGuard_( false ),
	reportIgnoredLeaks_( reportIgnoredLeaks ),
	reportCacheGrind_( reportCacheGrind ),
	curAllocId_( 0 ),
	totalStacksGenerated_( 0 ),
	totalStacksGeneratedSlow_( 0 ),
	callstackMode_( csMode )
{}

//-----------------------------------------------------------------------------

MemoryDebug::~MemoryDebug()
{
	internalAlloc_ = true;
	if (!internalInit_)
	{
		allocator_.printUsageStats();
#if ENABLE_FIXED_SIZED_POOL_STATISTICS
		debugAllocator_->debugPrintStatistics();
#endif
		allocationStats_->~AllocationStats();
		liveAllocations_->~LiveAllocationMap();
		lookupCache_->~LookupCacheMap();
		stringTable_->~HashStringMap();
		callstacks_->~CallstackMap();
#if ENABLE_SMARTPOINTER_TRACKING
		smartPointers_->~SmartPointerMap();
#endif
		debugAllocator_->~FixedSizedAllocator();
		allocationStats_ = 0;
		liveAllocations_ = 0;
		lookupCache_ = 0;
		stringTable_ = 0;
		callstacks_ = 0;
		smartPointers_ = 0;
		debugAllocator_ = 0;
	}
}

//------------------------------------------------------------------------------------

void MemoryDebug::init()
{
	if (internalInit_)
	{
		internalInit_ = false;

		BW_SCOPED_BOOL( internalAlloc_ );

		debugAllocator_ = new(debugAllocatorMem_) FixedSizedAllocator( 256, 
			512 * 1024, "MemoryDebug" );
		debugAllocator_->setLargeAllocationHooks(
			&Allocator::heapAllocate, 
			&Allocator::heapDeallocate, 
			&Allocator::heapReallocate, 
			&Allocator::heapMemorySize );

		callstacks_ = new( callstacksMem_ ) CallstackMap( 256 * 1024,
			HashCallstackHeaderPtr(),
			EqualsCallstackHeaderPtr(),
			detail::DebugStlAllocator< CallstackMap::value_type>( 
				debugAllocator_ ) );

		liveAllocations_ = new( liveAllocationsMem_ ) LiveAllocationMap( 
			256 * 1024, BW::hash< const void* >(), 
			std::equal_to< const void* >(),
			detail::DebugStlAllocator< LiveAllocationMap::value_type>( 
				debugAllocator_ ) );

		stringTable_ = new( stringTableMem_ ) HashStringMap( 256 * 1024,
			HashCStr(), EqualsCStr(), detail::DebugStlAllocator< 
			std::pair< const char*, size_t > >( debugAllocator_ ) );

		lookupCache_ = new ( lookupCacheMem_ ) LookupCacheMap( 1 * 1024,
			BW::hash< const void* >(), std::equal_to< const void* >(),
			detail::DebugStlAllocator< LookupCacheMap::value_type >(
				debugAllocator_ ) );

#if ENABLE_SMARTPOINTER_TRACKING
		smartPointers_ = new ( smartPointersMem_ ) SmartPointerMap( 256 * 1024,
			BW::hash< const void* >(), std::equal_to< const void* >(),
			detail::DebugStlAllocator< std::pair< const void *, 
				SmartPointerInfo > >( debugAllocator_ ) );
#endif

		allocationStats_ = new ( allocator_.allocate( 
			sizeof(AllocationStats) ) ) AllocationStats;

#if BW_ENABLE_STACKTRACE
		// check symbol resolution is available
		initStacktraces();

		if (stacktraceSymbolResolutionAvailable())
		{
#if defined( _WINDOWS_ )
			// generate a complete callstack from this point and record the 
			// head of the stack, we use this address to figure out if the fast
			// callstack method made it to the top or not
			CallstackAddressType callstack[64];
			int depth = stacktraceAccurate( callstack, 
				ARRAY_SIZE( callstack ) );

			const char* mainFuncName = "BaseThreadInitThunk";

			for (int i = 0; i < depth; ++i)
			{
				char name[SYMBOL_STRING_MAX_LENGTH] = { 0 };
				if ( convertAddressToFunction( callstack[i], name, 
					ARRAY_SIZE(name), NULL, 0, NULL ) )
				{
					if ( strstr( name, mainFuncName ) != 0 )
					{
						callstackTop_ = callstack[ i ];
						break;
					}
				}
			}
#endif // _WINDOWS_
		}
		else if (callstackMode_ != CM_BW_GUARD)
		{
			dprintf( " MemoryDebug : Disabling callstacks as symbol resolution is not available, reverting to using BW_GUARD mode\n" );
			callstackMode_ = CM_BW_GUARD;
		}
#endif // BW_ENABLE_STACKTRACE

		// remember what out memory was like on startup
#if defined( _WINDOWS_ )
		initialProcessMemoryInfo_ = getProcessMemoryInfo();
#endif // defined( _WINDOWS_ )
	}
}

//-----------------------------------------------------------------------------

void MemoryDebug::setSystemStage( Allocator::SystemStage stage )
{
	SimpleMutexHolder smh( lock_ );
	systemStage_ = stage;
}

//-----------------------------------------------------------------------------

void MemoryDebug::showDetailedStackForGuard( const char * str )
{
	strncpy( detailedCallstackForGuardString_, str, 
		ARRAY_SIZE( detailedCallstackForGuardString_ ) );
	showDetailedCallstackForGuard_ = true;
}

//-----------------------------------------------------------------------------
uint MemoryDebug::recordGuardCallstack(StackTracker::StackItem* guardStack,
	uint length, bool * requestDetailedCallstack)
{
	int guardDepth = std::min(StackTracker::stackSize(), length);
	for (int i = 0; i < guardDepth; ++i)
	{
		StackTracker::StackItem& stackItem = guardStack[i];
		stackItem = StackTracker::getStackItem( i );

		// make a copy of the temporary strings before generating hash for the
		// callstack
		if ( stackItem.temp )
		{
			stackItem.name = getStringTableEntry( stackItem.name );
			stackItem.file = getStringTableEntry( stackItem.file );
		}

		// make a copy of the annotation
		if ( stackItem.annotation )
		{
			stackItem.annotation = getStringTableEntry( stackItem.annotation );
		}

		if (showDetailedCallstackForGuard_ && 
			requestDetailedCallstack)
		{
			if (strstr( guardStack[i].name, detailedCallstackForGuardString_ )
				!= 0)
			{
				*requestDetailedCallstack = true;
			}
		}
	}
	return guardDepth;
}

//-----------------------------------------------------------------------------

void MemoryDebug::allocate( const void * ptr, size_t sizeUsed, 
	size_t sizeRequested, uint32 allocFlags )
{
	init();

	if( !ptr )
	{
		return;
	}

	// don't track allocations from us
	if (internalAlloc_)
	{
		return;
	}

	// IF_NOTRACK_ALLOC means the allocation shouldn't be tracked, usually because
	// it's already been tracked by Allocator::allocate and again through
	// Allocator::heapAllocate.
	MF_ASSERT_NOALLOC( !(allocFlags & Allocator::IF_NOTRACK_ALLOC) );

	SimpleMutexHolder smh( lock_ );
	BW_SCOPED_BOOL( internalAlloc_ );
	
	std::pair< const MemoryDebug::CallstackHeader * const, 
		MemoryDebug::CallstackStats > & csPair = 
		getCallstackEntry( CS_NONE, 2 );

	MF_ASSERT_NOALLOC( (csPair.first->flags & CS_SMARTPOINTER) == 0 );

	// inc callstack count
	++csPair.second.allocCount;

	// get the slot
	int slot = ThreadSlotTracker::instance().currentThreadSlot();

	// add to live allocations
	AllocationInfo info;
	info.callstack = csPair.first;
	info.slot = slot;
	info.id = curAllocId_++;
	info.sizeUsed = static_cast<int32>(sizeUsed);
	info.flags = (systemStage_ == Allocator::SS_PRE_MAIN) ? AF_ALLOCATION_PREMAIN : 0;
	info.flags |= (allocFlags & Allocator::IF_POOL_ALLOC) ? AF_ALLOCATION_POOL : 0;
	info.flags |= (allocFlags & Allocator::IF_INTERNAL_ALLOC) ? AF_ALLOCATION_INTERNAL | AF_ALLOCATION_IGNORELEAK : 0;
	info.flags |= (ignoreAllocs_ != 0) ? AF_ALLOCATION_IGNORELEAK : 0;

	liveAllocations_->insert( std::make_pair( ptr, info ) );

	// add to stats
	allocationStats_->incBytes( sizeUsed, slot, info.flags );
}

//-----------------------------------------------------------------------------

void MemoryDebug::deallocate( const void * ptr )
{
	if (internalAlloc_ || !ptr)
	{
		return;
	}

	SimpleMutexHolder smh( lock_ );
	BW_SCOPED_BOOL( internalAlloc_ );
	
	LiveAllocationMap::iterator it = liveAllocations_->find( ptr );

	if (it != liveAllocations_->end())
	{
		// update stats
		allocationStats_->decBytes( it->second.sizeUsed, it->second.slot, 
			it->second.flags );

		liveAllocations_->erase( it );
	}
	else
	{
		MF_ASSERT_NOALLOC( false && "Live allocation mismatch" );
	}
}

//-----------------------------------------------------------------------------

#if defined( _WINDOWS_ )
MemoryDebug::ProcessMemoryInfo MemoryDebug::getProcessMemoryInfo()
{
	MemoryDebug::ProcessMemoryInfo procMemInfo;

	HANDLE proc = ::GetCurrentProcess();

	// walk our process space and check to see what is responsible for each 
	// page allocation
	void * curAddr = 0;
	MEMORY_BASIC_INFORMATION memInfo;
	while (1)
	{
		if( !VirtualQueryEx ( proc, curAddr, &memInfo, sizeof(memInfo) ))
			break;

		switch( memInfo.Type )
		{
			case MEM_IMAGE : 
				procMemInfo.imageBytes += memInfo.RegionSize; break;
			case MEM_MAPPED : 
				procMemInfo.mappedBytes += memInfo.RegionSize; break;
			case MEM_PRIVATE : 
				procMemInfo.privateBytes += memInfo.RegionSize; break;
		}
		curAddr = (char*) curAddr + memInfo.RegionSize;
	}

	MEMORYSTATUSEX memoryStatus = { sizeof( memoryStatus ) };
	::GlobalMemoryStatusEx( &memoryStatus );

	procMemInfo.virtualAddressAvailable = (size_t)memoryStatus.ullTotalVirtual;
	procMemInfo.virtualAddressUsed = (size_t)memoryStatus.ullAvailVirtual;

	return procMemInfo;
}
#endif // defined( _WINDOWS_ )

//-----------------------------------------------------------------------------

std::pair< const MemoryDebug::CallstackHeader * const, 
	MemoryDebug::CallstackStats > & MemoryDebug::getCallstackEntry( uint8 flags,
	size_t skipFrames )
{
	uint8 callstackBuffer[MAX_CALLSTACK_SIZE];
	CallstackHeader* tempHdr = 
		reinterpret_cast< CallstackHeader* >( callstackBuffer );

	memset( tempHdr, 0, sizeof( *tempHdr ) );
	tempHdr->flags = flags;

	// we always collect BW_GUARD info 
	bool debugCompleteStackRequested = false;
	StackTracker::StackItem* guardStack = 
		CallstackHeader::guardstack( tempHdr );
	tempHdr->guardDepth = recordGuardCallstack( guardStack, MAX_GUARD_DEPTH, 
		&debugCompleteStackRequested);

	bool getCompleteStack = (callstackMode_ == CM_CALLSTACKS_COMPLETE) ||
		(tempHdr->guardDepth == 0) || debugCompleteStackRequested;

	tempHdr->depth = 0;
#if BW_ENABLE_STACKTRACE
	// get stacktrace for this allocation
	CallstackAddressType* callstack = CallstackHeader::callstack( tempHdr );

	// if mode is CM_BW_GUARD we do not attempt to collect any callstack info
	// unless there are no guards on the stack, in this case we just grab a 
	// quick stacktrace.
	if (callstackMode_ != CM_BW_GUARD || getCompleteStack)
	{
		// first we attempt to use the fast method and see if this makes it to
		// the top of the stack as recorded when the system was initialised, 
		// if it didn't we back to the slower method
		tempHdr->depth = stacktrace( callstack, MAX_CALLSTACK_DEPTH, skipFrames );
		++totalStacksGenerated_;

#if defined( _WINDOWS_ )
		// ideally we would potentially gather an accurate one but this is too
		// slow in the general case
		if (callstackMode_ != CM_BW_GUARD )
		{
			bool hasTop = false;
			for (int i = 0; i < tempHdr->depth ; ++i)
			{
				uint64 dist = (uint64)callstack[i] - (uint64)callstackTop_;
				// callstack points to a return address within the function so
				// see if we are near the top function
				if (dist < 1024)
				{
					hasTop = true;
					break;
				}
			}

			if (!hasTop && (callstackMode_ == CM_CALLSTACKS_COMPLETE || 
				getCompleteStack))
			{
				++totalStacksGeneratedSlow_;
				tempHdr->depth = stacktraceAccurate( callstack, 
					MAX_CALLSTACK_DEPTH );
			}
		}
#endif // _WINDOWS_
	}

#endif // BW_ENABLE_STACKTRACE

	if (getCompleteStack)
	{
		tempHdr->flags |= CS_ALLOCATION_HAS_COMPLETE;
	}

	// see if we already have this stack, if not add it
	CallstackMap::iterator it = callstacks_->find( tempHdr );
	if (it == callstacks_->end())
	{
		const size_t totalSize = tempHdr->totalSize();
		CallstackHeader * newHdr = (CallstackHeader *)allocator_.allocate( 
			totalSize );
		memcpy( newHdr, tempHdr, totalSize );
		CallstackStats stats;
		stats.allocCount = 0;
#ifdef MF_SERVER
		gettimeofday( &stats.firstTime, NULL );
#endif // MF_SERVER
		std::pair< CallstackMap::iterator, bool > insert = 
			callstacks_->insert( std::make_pair( newHdr, stats ) );
		MF_ASSERT_NOALLOC( insert.second );
		it = insert.first;
	}
	else
	{
		// Check for collisions
		// Make sure the data is the same length
		MF_ASSERT_NOALLOC( it->first->depth == tempHdr->depth );
		MF_ASSERT_NOALLOC( it->first->guardDepth == tempHdr->guardDepth );

		// Make sure the callstacks are equal in data 
#if BW_ENABLE_STACKTRACE
		MF_ASSERT_NOALLOC( memcmp( &callstack[0], 
			CallstackHeader::callstack( it->first ), 
			sizeof(callstack[0]) * it->first->depth ) == 0 );
#endif // BW_ENABLE_STACKTRACE
		MF_ASSERT_NOALLOC( memcmp( &guardStack[0], 
			CallstackHeader::guardstack( it->first ), 
			sizeof(guardStack[0]) * it->first->guardDepth ) == 0 );
	}

	return *it;
}

//-----------------------------------------------------------------------------

const char* MemoryDebug::getStringTableEntry( const char* str )
{
	HashStringMap::const_iterator itr = stringTable_->find( str );
	if (itr != stringTable_->end())
		return *itr;

	size_t len = strlen( str );
	char* entry = static_cast<char*>( allocator_.allocate( len + 1 ) );
	memcpy( entry, str, len + 1 );
	stringTable_->insert( entry );
	return entry;
}

//-----------------------------------------------------------------------------

#if BW_ENABLE_STACKTRACE
bool MemoryDebug::cachedResolve( const CallstackAddressType address,
		const char ** ppFuncname, const char ** ppFilename, int * pLineno )
{
	AddressLookupCacheEntry & entry = (*lookupCache_)[ address ];
	bool resolveFunc = ppFuncname != NULL;
	bool resolveFilename = ppFilename != NULL;
	bool resolveLineno = pLineno != NULL;

	if (ppFuncname && entry.funcname != NULL)
	{
		*ppFuncname = entry.funcname;
		resolveFunc = false;
	}

	if (ppFilename && entry.filename != NULL)
	{
		*ppFilename = entry.filename;
		resolveFilename = false;
	}

	if (pLineno && entry.lineno != -1)
	{
		*pLineno = entry.lineno;
		resolveLineno = false;
	}

	if (resolveFunc || resolveFilename || resolveLineno)
	{
		char funcBuffer[SYMBOL_STRING_MAX_LENGTH] = { 0 };
		char fileBuffer[SYMBOL_STRING_MAX_LENGTH] = { 0 };

		if (convertAddressToFunction( address, 
			resolveFunc ? funcBuffer : NULL,
			resolveFunc ? ARRAY_SIZE(funcBuffer) : 0,
			resolveFilename ? fileBuffer : NULL, 
			resolveFilename ? ARRAY_SIZE(fileBuffer) : 0,
			resolveLineno ? &entry.lineno : NULL ))
		{
			if (resolveFunc)
			{
				*ppFuncname = entry.funcname =
						getStringTableEntry( funcBuffer );
			}

			if (resolveFilename)
			{
				*ppFilename = entry.filename =
					getStringTableEntry( fileBuffer );
			}

			if (resolveLineno)
			{
				if (entry.lineno == -1)
				{
					// 0 = failed resolve, -1 = unresolved
					entry.lineno = 0;
				}
				*pLineno = entry.lineno;
			}
			entry.validAddress = true;
		}
		else
		{
			entry.lineno = 0;
			entry.funcname = "<unknown address>";
			entry.filename = "";
			entry.validAddress = false;
		}
	}

	return entry.validAddress;
}
#endif // BW_ENABLE_STACKTRACE

//-----------------------------------------------------------------------------

struct CallstackReport
{
	const MemoryDebug::CallstackHeader* callstack;
	uint32 leakSize;
	uint32 numAllocations;
	uint32 numIgnoredAllocations;
#ifdef MF_SERVER
		timeval firstTime;	// first time the allocation was performed
#endif // MF_SERVER
};

//-----------------------------------------------------------------------------

bool callstackSorter( const CallstackReport & lhs, const CallstackReport & rhs )
{
	return lhs.leakSize > rhs.leakSize;
}

//-----------------------------------------------------------------------------

bool sortCallstacksByAllocCount( const CallstackStatPair& lhs, 
	const CallstackStatPair& rhs )
{
	return lhs.second.allocCount > rhs.second.allocCount;
}

//-----------------------------------------------------------------------------

void MemoryDebug::formatShortGuardStack( const StackTracker::StackItem* items, 
	size_t numItems, BW::StringBuilder& builder )
{
	for (size_t i = 0; i < numItems; ++i)
	{
		const StackTracker::StackItem& item = items[i];
		builder.append( item.name );
		if (item.annotation)
		{
			builder.append( "@" );
			builder.append( item.annotation );
		}
		if (i != numItems - 1)
		{
			builder.append( " <- " );
		}
	}
}

//-----------------------------------------------------------------------------

#if BW_ENABLE_STACKTRACE
void MemoryDebug::formatShortCallstack( const CallstackAddressType* callstack,
	size_t numItems, BW::StringBuilder& builder )
{
	for (size_t i = 0, depth = numItems; i < depth; ++i)
	{
		const char * funcName;
		this->cachedResolve( callstack[i], &funcName );
		builder.append( funcName );
		if (i != numItems - 1)
		{
			builder.append( " <- " );
		}
	}
}
#endif // BW_ENABLE_STACKTRACE

//-----------------------------------------------------------------------------

void MemoryDebug::formatLongGuardStack( const StackTracker::StackItem * items,
	size_t numItems, BW::StringBuilder & builder )
{
	for (size_t i = 0; i < numItems; ++i)
	{
		const StackTracker::StackItem& item = items[i];
		builder.appendf( "%s(%d) : %s", item.file, item.line, item.name );
		if (item.annotation)
		{
			builder.append( "@" );
			builder.append( item.annotation );
		}
		builder.append( "\n" );
	}
}

//-----------------------------------------------------------------------------
#if BW_ENABLE_STACKTRACE
void MemoryDebug::formatLongCallstack( const CallstackAddressType * callstack,
	int depth, StringBuilder & builder )
{
	for (int i = 0; i < depth; ++i)
	{
		int line;
		const char * name;
		const char * file;
		if (this->cachedResolve( callstack[i], &name, &file, &line ))
		{
			builder.appendf( "%s(%d) : %s\n", file, line, name );
		}
		else
		{
			builder.append( "<unknown address>\n" );
		}
	}
}
#endif // BW_ENABLE_STACKTRACE

//-----------------------------------------------------------------------------

void MemoryDebug::saveAllocationsToFile(const char* filename)
{
	BW_SCOPED_BOOL( internalAlloc_ );

	FILE* f = fopen( filename, "wb+" );
	if (f)
	{
		SimpleMutexHolder smh( lock_ );

		// reuse string builder buffer
		char buffer[8192];
		StringBuilder builder( buffer, ARRAY_SIZE(buffer) );

		if (callstacks_)
		{
			// save hashes
			fprintf( f, "%" PRIzu "\n", callstacks_->size() );
			for (CallstackMap::const_iterator itr = callstacks_->begin(), 
				end = callstacks_->end(); itr != end; ++itr)
			{
				const CallstackHeader* hdr = itr->first;
				builder.clear();
#if BW_ENABLE_STACKTRACE
				if ((callstackMode_ != CM_BW_GUARD) || 
					(hdr->flags & CS_ALLOCATION_HAS_COMPLETE))
				{
					formatShortCallstack( CallstackHeader::callstack( hdr ), 
						hdr->depth, builder );
				}
				else
#endif // BW_ENABLE_STACKTRACE
				{
					formatShortGuardStack( CallstackHeader::guardstack( hdr ), 
						hdr->guardDepth, builder );
				}
				fprintf( f, "%p;%s\n", hdr, builder.string() );
			}
		}

		for (LiveAllocationMap::const_iterator itr = liveAllocations_->begin(), 
			end = liveAllocations_->end(); itr != end; ++itr )
		{
			const AllocationInfo& info = itr->second;
			const char* slotName = ThreadSlotTracker::instance().slotName( 
				info.slot );
			fprintf( f, "%s;%p;%d\n", slotName, info.callstack, 
				info.sizeUsed );
		}

		fclose(f);
	}
}

//-----------------------------------------------------------------------------
bool MemoryDebug::matchIgnoredFunction( const char * functionName )
{
	const char * ignoredFunctions[] = {
		"BW::MemoryDebug::allocate",
		"BW::Allocator::allocate",
		"BW::Allocator::reallocate",
		NULL
		};

	const char ** stringIter = &ignoredFunctions[0];
	while (*stringIter != NULL)
	{
		const char * curString = *stringIter++;
		if (strncmp(functionName, curString, strlen(curString)) == 0)
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
void MemoryDebug::saveAllocationsToCacheGrindFile( const char * filename )
{
	dprintf( "----------------------------------------------------------------------------------\n" );
#if BW_ENABLE_STACKTRACE
	dprintf( " Generating CacheGrind output: %s\n", filename );

	SimpleMutexHolder smh( lock_ );
	BW_SCOPED_BOOL( internalAlloc_ );
	
	// Avoid "most vexing parse" issue by declaring frameMapKeyCompare variable
	// before usage instead of inlining it into the frameMap constructor.
	CacheGrindFrameMap::key_compare frameMapKeyCompare;
	CacheGrindFrameMap frameMap( frameMapKeyCompare,
		detail::DebugStlAllocator< CacheGrindFrameMap::value_type >( 
			debugAllocator_ ) );

	if (callstacks_ == NULL ||
		liveAllocations_ == NULL)
	{
		dprintf( " Generation failed. No callstacks available.\n");
		dprintf( "----------------------------------------------------------------------------------\n" );
		return;
	}

	// Iterate over all the stack traces building up the frame info map
	for (CallstackMap::const_iterator itr = callstacks_->begin(), 
		end = callstacks_->end(); itr != end; ++itr)
	{
		const CallstackHeader* hdr = itr->first;

		// skip smart pointer callstacks
		if (hdr->flags & CS_SMARTPOINTER)
		{
			continue;
		}

		const CallstackAddressType* callstack = 
			CallstackHeader::callstack( hdr );
		size_t allocCount = itr->second.allocCount;
		const void* iterationToken_ = hdr;

		CallstackAddressType prevAddress = NULL;
		const char * prevName = NULL;
		for (int i = 0; i < hdr->depth; ++i)
		{
			const CallstackAddressType& address = callstack[i];

			const char* name = NULL;

			if (!this->cachedResolve( address, &name ))
				continue;

			// Remove memory tracking stack info from the top of the stack.
			if (matchIgnoredFunction( name ) && 
				prevAddress == NULL)
			{
				continue;
			}

			CacheGrindFrameMap::iterator findFrame = 
				frameMap.insert( std::make_pair( name, 
					CacheGrindFrameInfo( debugAllocator_ ) ) ).first;

			CacheGrindFrameInfo& frame = findFrame->second;
			frame.address = address;

			if (prevAddress == NULL)
			{
				frame.events[address].allocCount += allocCount;
			}
			else
			{
				// Add a link:
				CacheGrindCalleeMap::iterator findCallee = 
					frame.callees.insert( std::make_pair( address, 
						CacheGrindCalleeEventInfo( debugAllocator_ ) ) ).first;
				CacheGrindCalleeEventInfo& calleeInfo = findCallee->second;

				CacheGrindCalleeEventMap& calleeMap = calleeInfo.events;
				CacheGrindEventInfo& calleeEventInfo = calleeMap[prevName];

				if (calleeInfo.iterationToken_ != iterationToken_)
				{
					calleeEventInfo.allocCount += allocCount;
				}
				calleeInfo.iterationToken_ = iterationToken_;
			}

			prevName = name;
			prevAddress = address;
		}
	}
	
	// Iterate over all the currently live allocations
	for (LiveAllocationMap::const_iterator itr = liveAllocations_->begin(), 
		end = liveAllocations_->end(); itr != end; ++itr )
	{
		const AllocationInfo& info = itr->second;
		void* iterationToken_ = (void*)&info;

		CallstackMap::iterator stackFind = callstacks_->find( info.callstack );
		if (stackFind == callstacks_->end())
		{
			continue;
		}

		const CallstackHeader* hdr = stackFind->first;

		MF_ASSERT_NOALLOC( (hdr->flags & CS_SMARTPOINTER) == 0 );

		const CallstackAddressType* callstack = 
			CallstackHeader::callstack( hdr );
		
		CallstackAddressType prevAddress = NULL;
		const char * prevName = NULL;
		for (int i = 0; i < hdr->depth; ++i)
		{
			const CallstackAddressType& address = callstack[i];

			const char* name = NULL;
			if (!this->cachedResolve( address, &name ))
			{
				continue;
			}
			
			// Remove memory tracking stack info from the top of the stack.
			if (matchIgnoredFunction( name ) && 
				prevAddress == NULL)
			{
				continue;
			}

			// Attempt to find the frame in the frame map
			CacheGrindFrameMap::iterator findResult = frameMap.find( name );
			if (findResult == frameMap.end())
			{
				continue;
			}

			CacheGrindFrameInfo& frame = findResult->second;

			if (prevAddress == NULL)
			{
				// The first entry in the stack (adjust it's frame information)
				CacheGrindEventInfo& eventInfo = frame.events[address];
				
				eventInfo.addAllocation( info.sizeUsed, info.flags );
			}
			else
			{
				// Update the callee information
				CacheGrindCalleeMap::iterator findCallee = 
					frame.callees.find( address );
				MF_ASSERT( findCallee != frame.callees.end() );
				CacheGrindCalleeEventInfo& calleeInfo = findCallee->second;

				CacheGrindCalleeEventMap& calleeMap = calleeInfo.events;
				CacheGrindEventInfo& calleeEventInfo = calleeMap[prevName];

				if (calleeInfo.iterationToken_ != iterationToken_)
				{
					calleeEventInfo.addAllocation( info.sizeUsed, info.flags );
				}
				calleeInfo.iterationToken_ = iterationToken_;
			}

			prevName = name;
			prevAddress = address;
		}
	}

	// Output the data to file
	FILE * f = fopen( filename, "wb+" );
	if (f)
	{
		fprintf( f, "events: TotalAllocs LiveCount LiveSize IgnoredCount IgnoredSize LeakCount LeakSize InternalCount InternalSize\n");

		for (CacheGrindFrameMap::iterator iter = frameMap.begin(),
			end = frameMap.end(); iter != end; ++iter)
		{
			const char * frameName = iter->first;
			CacheGrindFrameInfo & frame = iter->second;

			// Output the filename of the frame
			const char * filename = NULL;
			if (this->cachedResolve( frame.address, NULL, &filename ) &&
					 *filename != 0)
			{
				// We replace the slashes here, so we copy it to not 
				// corrupt things
				char filenameCopy[BW_MAX_PATH];
				strncpy( filenameCopy, filename, ARRAY_SIZE( filenameCopy ) );
				char * begin = &filenameCopy[0];
				char * end = begin + strlen( filenameCopy );
				std::replace( begin, end, '\\', '/' );
				if (begin != end)
				{
					fprintf( f, "fl=%s\n", filenameCopy );
				}
			}

			fprintf( f, "fn=%s\n", frameName );

			// Output events for each line
			for ( CacheGrindEventMap::iterator iter = frame.events.begin(), 
				end = frame.events.end(); iter != end; ++iter)
			{
				CallstackAddressType address = iter->first;
				CacheGrindEventInfo& events = iter->second;

				// Fetch the line number of the function the events occurred on
				int line = 1;
				this->cachedResolve( address, NULL, NULL, &line );
				
				size_t leakCount = events.liveCount - events.ignoredCount;
				size_t leakSize = events.liveSize - events.ignoredSize;
				fprintf( f, "%d %" PRIzu " %" PRIzu " %" PRIzu " %" PRIzu " %"
						PRIzu " %" PRIzu " %" PRIzu " %" PRIzu " %" PRIzu "\n",
					line, events.allocCount, events.liveCount, events.liveSize,
					events.ignoredCount, events.ignoredSize, leakCount,
					leakSize, events.internalCount, events.internalSize );
			}

			// Output events for each callee 
			for ( CacheGrindCalleeMap::iterator iter = frame.callees.begin(), 
				end = frame.callees.end(); iter != end; ++iter)
			{
				CallstackAddressType address = iter->first;
				CacheGrindCalleeEventMap& calleeMap = iter->second.events;

				// Fetch the line number in the caller function
				int line = 1;
				this->cachedResolve( address, NULL, NULL, &line );

				for ( CacheGrindCalleeEventMap::iterator 
					calleeIter = calleeMap.begin(), end = calleeMap.end();
					calleeIter != end; ++calleeIter)
				{
					const char * calleeName = calleeIter->first;
					CacheGrindEventInfo& events = calleeIter->second;

					fprintf( f, "cfn=%s\n", calleeName );
					fprintf( f, "calls=%d %d\n", 0, 0 );
					
					size_t leakCount = events.liveCount - events.ignoredCount;
					size_t leakSize = events.liveSize - events.ignoredSize;
					fprintf( f, "%d %" PRIzu " %" PRIzu " %" PRIzu " %" PRIzu
							" %" PRIzu " %" PRIzu " %" PRIzu " %" PRIzu " %"
							PRIzu "\n",
						line, events.allocCount, events.liveCount,
						events.liveSize, events.ignoredCount,
						events.ignoredSize, leakCount, leakSize,
						events.internalCount, events.internalSize );
				}
			}
			fprintf( f, "\n\n" );
		}
		fclose( f );
	}
	else
	{
		dprintf( " ERROR: Unable to write to file.\n" );
	}
	dprintf( " Completed.\n" );
#else
	dprintf( " ERROR: CacheGrind output requires compilation with BW_ENABLE_STACKTRACE and ENABLE_MEMORY_DEBUG.\n" );
#endif // BW_ENABLE_STACKTRACE
	dprintf( "----------------------------------------------------------------------------------\n" );
}

//-----------------------------------------------------------------------------
void MemoryDebug::saveStatsToFile(const char* filename)
{
	BW_SCOPED_BOOL( internalAlloc_ );
	
	FILE* f = fopen( filename, "wb+" );
	if (f)
	{
		SimpleMutexHolder smh( lock_ );

		// Print header
		allocationStats_->reportSlotHeader( f );

		// Print global slots:
		allocationStats_->reportSlotToFile( f,
			allocationStats_->globalStats_ );
		allocationStats_->reportSlotToFile( f,
			allocationStats_->heapStats_ );
		allocationStats_->reportSlotToFile( f,
			allocationStats_->poolStats_ );
		allocationStats_->reportSlotToFile( f,
			allocationStats_->internalStats_ );

		// Print each slot:
		for (size_t i = 0; i < allocationStats_->slotStats_.size(); ++i)
		{
			AllocationStats::SlotStat& slot = allocationStats_->slotStats_[i];
			allocationStats_->reportSlotToFile(f, slot);
		}

		fclose(f);
	}
}

//-----------------------------------------------------------------------------

size_t MemoryDebug::printLiveAllocations()
{
	BW_SCOPED_BOOL( internalAlloc_ );
	const uint MAX_LEAKS_TO_SHOW = INT_MAX;
	const uint NUN_HOTTEST_TO_REPORT = 64;

	if ( systemStage_ == Allocator::SS_PRE_MAIN )
	{
		// if this is still premain either the app failed to start or system 
		// stage was not set. this can happen when loading and unloading cstdmf
		// as a dll without calling main - ie when the asset pipeline scans for 
		// converter dlls to dynamically load and finds cstdmf.dll
		return 0;
	}

	SimpleMutexHolder smh( lock_ );

	// go through all allocations and bucket them per callstack
	typedef StaticArray<CallstackReport, 16384> UniqueCallstackMap;
	UniqueCallstackMap uniqueCallstacks( 0 );
	size_t numLeakedMainAllocs = 0;
	size_t numLeakedPreMainAllocs = 0;
	size_t numLeakedIgnoredAllocs = 0;
	size_t numLeakedBytesPreMain = 0;
	size_t numLeakedBytesMain = 0;
	size_t numLeakedBytesIgnored = 0;
	
	for (LiveAllocationMap::iterator it = liveAllocations_->begin(); 
		it != liveAllocations_->end(); ++it)
	{
		const AllocationInfo& info = it->second;
		const CallstackMap::const_iterator csItr = callstacks_->find( 
			info.callstack );
		MF_ASSERT_NOALLOC( csItr != callstacks_->end() );
		const CallstackHeader* csHdr = info.callstack;

		MF_ASSERT_NOALLOC( (csHdr->flags & CS_SMARTPOINTER) == 0 );

		if ((info.flags & AF_ALLOCATION_IGNORELEAK) != 0)
		{
			numLeakedBytesIgnored += info.sizeUsed;
			++numLeakedIgnoredAllocs;
		}

		size_t ucEntry = 0;
		for ( ; ucEntry < uniqueCallstacks.size(); ++ucEntry)
		{
			if (uniqueCallstacks[ucEntry].callstack == csHdr)
				break;
		}

		if (ucEntry == uniqueCallstacks.size())
		{
			if (uniqueCallstacks.size() == uniqueCallstacks.capacity())
			{
				MF_DEBUGPRINT_NOALLOC("Live allocation unique callstack size exeeded");
				break;
			}
			CallstackReport report;
			report.callstack = csHdr;
			report.leakSize = info.sizeUsed;
#ifdef MF_SERVER
			report.firstTime = csItr->second.firstTime;
#endif // MF_SERVER

			if ((info.flags & AF_ALLOCATION_IGNORELEAK) == 0)
			{
				report.numAllocations = 1;
				report.numIgnoredAllocations = 0;
			}
			else
			{
				report.numAllocations = 0;
				report.numIgnoredAllocations = 1;
			}
				
			uniqueCallstacks.push_back( report );
		}
		else
		{
			uniqueCallstacks[ucEntry].leakSize += info.sizeUsed;

			if ((info.flags & AF_ALLOCATION_IGNORELEAK) == 0)
			{
				++uniqueCallstacks[ucEntry].numAllocations;
			}
			else
			{
				++uniqueCallstacks[ucEntry].numIgnoredAllocations;
			}
		}

		if ((info.flags & AF_ALLOCATION_PREMAIN) == 0)
		{
			++numLeakedMainAllocs;
			numLeakedBytesMain += info.sizeUsed;
		}
		else
		{
			++numLeakedPreMainAllocs;
			numLeakedBytesPreMain += info.sizeUsed;
		}
	}

	// Determine the number of leaks detected
	size_t numLeaksDetected = numLeakedPreMainAllocs + numLeakedMainAllocs - 
		numLeakedIgnoredAllocs;

	// cache symbol resolution
	char buffer[16 * 1024];
	StringBuilder builder(buffer, ARRAY_SIZE(buffer));
	if (numLeaksDetected > 0 || (reportIgnoredLeaks_ &&
		((numLeakedPreMainAllocs + numLeakedMainAllocs) > 0)))
	{
		// sort by size of leak per callstack
		std::sort( uniqueCallstacks.begin(), uniqueCallstacks.end(), 
			callstackSorter );

		dprintf( "----------------------------------------------------------------------------------\n" );
		dprintf( " Memory leaks detected\n" );

		if (uniqueCallstacks.size() > MAX_LEAKS_TO_SHOW)
		{
			dprintf( " Capping display to largest %u leaking callstacks\n", 
				MAX_LEAKS_TO_SHOW );
			uniqueCallstacks.resize( MAX_LEAKS_TO_SHOW );
		}

		// now loop over all callstacks and report allocations
		for (int i = 0, size = (int)uniqueCallstacks.size(); i < size; ++i)
		{
			const CallstackReport & report = uniqueCallstacks[i];
		
			if (!reportIgnoredLeaks_ &&
				report.numAllocations == 0)
			{
				continue;
			}

			const CallstackHeader* hdr = report.callstack;
			builder.clear();
#if BW_ENABLE_STACKTRACE
			if ((callstackMode_ != CM_BW_GUARD) || 
				(hdr->flags & CS_ALLOCATION_HAS_COMPLETE))
			{
				formatLongCallstack( CallstackHeader::callstack( hdr ), 
					hdr->depth, builder );
			}
			else
#endif // BW_ENABLE_STACKTRACE
			{
				formatLongGuardStack( CallstackHeader::guardstack( hdr ), 
					hdr->guardDepth, builder );
			}
			dprintf( builder.string() );


			builder.clear();
			builder.appendf( "   Leaked %d bytes from %d allocations",
					report.leakSize, report.numAllocations );

			if (reportIgnoredLeaks_)
			{
				builder.appendf( " (%d ignored)", report.numIgnoredAllocations );
			}

#ifdef MF_SERVER
			char timebuff[32];
			convertTime( timebuff, sizeof( timebuff ), &report.firstTime );
			builder.appendf( ", first time allocated %s", timebuff );
#endif // MF_SERVER
			builder.appendf( "\n\n" );
			dprintf( builder.string() );
		}
	}

	if (reportHottestCallstacks_)
	{
		for (CallstackMap::iterator it = callstacks_->begin(); 
			it != callstacks_->end(); ++it)
		{
			if ((it->first->flags & CS_SMARTPOINTER) == 0)
			{
				s_sortedCallstacks.push_back( *it );
			}
		}
		typedef bool (*CallstackPairSort)(const CallstackStatPair&, 
			const CallstackStatPair&);
		std::sort< CallstackStatPair*, CallstackPairSort >( 
			s_sortedCallstacks.begin(), s_sortedCallstacks.end(), 
			sortCallstacksByAllocCount );
		if (s_sortedCallstacks.size() > NUN_HOTTEST_TO_REPORT )
			s_sortedCallstacks.resize( NUN_HOTTEST_TO_REPORT );

		dprintf( " Top %u hottest callstacks\n", 
			(uint)s_sortedCallstacks.size() );
		dprintf( "----------------------------------------------------------------------------------\n" );
		for(int i = 0, size = (int)s_sortedCallstacks.size(); i < size; ++i)
		{
			const CallstackHeader* hdr = s_sortedCallstacks[i].first;
			builder.clear();
#if BW_ENABLE_STACKTRACE
			if ((callstackMode_ != CM_BW_GUARD) || 
				(hdr->flags & CS_ALLOCATION_HAS_COMPLETE))
			{
				formatLongCallstack( CallstackHeader::callstack( hdr ), 
					hdr->depth, builder );
			}
			else
#endif // BW_ENABLE_STACKTRACE
			{
				formatLongGuardStack( CallstackHeader::guardstack( hdr ), 
					hdr->guardDepth, builder );
			}

			dprintf( builder.string() );
			dprintf( "   %d Allocations\n", 
				s_sortedCallstacks[i].second.allocCount );
			dprintf( "----------------------------------------------------------------------------------\n" );
		}
	}

	dprintf( "----------------------------------------------------------------------------------\n" );
#if BW_ENABLE_STACKTRACE
	//dprintf( " Percentage of callstacks generated the slow way %d%%\n\n", 
	//	(int)((float)totalStacksGeneratedSlow_ / totalStacksGenerated_ * 100) );
#endif

	dprintf( " Number of leaks before main = %u (%.2fmb)\n", 
		(uint)numLeakedPreMainAllocs, 
		float(numLeakedBytesPreMain) / (1024 * 1024) );
	dprintf( " Number of leaks during main = %u (%.2fmb)\n", 
		(uint)numLeakedMainAllocs,
		float(numLeakedBytesMain) / (1024 * 1024) );
	dprintf( " Number of leaks ignored = %u (%.2fmb)\n", 
		(uint)numLeakedIgnoredAllocs,
		float(numLeakedBytesIgnored) / (1024 * 1024) );

	dprintf( "\n" );
	dprintf( " Number of leaks detected = %" PRIzu "\n", numLeaksDetected);
	if (numLeaksDetected > 0)
	{
		dprintf( " ## WARNING: MEMORY LEAKS DETECTED!! ##\n" );
	}

	dprintf( "----------------------------------------------------------------------------------\n" );

#if ENABLE_SMARTPOINTER_TRACKING
	dprintf( "----------------------------------------------------------------------------------\n" );
	dprintf( " Leaked %u Smart Pointers\n", (uint)smartPointers_->size() );
	dprintf( "----------------------------------------------------------------------------------\n" );

	if (!smartPointers_->empty())
	{
		int i = 0;
		for (SmartPointerMap::iterator it = smartPointers_->begin(), 
			end = smartPointers_->end(); it != end; ++it)
		{
			SmartPointerInfo& info = it->second;
			dprintf( " Leak %d : %p \n", i++, info.object );
			const CallstackHeader* hdr = info.callstack;
			MF_ASSERT_NOALLOC( (hdr->flags & CS_SMARTPOINTER) == CS_SMARTPOINTER );
			builder.clear();
#if BW_ENABLE_STACKTRACE
			if ((callstackMode_ != CM_BW_GUARD) || 
				(hdr->flags & CS_ALLOCATION_HAS_COMPLETE))
			{
				formatLongCallstack( CallstackHeader::callstack( hdr ), 
					hdr->depth, builder );
			}
#endif // BW_ENABLE_STACKTRACE
			formatLongGuardStack( CallstackHeader::guardstack( hdr ), 
				hdr->guardDepth, builder );
			dprintf( builder.string() );
			dprintf( "----------------------------------------------------------------------------------\n" );
		}
	}
#endif

	return numLeaksDetected;
}

//-----------------------------------------------------------------------------

void MemoryDebug::printAllocationStatistics()
{
	SimpleMutexHolder smh( lock_ );
	BW_SCOPED_BOOL( internalAlloc_ );
	allocationStats_->debugReport();
}

//-----------------------------------------------------------------------------

void MemoryDebug::readLiveAllocationStats( 
	Allocator::AllocationStats& stats ) const
{
	bw_zero_memory( &stats, sizeof(stats) );
	SimpleMutexHolder smh( lock_ );
	allocationStats_->readAllocationStats(stats);
}

//-----------------------------------------------------------------------------
void MemoryDebug::readSmartPointerStats( 
	Allocator::SmartPointerStats& stats ) const
{
	bw_zero_memory( &stats, sizeof(stats) );
	SimpleMutexHolder smh( lock_ );

#if ENABLE_SMARTPOINTER_TRACKING

	stats.totalAssigns_ = (size_t)totalSmartPointerAssigns_;
	stats.currentAssigns_ = smartPointers_->size();

#endif
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

#if ENABLE_WATCHERS
void MemoryDebug::addWatchers()
{
	AllocationStats * pNull = NULL;
	SequenceWatcher< AllocationStats::SlotStats > * pWatchSlots = 
		new SequenceWatcher< AllocationStats::SlotStats >( 
		(AllocationStats::SlotStats&)( pNull->getSlotStats() ) );

	DirectoryWatcher * pSlotWatcher = new DirectoryWatcher();

	pSlotWatcher->addChild( "name", makeWatcher( 
		&AllocationStats::SlotStat::name_ ) );

	AllocationStats::SlotStat * pNullSlot = NULL;
	pSlotWatcher->addChild( "curBytes", 
		makeWatcher( (long&)pNullSlot->liveAllocationBytes_ ) );
	pSlotWatcher->addChild( "curBlocks", 
		makeWatcher( (long&)pNullSlot->liveAllocationCount_ ) );

	pWatchSlots->addChild( "*", pSlotWatcher );
	pWatchSlots->setLabelSubPath( "name" );

	Watcher::rootWatcher().addChild( "MemoryTracker", pWatchSlots, 
		allocationStats_ );

	MF_WATCH( "Allocator::heapAllocate (bytes)",
		(long&)allocationStats_->getHeapStats().liveAllocationBytes_,
		Watcher::WT_READ_ONLY,
		"Current number of heap bytes allocated." );

	MF_WATCH( "Allocator::heapAllocate (count)",
		(long&)allocationStats_->getHeapStats().liveAllocationCount_,
		Watcher::WT_READ_ONLY,
		"Current number of heap allocations." );

	MF_WATCH( "Allocator::allocate (bytes)",
		(long&)allocationStats_->getGlobalStats().liveAllocationBytes_,
		Watcher::WT_READ_ONLY,
		"Current number of bytes allocated." );

	MF_WATCH( "Allocator::allocate (count)",
		(long&)allocationStats_->getGlobalStats().liveAllocationCount_,
		Watcher::WT_READ_ONLY,
		"Current number of allocations." );
}
#endif // ENABLE_WATCHERS

void MemoryDebug::allocTrackingIgnoreBegin()
{
	++ignoreAllocs_;
}

void MemoryDebug::allocTrackingIgnoreEnd()
{
	--ignoreAllocs_;
}

void MemoryDebug::trackSmartPointerAssignment( const void * smartPointer, 
	const void * object )
{
#if ENABLE_SMARTPOINTER_TRACKING
	init();

	SimpleMutexHolder smh( lock_ );
	BW_SCOPED_BOOL( internalAlloc_ );
	
	// Begin by recording the assignment
	++totalSmartPointerAssigns_;

	// Check if an entry for this smart pointer exists
	SmartPointerMap::iterator iter = smartPointers_->find( smartPointer );
	
	// Now see if it should exist or not:
	if (object == NULL)
	{
		// Assigning to null, so remove it's entry
		if (iter != smartPointers_->end())
		{
			smartPointers_->erase( iter );
		}
		return;
	}
	
	// Need to record a new assignment
	SmartPointerInfo smartPointerInfo;
	smartPointerInfo.object = object;

	std::pair< const MemoryDebug::CallstackHeader * const, 
		MemoryDebug::CallstackStats > & csPair = 
		getCallstackEntry( CS_SMARTPOINTER, 2 );

	// inc smartpointer callstack count
	++csPair.second.allocCount;

	smartPointerInfo.callstack = csPair.first;

	if (iter == smartPointers_->end())
	{
		smartPointers_->insert( std::make_pair( smartPointer, 
			smartPointerInfo ) );
	}
	else
	{
		iter->second = smartPointerInfo;
	}
#endif // ENABLE_SMARTPOINTER_TRACKING
}

/// Print smart pointers holding given object
void MemoryDebug::printSmartPointerTracking( const void * object )
{
#if ENABLE_SMARTPOINTER_TRACKING
	init();

	SimpleMutexHolder smh( lock_ );
	BW_SCOPED_BOOL( internalAlloc_ );

	dprintf( "----------------------------------------------------------------------------------\n" );
	dprintf( " %p Smart Pointers\n", object );

	char buffer[16 * 1024];
	StringBuilder builder(buffer, ARRAY_SIZE(buffer));

	// iterate the smart pointer map and print references to our object
	for (SmartPointerMap::const_iterator itr = smartPointers_->begin(),
		end = smartPointers_->end(); itr != end; ++itr)
	{
		const SmartPointerInfo & info = itr->second;
		if (info.object == object)
		{
			const CallstackHeader * hdr = info.callstack;
			MF_ASSERT_NOALLOC( (hdr->flags & CS_SMARTPOINTER) == CS_SMARTPOINTER );
			dprintf( "----------------------------------------------------------------------------------\n" );
#if BW_ENABLE_STACKTRACE
			if ((callstackMode_ != CM_BW_GUARD) || 
				(hdr->flags & CS_ALLOCATION_HAS_COMPLETE))
			{
				builder.clear();
				formatLongCallstack( CallstackHeader::callstack( hdr ), 
					hdr->depth, builder );
				dprintf( builder.string() );
			}
#endif // BW_ENABLE_STACKTRACE
			builder.clear();
			formatLongGuardStack( CallstackHeader::guardstack( hdr ), 
				hdr->guardDepth, builder );
			dprintf( builder.string() );
		}
	}
	dprintf( "----------------------------------------------------------------------------------\n" );
#endif // ENABLE_SMARTPOINTER_TRACKING
}

} // namespace BW

#endif // ENABLE_MEMORY_DEBUG
