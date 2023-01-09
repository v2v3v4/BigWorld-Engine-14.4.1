#include "pch.hpp"
#include "grow_only_pod_allocator.hpp"
#include "stdmf.hpp"
#include "bw_memory.hpp"
#include "debug.hpp"
#include "dprintf.hpp"
#include "allocator.hpp"

namespace BW
{


//------------------------------------------------------------------------------------

GrowOnlyPodAllocator::GrowOnlyPodAllocator(size_t pageSize) :
	pageSize_(pageSize),
	currentPage_(0)
{			
}

//------------------------------------------------------------------------------------

GrowOnlyPodAllocator::~GrowOnlyPodAllocator()
{
	releaseAll();
}		


//------------------------------------------------------------------------------------

void GrowOnlyPodAllocator::releaseAll()
{
	PageHeader *page = currentPage_;
	while (page)
	{
		PageHeader *next = page->nextPage;
		bw_free( page );
		page = next;
	}
	currentPage_ = 0;
}

//------------------------------------------------------------------------------------

void* GrowOnlyPodAllocator::allocate( size_t size )
{
	MF_ASSERT_NOALLOC( size < pageSize_ );

	// check to see if we have room in current page
	if (!currentPage_ || ((currentPage_->used + size) > pageSize_))
	{
		this->addPage();
	}

	void *ptr = (char*)currentPage_ + currentPage_->used;
	currentPage_->used += size;				
	return ptr;
}

//------------------------------------------------------------------------------------

void GrowOnlyPodAllocator::addPage()
{
	// allocate space and initialise
	PageHeader *newPage = (PageHeader *) Allocator::heapAllocate( pageSize_ );
	newPage->used		= sizeof(PageHeader);

	// link to front of list
	newPage->nextPage = currentPage_;
	currentPage_ = newPage;
}

//------------------------------------------------------------------------------------

void GrowOnlyPodAllocator::printUsageStats() const
{
	size_t numPages = 0;
	size_t totalMem = 0;
	size_t usedMem = 0;

	PageHeader *page = currentPage_;
	while(page)
	{
		++numPages;
		totalMem += pageSize_;
		usedMem += page->used;
		page = page->nextPage;
	}

	dprintf( "GrowOnlyPodAllocator Memory usage : %lu pools : %lu KB total : %lu KB used : %lu KB free\n", numPages, totalMem / 1024, usedMem / 1024, (totalMem-usedMem) / 1024 );
}

} // end namespace
