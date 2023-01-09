#include "pch.hpp"

#include "cstdmf/bw_memory.hpp"

#include <time.h>
#include <stdint.h>

BW_BEGIN_NAMESPACE

static void testAlign( size_t align, 
	TestResult & result_,
	const char * m_name )
{
	void * ptr = bw_malloc_aligned( 16, align );
	CHECK( ((uintptr_t)ptr % align) == 0 );
	bw_free_aligned( ptr );
}

TEST( AlignedAlloc_test )
{
	testAlign( 4, result_, m_name );
	testAlign( 8, result_, m_name );
	testAlign( 16, result_, m_name );
	testAlign( 32, result_, m_name );
	testAlign( 64, result_, m_name );
}

BW_END_NAMESPACE


// test_aligned_allocate.cpp

