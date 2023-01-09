#include "pch.hpp"
#include "bw_memory.hpp"
#include "allocator.hpp"

#include <new>
#include "concurrency.hpp"

#include <cstring>
#include <cwchar>

//-----------------------------------------------------------------------------

namespace
{

std::new_handler globalNewHandler()
{
#if (__cplusplus >= 201103L && (!defined(__GLIBCXX__) || __GLIBCXX__ > 20150623)) \
	|| (_MSC_VER >= 1700)
	return std::get_new_handler();
#else
	// pre C++11 implementation
	static BW::SimpleMutex mutex;
	BW::SimpleMutexHolder lock( mutex );
	std::new_handler handler = std::set_new_handler( NULL );
	std::set_new_handler( handler );
	return handler;
#endif
}

} // namespace anonymous

//-----------------------------------------------------------------------------

void * bw_new( size_t size )
{
	if (size == 0)
	{
		size = 1;
	}

	while (true)
	{
		if (void * p = BW::Allocator::allocate( size ))
		{
			return p;
		}

		if (std::new_handler handler = globalNewHandler())
		{
			(*handler)();
		}
		else
		{
			throw std::bad_alloc();
		}
	}
}

//-----------------------------------------------------------------------------

void * bw_new( size_t size, const std::nothrow_t & ) throw()
{
	if (size == 0)
	{
		size = 1;
	}

	return BW::Allocator::allocate( size );
}

//-----------------------------------------------------------------------------

void * bw_new_array( size_t size )
{
	if (size == 0)
	{
		size = 1;
	}

	while (true)
	{
		if (void * p = BW::Allocator::allocate( size ))
		{
			return p;
		}

		if (std::new_handler handler = globalNewHandler())
		{
			(*handler)();
		}
		else
		{
			throw std::bad_alloc();
		}
	}
}

//-----------------------------------------------------------------------------

void * bw_new_array( size_t size, const std::nothrow_t & ) throw()
{
	if (size == 0)
	{
		size = 1;
	}

	return BW::Allocator::allocate( size );
}

//-----------------------------------------------------------------------------

void bw_delete( void * p ) throw()
{
	BW::Allocator::deallocate( p );
}

//-----------------------------------------------------------------------------

void bw_delete( void * p, const std::nothrow_t & ) throw()
{
	BW::Allocator::deallocate( p );
}

//-----------------------------------------------------------------------------

void bw_delete_array( void * p ) throw()
{
	BW::Allocator::deallocate( p );
}

//-----------------------------------------------------------------------------

void bw_delete_array( void * p, const std::nothrow_t & ) throw()
{
	BW::Allocator::deallocate( p );
}

//-----------------------------------------------------------------------------

void * bw_malloc( size_t size )
{
	return BW::Allocator::allocate( size );
}

//-----------------------------------------------------------------------------

void bw_free( void * ptr ) 
{
	return BW::Allocator::deallocate( ptr );
}

//-----------------------------------------------------------------------------

void * bw_malloc_aligned( size_t size, size_t alignment )
{
	return BW::Allocator::heapAllocateAligned( size, alignment );
}

//-----------------------------------------------------------------------------

void bw_free_aligned( void * ptr )
{
	BW::Allocator::heapDeallocateAligned( ptr );
}

//-----------------------------------------------------------------------------

void * bw_realloc( void* ptr, size_t size )
{
	return BW::Allocator::reallocate( ptr, size );
}

//-----------------------------------------------------------------------------

char * bw_strdup( const char * s )
{
	size_t len = strlen( s ) + 1;
	size_t size = len * sizeof(char);
	char* ns = ( char* )BW::Allocator::allocate( size );
	memcpy( ns, s, size );
	return ns;
}

//-----------------------------------------------------------------------------

wchar_t * bw_wcsdup( const wchar_t * s )
{
	size_t len = wcslen( s ) + 1;
	size_t size = len * sizeof(wchar_t);
	wchar_t* ns = ( wchar_t * )BW::Allocator::allocate( size );
	memcpy( ns, s, size );
	return ns;
}

//-----------------------------------------------------------------------------

size_t bw_memsize( void * p )
{
	return BW::Allocator::memorySize( p );
}
