#include "pch.hpp"
#include "memory_load.hpp"
#include "allocator.hpp"

#if defined( WIN32 )

#include <psapi.h>
#pragma comment( lib, "psapi.lib" )

BW_BEGIN_NAMESPACE

namespace Memory
{
	// HACK: secret fragmentation buffer
	// allow 300MB on fragmentation, stack usage, .exe and .dll VA mapping,
	// file VA mapping and D3D frame buffers VA mapping
	const  MemSize FRAGMENTATION_MEMORY_BUFFER_SIZE = 300 * 1024 * 1024;
	// on 64 bit machines we are running out of video memory under we can fix this limit amount of system
	// memory to prevent exceeding the limit.
	const MemSize LIMIT_64BIT = 3ll * 1024ll * 1024ll * 1024ll;

	// returns max available  memory
	// usually it's limited by VA size
	// VA size could be either 2GB or 4GB depending on LARGEADDRESSAWARE
	// visual studio project option
	MemSize	maxAvailableMemory()
	{
		MEMORYSTATUSEX memoryStatus = { sizeof( memoryStatus ) };
		GlobalMemoryStatusEx( &memoryStatus );

		MemSize cap = memoryStatus.ullTotalVirtual - FRAGMENTATION_MEMORY_BUFFER_SIZE;
		if( cap > memoryStatus.ullTotalPhys * 2 )
		{
			cap = memoryStatus.ullTotalPhys * 2;
		}

		if (cap > LIMIT_64BIT)
		{
			cap = LIMIT_64BIT;
		}

		return cap;
	}


	// returns currently used memory
	// Total amount of memory the memory manager has committed for this process.
	MemSize	usedMemory()
	{
		PROCESS_MEMORY_COUNTERS pmc = { sizeof( pmc ) };
		GetProcessMemoryInfo( GetCurrentProcess(), &pmc, sizeof( pmc ) );
		return pmc.PagefileUsage;
	}


	// returns approx. largest available block
	MemSize	largestBlockSize()
	{
		// run a primitive binary search until we hit 32K border
		MemSize	minSize = 1024;
		MemSize maxSize = maxAvailableMemory() - usedMemory();

		MemSize lastSuccessfulSize = 0;

		MemSize precisionValue = 32 * 1024;

		while (maxSize - minSize > precisionValue)
		{
			size_t curSize = (size_t)((maxSize + minSize ) / 2);
			// try to reserve curSize bytes	
			void * rv = BW::Allocator::heapAllocate( curSize );
			if (rv == NULL)
			{
				// we failed, decrease the max size
				maxSize = curSize;
			}
			else
			{
				// release the memory and increase the min size
				BW::Allocator::heapDeallocate( rv );

				lastSuccessfulSize = curSize;
				minSize = curSize;
			}
		}
		// make it 32kb round
		return lastSuccessfulSize / precisionValue * precisionValue;
	}


	// returns current memory load in [ 0.0f; 100.0f ] range
	float memoryLoad()
	{
		MemSize usedMem = usedMemory();
		MemSize availMem = maxAvailableMemory();
		if (usedMem > availMem)
		{
			usedMem = availMem;
		}
		return usedMem * 100.0f / (float)availMem;
	}


	// returns current available VA space
	MemSize availableVA()
	{
		MEMORYSTATUSEX memoryStatus = { sizeof( memoryStatus ) };
		GlobalMemoryStatusEx( &memoryStatus );
		return memoryStatus.ullAvailVirtual;
	}
}

BW_END_NAMESPACE

#endif // defined( WIN32 )
