#ifndef BW_ALLOCATOR_HPP
#define BW_ALLOCATOR_HPP

#include "cstdmf/config.hpp"

#ifdef MF_SERVER
#include "cstdmf/stdmf.hpp"
#else
#include "cstdmf/cstdmf_dll.hpp"
#endif

#include <stddef.h>

#if !ENABLE_MEMORY_DEBUG
#define BW_SYSTEMSTAGE_MAIN()
#else
#define BW_SYSTEMSTAGE_MAIN() \
	BW::Allocator::ScopedSystemStage _scopedStage( BW::Allocator::SS_MAIN, BW::Allocator::SS_POST_MAIN )
#endif

#if !ENABLE_SMARTPOINTER_TRACKING
#define BW_MEMORYDEBUG_SMARTPOINTER_ASSIGN(ptr, obj)
#else
#define BW_MEMORYDEBUG_SMARTPOINTER_ASSIGN(ptr, obj) \
	BW::Allocator::trackSmartPointerAssignment(ptr, obj)
#endif

// Called by memHook to make sure Allocator is initailised
CSTDMF_DLL void memHookInitFunc();

namespace BW
{
class StringBuilder;
 
/// This is the concrete interface to underlying memory allocators
namespace Allocator
{
	enum SystemStage
	{
		SS_PRE_MAIN,
		SS_MAIN,
		SS_POST_MAIN
	};

	enum InfoFlags
	{
		IF_POOL_ALLOC     = 1, ///< Allocation came from a fixed size pool
		IF_NOTRACK_ALLOC  = 2, ///< Don't register this alloc with MemoryDebug (usually because it's already been tracked elsewhere)
		IF_INTERNAL_ALLOC = 4, ///< Allocation was an internal allocation made by fixed sized pool
		IF_DEBUG_ALLOC    = 8  ///< Allocation was from a debug allocator
	};

	/// Used for reporting current allocations for profiling display
	struct AllocationStats
	{
		size_t currentAllocs_;
		size_t currentBytes_;
	};

	// Used for reporting currently tracked smart pointers for testing and reporting
	struct SmartPointerStats
	{
		size_t totalAssigns_;
		size_t currentAssigns_;
	};

	// patterns to fill memory on alloc/free, different from heap values
	static const int CleanLandFill  = 0xCD;
	static const int DeadLandFill   = 0xDD;
	static const int NoMansLandFill = 0xFE;
	static const int AlignLandFill  = 0xFD;

	/// inform the memory debugger of the stage of initialisation
	CSTDMF_DLL void setSystemStage( SystemStage stage );

	/// Report leaks and memory stats on exit (default false)
	CSTDMF_DLL void setReportOnExit( bool reportOnExit );

	/// Force crash if memory leaks found on exit (default false)
	/// Requires report on exit
	CSTDMF_DLL void setCrashOnLeak( bool crashOnLeak );
    
	/// Report memory stats to file on exit (default false)
	void setReportStatsToFile( bool reportStatsToFile );

	/// allocate some memory
	CSTDMF_DLL void* allocate( size_t size );

	/// free some memory
	CSTDMF_DLL void deallocate( void* ptr );

	/// reallocate some memory
	void* reallocate( void* ptr, size_t size );

	/// \returns size of allocated block
	size_t memorySize( void* ptr );

    /// Full debug report information
	/// \returns the number of live main allocations (requires ENABLE_MEMORY_DEBUG)
    size_t debugReport();

	/// Prints current allocation stats
	void debugAllocStatsReport();

	/// save allocations to file
	void saveAllocationsToFile( const char* filename );

	/// save allocations to cachegrind file
	void saveAllocationsToCacheGrindFile( const char* filename );

	/// save stats to file
	void saveStatsToFile( const char* filename );

	void init();
	void fini();

	// These functions all allocate directly from the heap and will not get
	// routed through fixed sized allocator if present

	/// allocate some aligned memory from heap
	void* heapAllocateAligned( size_t size, size_t alignment, 
		unsigned int heapAllocFlags = 0 );

	/// free some aligned memory from heap
	void  heapDeallocateAligned( void* ptr, unsigned int heapAllocFlags = 0 );

	/// Allocator from heap
	/// \param size requested allocation size
	/// \param heapAllocFlags Allocator::InfoFlags describing the allocation
	void* heapAllocate( size_t size, unsigned int heapAllocFlags = 0 );

	/// Free a heap allocation
	/// \param ptr pointer to heap allocated memory to free
	/// \param heapAllocFlags Allocator::InfoFlags describing the allocation
	void  heapDeallocate( void* ptr, unsigned int heapAllocFlags = 0 );

	/// Reallocate a heap allocation
	/// \param ptr pointer to heap allocated memory to reallocate
	/// \param size requested allocation size
	/// \param heapAllocFlags Allocator::InfoFlags describing the allocation
	void* heapReallocate( void* ptr, size_t size, unsigned int heapAllocFlags = 0 );

	/// \param ptr pointer to heap allocated memory
	/// \returns the size used for the allocation
	size_t heapMemorySize( void* ptr );

#if ENABLE_MEMORY_DEBUG
	/// Populates the given struct with current global allocation stats
	CSTDMF_DLL void readAllocationStats( AllocationStats& stats );

	/// Populates the given struct with current smart pointer tracking stats
	void readSmartPointerStats( SmartPointerStats& stats );
#endif

#if ENABLE_WATCHERS
	void addWatchers();
#endif

	CSTDMF_DLL void onThreadFinish();

	/// Begin ignoring allocations between begin and end. (For leaky third party libraries)
	CSTDMF_DLL void allocTrackingIgnoreBegin();

	/// Begin ignoring allocations between begin and end. (For leaky third party libraries)
	CSTDMF_DLL void allocTrackingIgnoreEnd();
	
	CSTDMF_DLL void trackSmartPointerAssignment( const void * smartPointer, const void * object );

	CSTDMF_DLL void debugSmartPointerReport( const void * object );
	
	// Scoped stage setter than can change the system stage on stack unwind
	class ScopedSystemStage
	{
		SystemStage popStage_;
	public:
		ScopedSystemStage( SystemStage pushStage, SystemStage popStage )
			: popStage_( popStage )
		{
			setSystemStage( pushStage );
		}

		~ScopedSystemStage()
		{
			setSystemStage( popStage_ );
		}
	};

} // namespace Allocator

} // end namespace

#endif // BW_ALLOCATOR_HPP
