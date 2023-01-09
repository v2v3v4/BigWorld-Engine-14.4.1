#ifndef BW_GROW_ONLY_POD_ALLOCATOR_HPP
#define BW_GROW_ONLY_POD_ALLOCATOR_HPP

#include <cstddef>
#include "cstdmf/stdmf.hpp"

namespace BW
{

/// simple grow only allocator, very fast but all elements must be free'd together 
/// and be pod type. this is useful when there are many small allocations and few
/// deallocations. 
class CSTDMF_DLL GrowOnlyPodAllocator
{
public:
	/// constructor
	GrowOnlyPodAllocator( size_t pageSize );

	/// destructor
	~GrowOnlyPodAllocator();

	/// cleanup all allocated memory
	void releaseAll();

	/// allocate some bytes
	void* allocate( size_t size );

	/// free some bytes, does nothing.
	void deallocate(void*) 
	{ /* no-op */ }		

	void printUsageStats() const;


private:
	/// per page header
	struct PageHeader
	{
		size_t		used;
		PageHeader *nextPage;
	};

private:
	void addPage();

private:
	const size_t	pageSize_;
	PageHeader*		currentPage_;
};

} // end namespace

#endif // BW_GROW_ONLY_POD_ALLOCATOR_HPP

