#include "pch.hpp"
#include "protected_allocator.hpp"
#include "ansi_allocator.hpp"
#include "string_builder.hpp"
#include "scoped_bool.hpp"
#include "cstdmf/concurrency.hpp"

#ifdef PROTECTED_ALLOCATOR

#include <set>
#include <list>
#include <map>

#ifdef _WIN32
#else
#include <unistd.h>
#include <sys/mman.h>
#endif // _WIN32

#include "debug.hpp"

namespace BW
{

namespace
{
size_t pageSize = 4096;
size_t pageBitsMask = pageSize - 1;

size_t allocationGranularity = 64 * 1024;
size_t granularityBitsMask = allocationGranularity - 1;

size_t granularityPages = allocationGranularity / pageSize;

void * valloc( size_t size )
{
/*
 * we allocate pages and make them unaccessible
 * later in ProtectedAllocator::allocate() everything except the last page is unprotected
 */
#ifdef _WIN32
	void * ptr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_NOACCESS);
	return ptr;
#else
	void * ptr = mmap(NULL, size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
	return ptr == MAP_FAILED ? NULL : ptr;
#endif
}

bool vfree( void * ptr, size_t size )
{
#ifdef _WIN32
	return VirtualFree( ptr, size, MEM_DECOMMIT ) != false;
#else
	return munmap( ptr, size ) == 0;
#endif
}

bool vunprotect( void * ptr, size_t size )
{
#ifdef _WIN32
	DWORD fFlags = PAGE_READWRITE;
	bool result = true;
	while (size > 0)
	{
		size_t remainingInPage = allocationGranularity -
			(uintptr(ptr) & granularityBitsMask);
		result &= VirtualProtect( ptr, std::min( size,
			remainingInPage ), PAGE_EXECUTE_READWRITE, &fFlags ) != 0;
		MF_ASSERT_NOALLOC( result );
		ptr = (char*)ptr + remainingInPage;
		if (size > remainingInPage)
		{
			size -= remainingInPage;
		}
		else
		{
			size = 0;
		}
	}
	return result;
#else
	return mprotect( ptr, size, PROT_EXEC | PROT_READ | PROT_WRITE ) == 0;
#endif
}

bool vprotect( void * ptr, size_t size )
{
#ifdef _WIN32
	DWORD fFlags = PAGE_NOACCESS;
	bool result = true;
	while (size > 0)
	{
		size_t remainingInPage = allocationGranularity -
			(uintptr(ptr) & granularityBitsMask);
		result &= VirtualProtect( ptr, std::min( size,
			remainingInPage ), PAGE_NOACCESS, &fFlags ) != 0;
		MF_ASSERT_NOALLOC( result );
		ptr = (char*)ptr + remainingInPage;
		if (size > remainingInPage)
		{
			size -= remainingInPage;
		}
		else
		{
			size = 0;
		}
	}
	return result;
#else
	return mprotect( ptr, size, PROT_NONE ) == 0;
#endif
}

struct ChunkHeader
{
	size_t size;
};

#define NOT_INITED		0
#define INITING			1
#define INIT_FINISHED	2

#ifdef _WIN32
#ifndef sleep
#define sleep(i) Sleep(i*1000)
#endif
#endif

static bool shouldInitOnce(bw_atomic32_t *init) // TODO probably move to cstdmf threading
{
	int local;
	while ((local = BW_ATOMIC32_COMPARE_AND_SWAP(init, NOT_INITED, INITING)) == INITING)
	{
		sleep(0);
	}

	return local == NOT_INITED;

}

void init()
{
	static bw_atomic32_t init = NOT_INITED;

	if (shouldInitOnce(&init))
	{
#ifdef _WIN32
		SYSTEM_INFO systemInfo;
		GetSystemInfo (&systemInfo);

		pageSize = systemInfo.dwPageSize;
		allocationGranularity = systemInfo.dwAllocationGranularity;
#else
		pageSize = sysconf(_SC_PAGESIZE);
		allocationGranularity = pageSize;
#endif
		pageBitsMask = pageSize - 1;
		granularityBitsMask = allocationGranularity - 1;
		granularityPages = allocationGranularity / pageSize;

		init = INIT_FINISHED;
	}
}

THREADLOCAL(bool) isInternal = false;

class PageAllocator
{
	static const size_t DEAD_LIST_SIZE = 2048;
	// 4kb page size
	// total memory = DEAD * pageSize = 4096 * 2048 = 8mb
private:
#ifndef _WIN32
inline void registerAlloc( void *ptr, size_t size)
{
}
inline bool unregisterAlloc( void * ptr, size_t size )
{
	return true;
}
#else
	void registerAlloc( void * ptr, size_t size )
	{
		if (size > allocationGranularity)
		{
			bool success = largeAllocs_.insert(
				LargeAllocs::value_type( 
				ptr, std::pair<size_t, size_t> (size, size) ) ).second;
			MF_ASSERT_NOALLOC( success );
		}
	}
	bool unregisterAlloc( void * ptr, size_t size )
	{		
		LargeAllocs::iterator it = largeAllocs_.upper_bound( ptr );

		if (largeAllocs_.size() != 0 && it != largeAllocs_.begin())
		{
			--it;

			void * const & regionStart = it->first;
			size_t & regionSizeOrig = it->second.first,
				   & regionSize = it->second.second;

			if (regionStart <= ptr &&
					ptr < (char*)regionStart + regionSizeOrig)
			{
				MF_ASSERT_NOALLOC( regionSize >= size );
				regionSize -= size;
				if (regionSize == 0)
				{
					largeAllocs_.erase( it );
				}
				else
				{
					return true;
				}
			}
		}
	
		return VirtualFree( ptr, 0, MEM_RELEASE ) != 0;
	}
#endif

public:
	PageAllocator()
	{
	}

	void * allocate( size_t allocationSize )
	{
		MF_ASSERT_NOALLOC( (allocationSize & pageBitsMask) == 0 );
		SimpleMutexHolder smh( mutex_ );
		BW_SCOPED_BOOL( isInternal );

		// The reason behind going from the front of the freeList
		// is that this allows the pages to collect up with larger
		// addresses to remaing in the list longer, and hopefully
		// have enough for their original allocation size
		FreeList::iterator it = freeList_.begin();
		while (it != freeList_.end() &&
			it->second < allocationSize )
		{
			++it;
		}

		if (it != freeList_.end() && it->second >= allocationSize)
		{
			it->second -= allocationSize;
			void * ptr = (char *)it->first + it->second;
			if (it->second == 0)
			{
				freeList_.erase(it);
			}
			return ptr;
		}

		size_t totalAllocation = (allocationSize + granularityBitsMask) & 
			~granularityBitsMask;
		char * morepages = (char*)valloc( totalAllocation );

		MF_ASSERT_NOALLOC( morepages );

		this->registerAlloc(morepages, totalAllocation);

		if (totalAllocation != allocationSize)
		{
			freeList_.insert(
				FreeList::value_type( morepages + allocationSize,
					totalAllocation - allocationSize ) );
		}

		return morepages;
	}

	void deallocate( void * ptr, size_t size )
	{
		MF_ASSERT_NOALLOC( (size & pageBitsMask) == 0 );
		SimpleMutexHolder smh( mutex_ );
		BW_SCOPED_BOOL( isInternal );

		deadList_.push_back( DeadList::value_type( ptr, size ) );

		if (deadList_.size() > DEAD_LIST_SIZE)
		{
			DeadList::value_type front = deadList_.front();
			deadList_.pop_front();
			
			FreeList::iterator higher = freeList_.upper_bound( front.first ),
				lower = higher;

			if (lower == freeList_.begin())
			{
				lower = freeList_.end();
			}
			else
			{
				--lower;
			}

			MF_ASSERT_NOALLOC( lower == freeList_.end() ||
				lower->first != front.first );

			bool startConnected = freeList_.size() > 0 &&
				lower != freeList_.end() &&
				lower->first == (char*)front.first - lower->second;
			bool endConnected = freeList_.size() > 0 &&
				higher != freeList_.end() &&
				higher->first == (char*)front.first + front.second;

			FreeList::iterator newFreeRegion;
			if (endConnected)
			{
				front.second += higher->second;
				freeList_.erase(higher);
			}

			if (startConnected)
			{
				newFreeRegion = lower;
				lower->second += front.second;
			}
			else
			{
				std::pair< FreeList::iterator, bool > result = freeList_.insert( front );
				MF_ASSERT_NOALLOC( result.second );
				newFreeRegion = result.first;
			}

			void * allocStart = (void*)(uintptr(front.first) &
					~granularityBitsMask);
			size_t allocSize = front.second & ~granularityBitsMask;
			
			if (allocStart >= newFreeRegion->first &&
				(char*)allocStart + allocSize <= (char*)newFreeRegion->first + newFreeRegion->second)
			{
				size_t startOffset = (char*)allocStart - (char*)newFreeRegion->first,
					endOffset = newFreeRegion->second - (allocSize + startOffset);
			
				vfree( allocStart, allocSize );
				this->unregisterAlloc( allocStart, allocSize );

				if (endOffset > 0)
				{
					freeList_.insert(
						FreeList::value_type( (char*)allocStart + allocSize, endOffset ) );
				}

				if (startOffset > 0)
				{
					newFreeRegion->second = startOffset;
				}
				else
				{
					freeList_.erase( newFreeRegion );
				}
			}
		}
	}
private:
	// TODO: Potential dead lock with memory debug
	SimpleMutex mutex_;

	// We use STL directly to not use our allocators
	typedef std::map<void*,size_t> FreeList;
	FreeList freeList_;
	typedef std::list< std::pair<void*,size_t> > DeadList;
	DeadList deadList_;
#ifdef _WIN32
	typedef std::map< void*,std::pair<size_t, size_t> > LargeAllocs;
	LargeAllocs largeAllocs_;
#endif
	
};

PageAllocator & pageAllocator()
{
	static unsigned char * allocatorMem[sizeof(PageAllocator)];
	static PageAllocator * allocatorPtr = NULL;
	static bw_atomic32_t init = NOT_INITED;

	if (shouldInitOnce(&init))
	{
		BW_SCOPED_BOOL( isInternal );
		allocatorPtr = new(allocatorMem) PageAllocator();
		init = INIT_FINISHED;
	}

	return *allocatorPtr;
}


ChunkHeader * getChunkHeader( void * ptr )
{
	// Check if the header is on the lower page
	if (((uintptr_t)ptr & pageBitsMask) < sizeof(ChunkHeader))
	{
		ChunkHeader * result = (ChunkHeader*)(((uintptr_t)ptr - pageSize) & ~pageBitsMask);
		return result;
	}
	
	ChunkHeader * result = (ChunkHeader*)((uintptr_t)ptr & ~pageBitsMask);
	return result;
}

size_t calculateAlignedSize( size_t size )
{
	return (size + pageSize + pageBitsMask 
		+ sizeof(ChunkHeader)) & ~pageBitsMask;
}

} // anonymous namespace

//-----------------------------------------------------------------------------
void* ProtectedAllocator::allocate( size_t size )
{
	init();

	size_t aligned = calculateAlignedSize( size );
	ChunkHeader * hdr;

	if (aligned < size)
	{
		// We've looped back around (impossible allocation)
		return NULL;
	}

	if (isInternal)
	{
		// TODO: This is hacky
		return AnsiAllocator::allocate( size );
	}

	hdr = (ChunkHeader*)pageAllocator().allocate( aligned );
	if (hdr)
	{
		if(!vunprotect( hdr, aligned - pageSize ))
		{
			MF_DEBUGPRINT_NOALLOC( "Failed to unprotect memory\n" );
		}

		hdr->size = size;

		void * ptr = (char *)hdr + aligned - pageSize - size;
		MF_ASSERT_NOALLOC( getChunkHeader(ptr) == hdr );
		return ptr;
	}

	return NULL;
}

//-----------------------------------------------------------------------------

void ProtectedAllocator::deallocate( void* ptr )
{
	if (isInternal)
	{
		// TODO: This is hacky
		return AnsiAllocator::deallocate( ptr );
	}

	if (!ptr)
	{
		return;
	}

	ChunkHeader * hdr = getChunkHeader( ptr );

	// Double free should cause access denied when size is read
	size_t realSize = calculateAlignedSize( hdr->size );

	if (!vprotect( hdr, realSize - pageSize ))
	{
		MF_DEBUGPRINT_NOALLOC( "Failed to protect free memory\n" );
	}

	pageAllocator().deallocate( hdr, realSize );
}

//-----------------------------------------------------------------------------

void* ProtectedAllocator::reallocate( void* ptr, size_t size )
{
	void * newPtr = allocate(size);
	if (ptr)
	{
		ChunkHeader * hdr = getChunkHeader(ptr);
		size_t oldsize = hdr->size;
		memcpy (newPtr, ptr, oldsize >= size ? size : oldsize);
		deallocate(ptr);
	}

	return newPtr;
}

//-----------------------------------------------------------------------------

void* ProtectedAllocator::allocateAligned( size_t size, size_t alignment )
{
	// TODO: Should we assume alignment is a multiple of 2
	size_t unprotectedPadding = 
		(pageSize - (size & pageBitsMask)) % alignment;
	void * ptr = allocate( size + unprotectedPadding );
	MF_ASSERT_NOALLOC( ((uintptr_t)ptr % alignment) == 0 );
	return ptr;
}

//-----------------------------------------------------------------------------

void ProtectedAllocator::deallocateAligned( void* ptr )
{
	deallocate( ptr );
}


//-----------------------------------------------------------------------------

size_t ProtectedAllocator::memorySize( void* ptr )
{
	if (isInternal)
	{
		return AnsiAllocator::memorySize( ptr );
	}

	if (!ptr)
	{
		return 0;
	}

	// TODO: Should this be real size, or usable size?
	ChunkHeader * hdr = getChunkHeader( ptr );
	return hdr->size;
}

//-----------------------------------------------------------------------------

void ProtectedAllocator::debugReport( StringBuilder& builder )
{
	// TODO: Output page waste?
}

void ProtectedAllocator::onThreadFinish()
{
	// no-op
}

//-----------------------------------------------------------------------------

} // end namespace

#endif // PROTECTED_ALLOCATOR
