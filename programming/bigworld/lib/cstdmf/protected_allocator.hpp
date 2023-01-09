#ifndef BW_PROTECTED_ALLOCATOR_HPP
#define BW_PROTECTED_ALLOCATOR_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/allocator.hpp"

namespace BW
{

namespace ProtectedAllocator
{
	/// allocate some memory
	void* allocate( size_t size );

	/// free some memory
	void deallocate( void* ptr );

	/// reallocate some memory
	void* reallocate( void* ptr, size_t size );

	/// allocate some aligned memory
	void* allocateAligned( size_t size, size_t alignment );

	/// free some memory allocated with allocate_aligned
	void deallocateAligned( void* ptr );

	/// returns size of allocated block
	size_t memorySize( void* ptr );

    /// report debug information
    void debugReport( StringBuilder& builder );
	
	/// Perform clean up on thread finish
	void onThreadFinish();
}

} // end namespace

#endif // BW_PROTECTED_ALLOCATOR_HPP

