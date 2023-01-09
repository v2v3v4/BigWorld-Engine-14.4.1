#ifndef SYSALLOC_HPP
#define SYSALLOC_HPP

#include "config.hpp"

#if SCALEFORM_SUPPORT

#include <Kernel/HeapPT/HeapPT_SysAllocWinAPI.h>

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
	class SysAlloc : public SysAllocWinAPI
	{
		size_t total_allocated_memory;
		static SysAlloc instance_;
	public:

		SysAlloc();

		static SysAlloc& instance() { return instance_; }
		size_t MemoryUsed() { return total_allocated_memory; }

		virtual void	GetInfo(Info* i) const;
		virtual void*	Alloc(UPInt size, UPInt align);
		virtual bool    Free(void* ptr, UPInt size, UPInt align);
	};
}	//namespace ScaleformBW

BW_END_NAMESPACE

#endif

#endif
