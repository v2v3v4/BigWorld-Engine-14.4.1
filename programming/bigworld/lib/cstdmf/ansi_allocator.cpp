#include "pch.hpp"

#include "ansi_allocator.hpp"

#include "debug.hpp"

#include <memory.h>

#if defined( __APPLE__ )
#include <malloc/malloc.h>
#endif // defined( __APPLE__ )

namespace BW
{

//-----------------------------------------------------------------------------

void* AnsiAllocator::allocate( size_t size )
{
    return ::malloc( size );
}

//-----------------------------------------------------------------------------

void AnsiAllocator::deallocate( void* ptr )
{
    ::free( ptr );
}

//-----------------------------------------------------------------------------

void* AnsiAllocator::reallocate( void* ptr, size_t size )
{
    return ::realloc( ptr, size );
}

//-----------------------------------------------------------------------------

void* AnsiAllocator::allocateAligned( size_t size, size_t alignment )
{
#if defined(_MSC_VER)
	return ::_aligned_malloc( size, alignment );
#else
#if !defined( __ANDROID__ )
	void* ptr = 0;
	::posix_memalign( &ptr, alignment, size );
	return ptr;
#else
	return ::memalign( alignment, size );
#endif
#endif
}

//-----------------------------------------------------------------------------

void AnsiAllocator::deallocateAligned( void* ptr )
{
#if defined(_MSC_VER)
	::_aligned_free( ptr );
#else
	::free( ptr );
#endif
}

//-----------------------------------------------------------------------------

size_t AnsiAllocator::memorySize( void* ptr )
{
#if defined( _MSC_VER )
	return ::_msize( ptr );
#elif (defined( __linux__ ) || defined( EMSCRIPTEN )) && !defined( __ANDROID__ )
	return ::malloc_usable_size( ptr );
#elif defined( __APPLE__ )
	return ::malloc_size( ptr );
#else
	MF_ASSERT( false &&
		"memorySize() not supported" );
	return 0;
#endif
}

//-----------------------------------------------------------------------------

void AnsiAllocator::debugReport( StringBuilder& )
{
    // no-op
}

//-----------------------------------------------------------------------------

void AnsiAllocator::onThreadFinish()
{
	// no-op
}

//-----------------------------------------------------------------------------

} // end namespace

