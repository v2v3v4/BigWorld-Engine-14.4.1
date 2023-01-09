#ifndef __CSTDMF_MEMORY_LOAD_HPP__
#define __CSTDMF_MEMORY_LOAD_HPP__

#ifdef WIN32

#include "stdmf.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Runtime memory information
 */
namespace Memory
{
	typedef DWORDLONG MemSize;
	/**
	*	returns maximum available memory
	*/
	CSTDMF_DLL MemSize	maxAvailableMemory();
	/**
	*	returns how much memory is currently in use
	*/
	CSTDMF_DLL MemSize	usedMemory();
	/**
	*	returns current memory load [ 0; 100 ]
	*/
	CSTDMF_DLL float	memoryLoad();
	/**
	*	returns largest available memory block size
	*	expensive 
	*/
	CSTDMF_DLL MemSize	largestBlockSize();
	/**
	*	returns amount of available VA space
	*/
	CSTDMF_DLL MemSize	availableVA();

} // namespace Memory

BW_END_NAMESPACE

#endif // WIN32

#endif // __CSTDMF_MEMORY_LOAD_HPP__
