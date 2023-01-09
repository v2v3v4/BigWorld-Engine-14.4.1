#include "pch.hpp"

#if !defined(BW_EXPORTER)
#include "ned_allocator.hpp"
#include "string_builder.hpp"

#define RUNTIME_MEMORY_INFO 1
#define NO_NED_NAMESPACE

#if RUNTIME_MEMORY_INFO
struct mallinfo {
  size_t arena;    /* non-mmapped space allocated from system */
  size_t ordblks;  /* number of free chunks */
  size_t smblks;   /* always 0 */
  size_t hblks;    /* always 0 */
  size_t hblkhd;   /* space in mmapped regions */
  size_t usmblks;  /* maximum total allocated space */
  size_t fsmblks;  /* always 0 */
  size_t uordblks; /* total allocated space */
  size_t fordblks; /* total free space */
  size_t keepcost; /* releasable (via malloc_trim) space */
};
#else
#define NO_MALLINFO 1
#endif

#include "nedalloc/nedmalloc.h"

namespace BW
{

//-----------------------------------------------------------------------------

void* NedAllocator::allocate( size_t size )
{
    return nedmalloc( size );
}

//-----------------------------------------------------------------------------

void NedAllocator::deallocate( void* ptr )
{
	if (ptr)
	{
		nedfree( ptr );
	}
}

//-----------------------------------------------------------------------------

void* NedAllocator::reallocate( void* ptr, size_t size )
{
    return nedrealloc( ptr, size );
}

//-----------------------------------------------------------------------------

void* NedAllocator::allocateAligned( size_t size, size_t alignment )
{
	return nedmemalign( alignment, size );
}

//-----------------------------------------------------------------------------

void NedAllocator::deallocateAligned( void* ptr )
{
	// nedfree doesn't handle null pointers!
	if ( ptr )
		nedfree( ptr );
}


//-----------------------------------------------------------------------------

size_t NedAllocator::memorySize( void* ptr )
{
	return nedblksize( ptr );
}

//-----------------------------------------------------------------------------

void NedAllocator::debugReport( StringBuilder& builder )
{
#if RUNTIME_MEMORY_INFO
		mallinfo minfo = nedmallinfo( );
		builder.append( "********************** Malloc info **********************\n" );
		builder.appendf( "non-mmapped space allocated from system = %d\n", minfo.arena );
		builder.appendf( "number of free chunks info = %d\n", minfo.ordblks );
		builder.appendf( "space in mmapped regions info = %d\n", minfo.hblkhd );
		builder.appendf( "maximum total allocated space info = %d\n", minfo.usmblks );
		builder.appendf( "total allocated space = %d\n", minfo.uordblks );
		builder.appendf( "total free space = %d\n", minfo.fordblks );
		builder.appendf( "releasable (via malloc_trim) space = %d\n", minfo.keepcost );
#endif
}

//-----------------------------------------------------------------------------

void NedAllocator::onThreadFinish()
{
	neddisablethreadcache( NULL );
}

//-----------------------------------------------------------------------------

} // end namespace

#endif // BW_EXPORTERS
