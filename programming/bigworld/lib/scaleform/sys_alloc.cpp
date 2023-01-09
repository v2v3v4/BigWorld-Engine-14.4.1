#include "pch.hpp"

#if SCALEFORM_SUPPORT

#if ENABLE_RESOURCE_COUNTERS
	#include "cstdmf/resource_counters.hpp"
#endif



#include "sys_alloc.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
/**
 *	Wrap Scaleform memory allocations with our memory tracking
 */
MEMTRACKER_DECLARE( ThirdParty_Scaleform, "ThirdParty - Scaleform", 0 );

	SysAlloc::SysAlloc() : total_allocated_memory(0)
	{
	}

	SysAlloc SysAlloc::instance_;

	void SysAlloc::GetInfo( Info* i ) const
	{
		i->MinAlign = 1;
		i->MaxAlign = 1;
		i->Granularity = 64*1024;
		i->HasRealloc = false;
	}

	void* SysAlloc::Alloc( UPInt size, UPInt align )
	{
	#if ENABLE_RESOURCE_COUNTERS
		RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("Scaleform/Summary", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER), size, 0);
	#endif
		total_allocated_memory+=size;
		// Ignore 'align' since reported MaxAlign is 1.
		MF_ASSERT( align == 1 );
		MEMTRACKER_SCOPED( ThirdParty_Scaleform );
		return bw_malloc( size );
	}

	bool SysAlloc::Free( void* ptr, UPInt size, UPInt align )
	{
	#if ENABLE_RESOURCE_COUNTERS
		RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("Scaleform/Summary", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER), size, 0);
	#endif
		total_allocated_memory-=size;
		// free() doesn't need size or alignment of the memory block, but
		// you can use it in your implementation if it makes things easier.
		MEMTRACKER_SCOPED( ThirdParty_Scaleform );
		bw_free( ptr );
		return true;
	}
}	//namespace ScaleformBW

BW_END_NAMESPACE
#endif