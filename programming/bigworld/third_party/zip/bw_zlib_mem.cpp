/**
 *	Bigworld allocators hooks for zlib
 */

#include <cstddef>

extern void* bw_malloc( size_t size );
extern void bw_free( void* ptr );

extern "C"
{

#include "zutil.h"

voidpf zcalloc OF( ( voidpf opaque, unsigned items, unsigned size ) )
{
	return bw_malloc( items * size );
}

void zcfree OF( ( voidpf opaque, voidpf ptr ) )
{
	bw_free( ptr );
}

};
