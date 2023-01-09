#include "pch.hpp"

#include "bw_safe_allocatable.hpp"

#include "bw_memory.hpp"

namespace BW
{

/**
 *	This method overrides 'operator new' for this class, forwarding to
 *	::bw_new, even if 'new' was called with a different active
 *	global allocator.
 */
/* static */ void * SafeAllocatable::operator new( std::size_t size )
{
	return ::bw_new( size );
}


/**
 *	This method overrides 'operator new[]' for this class, forwarding to
 *	::bw_new_array, even if 'new[]' was called with a different active
 *	global allocator.
 */
/* static */ void * SafeAllocatable::operator new[]( std::size_t size )
{
	return ::bw_new_array( size );
}


/**
 *	This method overrides 'operator delete' for this class, forwarding to
 *	::bw_delete, even if 'delete' was called with a different active
 *	global allocator.
 */
/* static */ void SafeAllocatable::operator delete( void * ptr )
{
	return ::bw_delete( ptr );
}


/**
 *	This method overrides 'operator delete[]' for this class, forwarding to
 *	::bw_delete_array, even if 'delete[]' was called with a different active
 *	global allocator.
 */
/* static */ void SafeAllocatable::operator delete[]( void * ptr )
{
	return ::bw_delete_array( ptr );
}


} // namespace BW

// bw_safe_allocatable.cpp
