#ifndef BW_SAFE_ALLOCATABLE_HPP
#define BW_SAFE_ALLOCATABLE_HPP
#pragma once

#include "cstdmf_dll.hpp"

#include <cstddef>

namespace BW
{
/**
 *	This provides a class to inherit from that ensures that allocations
 *	and deallocations of class instances use the BigWorld memory allocator,
 *	avoiding an issue when a class in a DLL is allocated by the DLL-loading
 *	application, but deleted by code within the DLL.
 *
 *	Note that Visual Studio inlines 'operator delete' at build-time in non-
 *	Debug builds for virtual base classes, so merely calling 'new' and 'delete'
 *	in the application isn't enough to avoid this problem.
 *
 *	As an alternative to using this class, the ensure class can be exported
 *	in the DLL, and given a virtual destructor, in which case Visual Studio
 *	should not inline the 'operator delete' for the class.
 *
 *	See http://support.microsoft.com/kb/122675
 */
class SafeAllocatable
{
public:
	CSTDMF_DLL static void * operator new( std::size_t size );
	CSTDMF_DLL static void * operator new[]( std::size_t size );
	CSTDMF_DLL static void operator delete( void * ptr );
	CSTDMF_DLL static void operator delete[]( void * ptr );

protected:
	SafeAllocatable() {}
	~SafeAllocatable() {}
};
}

#endif // BW_SAFE_ALLOCATABLE_HPP
