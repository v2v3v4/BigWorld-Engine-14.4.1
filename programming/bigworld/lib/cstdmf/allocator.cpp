#include "pch.hpp"

#include "allocator.hpp"

#include "bw_memory.hpp"
#include "bw_util.hpp"
#include "dprintf.hpp"
#include "concurrency.hpp"
#include "fixed_sized_allocator.hpp"
#include "memory_debug.hpp"
#include "slot_tracker.hpp"
#include "string_builder.hpp"
#include "watcher.hpp"
#include "debug_filter.hpp"

#if ENABLE_MEMORY_DEBUG || ENABLE_ALLOCATOR_STATISTICS
#include "fini_job.hpp"
#endif // ENABLE_MEMORY_DEBUG || ENABLE_ALLOCATOR_STATISTICS
#if defined(USE_MEMHOOK)
#include "memhook/memhook.hpp"
#endif // USE_MEMHOOK

#include <time.h>

#ifndef PROTECTED_ALLOCATOR
#if defined( BW_EXPORTER ) || defined( __linux__ ) || \
	defined( __ANDROID__ ) || defined( __APPLE__ ) || \
	defined( __EMSCRIPTEN__ )
#define ANSI_ALLOCATOR
#else
#define NED_ALLOCATOR
#endif
#endif // PROTECTED_ALLOCATOR

#ifdef ANSI_ALLOCATOR
#include "ansi_allocator.hpp"
#define SystemAllocator BW::AnsiAllocator
#elif defined ( NED_ALLOCATOR )
#include "ned_allocator.hpp"
#define SystemAllocator BW::NedAllocator
#elif defined PROTECTED_ALLOCATOR
#include "protected_allocator.hpp"
#define SystemAllocator BW::ProtectedAllocator
#endif

//-----------------------------------------------------------------------------

// Enable this to fill allocated and freed memory
#define ENABLE_ALLOCATOR_MEMORY_FILLS _DEBUG
#if ENABLE_ALLOCATOR_MEMORY_FILLS
inline void markAlloc( void * __restrict ptr, size_t size, size_t allocSize, int fill, int pad )
{
	if ( ptr )
	{
		memset( ptr, fill, size );
		memset( (char*)ptr + size, pad, allocSize - size );
	}
}

inline void markDealloc( void* __restrict ptr, int fill, size_t allocSize )
{
	if ( ptr )
	{
		memset( ptr, fill, allocSize );
	}
}
#endif

//-----------------------------------------------------------------------------

// Called by memHook to make sure Allocator is initailised
void memHookInitFunc()
{
	BW::Allocator::init();
}

//-----------------------------------------------------------------------------

namespace
{

//-----------------------------------------------------------------------------

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable: 4073)
#pragma init_seg(lib)				// Ensure we get constructed first
#pragma warning(pop)
#endif // WIN32

struct AllocatorScopedInit
{
	AllocatorScopedInit()
	{
		BW::Allocator::init();
	}
	~AllocatorScopedInit()
	{
		BW_NAMESPACE DebugFilter::fini();
		BW::Allocator::fini();
	}
};
AllocatorScopedInit BW_STATIC_INIT_PRIORITY(1200) s_allocatorScopedInit;

//-----------------------------------------------------------------------------

#if ENABLE_FIXED_SIZED_POOL_ALLOCATOR
BW::FixedSizedAllocator*	s_fixedSizedAllocator;
char						s_fixedSizedAllocatorMem[sizeof(BW::FixedSizedAllocator)];
#endif // ENABLE_FIXED_SIZED_POOL_ALLOCATOR

#if ENABLE_MEMORY_DEBUG
BW::MemoryDebug*			s_memoryDebug;
char						s_memoryDebugMem[sizeof(BW::MemoryDebug)];
#endif // ENABLE_MEMORY_DEBUG

//-----------------------------------------------------------------------------

bool s_initialised = false;
bool s_reportOnExit = true;
bool s_crashOnLeak = false;
bool s_reportStatsToFile = false;

} // anonymous namespace

//-----------------------------------------------------------------------------

namespace BW
{

//-----------------------------------------------------------------------------

void Allocator::setSystemStage( SystemStage stage )
{
#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug) s_memoryDebug->setSystemStage( stage );
#endif // ENABLE_MEMORY_DEBUG
}

//-----------------------------------------------------------------------------

void Allocator::setReportOnExit( bool reportOnExit )
{
	s_reportOnExit = reportOnExit;
}

//-----------------------------------------------------------------------------

void Allocator::setCrashOnLeak( bool crashOnLeak )
{
	s_crashOnLeak = crashOnLeak;
}

//-----------------------------------------------------------------------------

void Allocator::setReportStatsToFile( bool reportStatsToFile )
{
	s_reportStatsToFile = reportStatsToFile;
}

//-----------------------------------------------------------------------------

void* Allocator::allocate( size_t size )
{
	init();

#if ENABLE_FIXED_SIZED_POOL_ALLOCATOR
    void* ptr = s_fixedSizedAllocator->allocate( size, IF_NOTRACK_ALLOC );

#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug)
	{
		FixedSizedAllocator::AllocationInfo info;
		s_fixedSizedAllocator->allocationInfo( ptr, info );
		s_memoryDebug->allocate( ptr, info.allocSize_, size, info.allocFlags_ );
	}
#endif

#else
	void* ptr = heapAllocate( size );
#endif

	return ptr;
}

//-----------------------------------------------------------------------------

void Allocator::deallocate( void* ptr )
{
#if ENABLE_FIXED_SIZED_POOL_ALLOCATOR
#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug) 
	{
		s_memoryDebug->deallocate( ptr );
	}
#endif
 s_fixedSizedAllocator->deallocate( ptr, IF_NOTRACK_ALLOC );
#else
	heapDeallocate( ptr );
#endif
}


//-----------------------------------------------------------------------------

void* Allocator::heapAllocateAligned( size_t size, size_t alignment, unsigned int heapAllocFlags )
{
	init();

	MF_ASSERT_NOALLOC( !(heapAllocFlags & IF_POOL_ALLOC) );

	void* ptr = SystemAllocator::allocateAligned( size, alignment );

#if ENABLE_ALLOCATOR_MEMORY_FILLS
	size_t allocSize = SystemAllocator::memorySize( ptr );
#endif

#if ENABLE_ALLOCATOR_MEMORY_FILLS
	markAlloc( ptr, size, allocSize, CleanLandFill, AlignLandFill );
#endif

#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug  && !(heapAllocFlags & IF_NOTRACK_ALLOC)) 
	{
#if !ENABLE_ALLOCATOR_MEMORY_FILLS
		size_t allocSize = SystemAllocator::memorySize( ptr );
#endif
		s_memoryDebug->allocate( ptr, allocSize, size, heapAllocFlags );
	}
#endif

	return ptr;
}

//-----------------------------------------------------------------------------

void Allocator::heapDeallocateAligned( void* ptr, unsigned int heapAllocFlags )
{
	MF_ASSERT_NOALLOC( !(heapAllocFlags & IF_POOL_ALLOC) );

#if ENABLE_ALLOCATOR_MEMORY_FILLS
	markDealloc( ptr, DeadLandFill, SystemAllocator::memorySize( ptr ) );
#endif

#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug && !(heapAllocFlags & IF_NOTRACK_ALLOC)) 
	{
		s_memoryDebug->deallocate( ptr );
	}
#endif

	return SystemAllocator::deallocateAligned( ptr );
}

//-----------------------------------------------------------------------------

void* Allocator::reallocate( void* ptr, size_t size )
{
#if ENABLE_FIXED_SIZED_POOL_ALLOCATOR
#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug) 
	{
		s_memoryDebug->deallocate( ptr );
	}
#endif

	ptr = s_fixedSizedAllocator->reallocate( ptr, size, IF_NOTRACK_ALLOC );

#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug) 
	{
		FixedSizedAllocator::AllocationInfo info;
		s_fixedSizedAllocator->allocationInfo( ptr, info );
		s_memoryDebug->allocate( ptr, info.allocSize_, size, info.allocFlags_ );
	}
#endif
#else
	ptr = heapReallocate( ptr, size );
#endif

	return ptr;
}

//-----------------------------------------------------------------------------

size_t Allocator::memorySize( void* ptr )
{
#if ENABLE_FIXED_SIZED_POOL_ALLOCATOR
	return s_fixedSizedAllocator->memorySize( ptr );
#else
	return heapMemorySize( ptr );
#endif
}

//-----------------------------------------------------------------------------

void Allocator::saveAllocationsToFile( const char* filename )
{
	char buffer[4096];
	StringBuilder builder( buffer, ARRAY_SIZE( buffer ) );
	SystemAllocator::debugReport( builder );
	dprintf( "%s", builder.string() );
#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug) 
	{
		s_memoryDebug->saveAllocationsToFile( filename );
	}
#endif
}

//-----------------------------------------------------------------------------

void Allocator::saveAllocationsToCacheGrindFile( const char* filename )
{
#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug) 
	{
		s_memoryDebug->saveAllocationsToCacheGrindFile( filename );
	}
#endif
}

//-----------------------------------------------------------------------------
void Allocator::saveStatsToFile( const char* filename )
{
#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug) 
	{
		s_memoryDebug->saveStatsToFile( filename );
	}
#endif
}

//-----------------------------------------------------------------------------

size_t Allocator::debugReport()
{
	size_t numMainAllocs = 0;
#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug)
	{
		numMainAllocs = s_memoryDebug->printLiveAllocations();
		
		if (s_memoryDebug->shouldReportCacheGrind())
		{
			char callgrindPath[BW_MAX_PATH];
			
			char exePath[BW_MAX_PATH];
			if (BWUtil::getExecutablePath( exePath, ARRAY_SIZE(exePath) ))
			{
				char * directory = NULL;
				char * basename = NULL;
				time_t now = time( NULL );
				tm* t = localtime( &now );

				BWUtil::breakPath( exePath, &directory, &basename, NULL );
				bw_snprintf( callgrindPath, ARRAY_SIZE( callgrindPath ),
					"%s%scallgrind.%s_%04d%02d%02d_%02d%02d%02d.%d",
					directory, directory ? "/" : "", basename,
					1900+t->tm_year, t->tm_mon+1, t->tm_mday,
					t->tm_hour, t->tm_min, t->tm_sec, mf_getpid() );
			}
			else
			{
				strncpy( callgrindPath, "callgrind.bw", 
					ARRAY_SIZE( callgrindPath ) );
			}
			
			s_memoryDebug->saveAllocationsToCacheGrindFile( callgrindPath );
		}
	}
#endif
#if ENABLE_FIXED_SIZED_POOL_ALLOCATOR && ENABLE_FIXED_SIZED_POOL_STATISTICS
	s_fixedSizedAllocator->debugPrintStatistics();
#endif
	debugAllocStatsReport();
	return numMainAllocs;
}

//-----------------------------------------------------------------------------

void Allocator::debugAllocStatsReport()
{
	char buffer[4096];
	StringBuilder builder( buffer, ARRAY_SIZE( buffer ) );
	SystemAllocator::debugReport( builder );
	dprintf( "%s", builder.string() );
#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug) 
	{
		s_memoryDebug->printAllocationStatistics();
	}
#endif
}

//-----------------------------------------------------------------------------

void Allocator::init()
{
    if( !s_initialised )
    {
		s_initialised = true;

#if ENABLE_MEMORY_DEBUG
		bool disabled = false;

		// find the memory related options
		MemoryDebug::CallstackMode callstackMode = MemoryDebug::CM_BW_GUARD;
		bool reportHotspots = false;
		bool reportIgnoredLeaks = false;
		bool reportCacheGrind = false;
		char debugGuardString[256] = { 0 };
		bool debugGuard = false;

#ifdef _WIN32
		// get command line options
		LPSTR commandLine = ::GetCommandLineA();
#else
		char commandLine[2048];
		memset( commandLine, 0, sizeof(commandLine) );
		FILE * fp = fopen( "/proc/self/cmdline", "r" );
		size_t readCount = fread( commandLine, 1, sizeof(commandLine) - 1, fp );
		fclose( fp );
		for (size_t i = 0; i < readCount; ++i)
		{
			if (commandLine[i] == 0)
			{
				commandLine[i] = ' ';
			}
		}
#endif
		// check for the callstack mode
		if (strstr( commandLine, "-memdebug=disable" ))
		{
			disabled = true;
		}

		if (strstr( commandLine, "-memdebug=cs_fast" ))
		{
			callstackMode = MemoryDebug::CM_CALLSTACKS_FAST;
		}
		else if(strstr( commandLine, "-memdebug=cs_complete"))
		{
			callstackMode = MemoryDebug::CM_CALLSTACKS_COMPLETE;
		}
		else if(strstr( commandLine, "-memdebug=cs_guard"))
		{
			callstackMode = MemoryDebug::CM_BW_GUARD;
		}

		if(strstr( commandLine, "-memhotspots"))
		{
			reportHotspots = true;
		}

		if(strstr( commandLine, "-memreportignoredleaks"))
		{
			reportIgnoredLeaks = true;
		}

		if(strstr( commandLine, "-memdumpstats"))
		{
			setReportStatsToFile(true);
		}

		if(strstr( commandLine, "-memreportcachegrind"))
		{
			reportCacheGrind = true;
		}

		// check to see if user has specified a BW_GUARD to generate complete callstacks for
		// note : we cannot use sscanf here as that internally allocates memory so we need to
		//        go a slightly longer route.
		{
			const char DebugGuardOption[] = "-memdebugguard=";
			const char * opt = strstr( commandLine, DebugGuardOption );
			if (opt)
			{
				// step past option
				opt += sizeof( DebugGuardOption ) - 1;
				char * p = debugGuardString;
				while ( *opt && *opt != ' ')
					*p++ = *opt++;

				if (debugGuardString[0])
					debugGuard = true;
			}
		}
#endif // ENABLE_MEMORY_DEBUG

#if ENABLE_FIXED_SIZED_POOL_ALLOCATOR
		// dynamically work out pool sizes depending on pointer size
		size_t sizes[9];
		size_t numSizes = 0;
		for ( ; (sizeof(void*) << numSizes) < 2048; ++numSizes )
			sizes[numSizes] = sizeof(void*) << numSizes;
		const size_t poolSize = 1 * 1024 * 1024 / 2;
		s_fixedSizedAllocator = new(s_fixedSizedAllocatorMem) BW::FixedSizedAllocator(
			sizes, numSizes, poolSize, "System" );

		s_fixedSizedAllocator->setLargeAllocationHooks( &heapAllocate, 
			&heapDeallocate, &heapReallocate, &heapMemorySize );
#endif // ENABLE_FIXED_SIZED_POOL_ALLOCATOR

#if ENABLE_SLOT_TRACKER
		ThreadSlotTracker::init();
#endif

#if defined(USE_MEMHOOK)
		BW::Memhook::AllocFuncs allocFuncs = { 
			::bw_new, ::bw_new /* nothrow */, 
			::bw_new_array, ::bw_new_array /* nothrow */, 
			::bw_delete, ::bw_delete /* nothrow */,
			::bw_delete_array, ::bw_delete_array  /* nothrow */, 
			::bw_malloc, ::bw_free,
			::bw_malloc_aligned, ::bw_free_aligned,
			::bw_realloc,
			::bw_memsize
		};
		BW::Memhook::allocFuncs( allocFuncs );
#endif

#if ENABLE_MEMORY_DEBUG
		if (!disabled)
		{
			s_memoryDebug = new(s_memoryDebugMem) BW::MemoryDebug( callstackMode, reportHotspots, reportIgnoredLeaks, reportCacheGrind );
			if (debugGuard)
			{
				s_memoryDebug->showDetailedStackForGuard( debugGuardString );
			}
		}
#endif // ENABLE_MEMORY_DEBUG
    }
}

//-----------------------------------------------------------------------------

void Allocator::fini()
{
#if ENABLE_MEMORY_DEBUG || ENABLE_ALLOCATOR_STATISTICS
	// MemTracker did this, not sure if it's still required
	FiniJob::runAll();
#endif

	if ( s_reportOnExit )
	{
		size_t numMainAllocs = debugReport();
		if ( numMainAllocs && s_crashOnLeak )
			*((volatile int *)0) = 0;
	}
#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug) 
	{
		if (s_reportStatsToFile)
		{
			saveStatsToFile("memStats.csv");
		}

		s_memoryDebug->~MemoryDebug();
		s_memoryDebug = 0;
	}
#endif
#if ENABLE_FIXED_SIZED_POOL_ALLOCATOR
	s_fixedSizedAllocator->~FixedSizedAllocator();
	s_fixedSizedAllocator = 0;
#endif
}

//-----------------------------------------------------------------------------

void*  Allocator::heapAllocate( size_t size, unsigned int heapAllocFlags )
{
	MF_ASSERT_NOALLOC( !(heapAllocFlags & IF_POOL_ALLOC) );

	void* ptr = SystemAllocator::allocate( size );

#if ENABLE_ALLOCATOR_MEMORY_FILLS
	size_t allocSize = SystemAllocator::memorySize( ptr );
#endif

#if ENABLE_ALLOCATOR_MEMORY_FILLS
	markAlloc( ptr, size, allocSize, CleanLandFill, NoMansLandFill );
#endif

#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug && !(heapAllocFlags & IF_NOTRACK_ALLOC))
	{
#if !ENABLE_ALLOCATOR_MEMORY_FILLS
		size_t allocSize = SystemAllocator::memorySize( ptr );
#endif
		s_memoryDebug->allocate( ptr, allocSize, size, heapAllocFlags );
	}
#endif // ENABLE_MEMORY_DEBUG

	return ptr;
}

//-----------------------------------------------------------------------------

void  Allocator::heapDeallocate( void* ptr, unsigned int heapAllocFlags )
{
#if ENABLE_ALLOCATOR_MEMORY_FILLS
	markDealloc( ptr, DeadLandFill, SystemAllocator::memorySize( ptr ) );
#endif

#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug && !(heapAllocFlags & IF_NOTRACK_ALLOC))
	{
		s_memoryDebug->deallocate( ptr );
	}
#endif

	SystemAllocator::deallocate( ptr );
}

//-----------------------------------------------------------------------------

void* Allocator::heapReallocate( void* ptr, size_t size, unsigned int heapAllocFlags )
{
#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug && !(heapAllocFlags & IF_NOTRACK_ALLOC))
	{
		s_memoryDebug->deallocate( ptr );
	}
#endif

    ptr = SystemAllocator::reallocate( ptr, size );

#if ENABLE_MEMORY_DEBUG
	if (s_memoryDebug && !(heapAllocFlags & IF_NOTRACK_ALLOC))
	{
		s_memoryDebug->allocate( ptr, SystemAllocator::memorySize( ptr ), size, heapAllocFlags );
	}
#endif

	return ptr;
}

//-----------------------------------------------------------------------------

size_t Allocator::heapMemorySize( void* ptr )
{
	return SystemAllocator::memorySize( ptr );
}

//-----------------------------------------------------------------------------

#if ENABLE_WATCHERS
void Allocator::addWatchers()
{
#if ENABLE_MEMORY_DEBUG
	if ( s_memoryDebug )
	{
		s_memoryDebug->addWatchers();
	}
#endif
}
#endif

//-----------------------------------------------------------------------------

#if ENABLE_MEMORY_DEBUG
void Allocator::readAllocationStats( AllocationStats& stats )
{
	if ( s_memoryDebug ) 
	{
		s_memoryDebug->readLiveAllocationStats( stats );
	}
}


//-----------------------------------------------------------------------------

void Allocator::readSmartPointerStats( SmartPointerStats& stats )
{
	if ( s_memoryDebug ) 
	{
		s_memoryDebug->readSmartPointerStats( stats );
	}
}

#endif

} // end namespace

//-----------------------------------------------------------------------------

void BW::Allocator::onThreadFinish()
{
	SystemAllocator::onThreadFinish();
}

//-----------------------------------------------------------------------------

void BW::Allocator::allocTrackingIgnoreBegin()
{
#if ENABLE_MEMORY_DEBUG
	if ( s_memoryDebug )
	{
		s_memoryDebug->allocTrackingIgnoreBegin();
	}
#endif
}

void BW::Allocator::allocTrackingIgnoreEnd()
{
#if ENABLE_MEMORY_DEBUG
	if ( s_memoryDebug )
	{
		s_memoryDebug->allocTrackingIgnoreEnd();
	}
#endif
}

void BW::Allocator::trackSmartPointerAssignment( const void * smartPointer, const void * object )
{
#if ENABLE_MEMORY_DEBUG
	if ( s_memoryDebug )
	{
		s_memoryDebug->trackSmartPointerAssignment( smartPointer, object );
	}
#endif
}


void BW::Allocator::debugSmartPointerReport( const void * object )
{
#if ENABLE_MEMORY_DEBUG
	if ( s_memoryDebug )
	{
		s_memoryDebug->printSmartPointerTracking( object );
	}
#endif
}
