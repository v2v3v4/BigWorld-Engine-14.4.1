#ifndef IALLOCATOR_HPP
#define IALLOCATOR_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class IAllocator
{
public:
	virtual void * intAllocate( size_t size, const void * hint ) = 0;
	virtual void intDeallocate( void * p, size_t size ) = 0;
	virtual bool canAllocate() = 0;
	virtual size_t getMaxSize() const = 0;
};

BW_END_NAMESPACE

#endif

