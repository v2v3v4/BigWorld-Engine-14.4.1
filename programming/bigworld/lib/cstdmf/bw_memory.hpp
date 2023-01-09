#ifndef BW_MEMORY_HPP
#define BW_MEMORY_HPP

#include "cstdmf/cstdmf_dll.hpp"
#include <cstddef>

// forward declare std::nothrow_t
namespace std { struct nothrow_t; }

CSTDMF_DLL void *    bw_new( size_t size );
CSTDMF_DLL void *    bw_new( size_t size, const std::nothrow_t & ) throw();
CSTDMF_DLL void *    bw_new_array( size_t size );
CSTDMF_DLL void *    bw_new_array( size_t size, const std::nothrow_t & ) throw();
CSTDMF_DLL void      bw_delete( void * ptr ) throw();
CSTDMF_DLL void      bw_delete( void * ptr, const std::nothrow_t & ) throw();
CSTDMF_DLL void      bw_delete_array( void * ptr ) throw();
CSTDMF_DLL void      bw_delete_array( void * ptr, const std::nothrow_t & ) throw();
CSTDMF_DLL void *    bw_malloc( size_t size );
CSTDMF_DLL void      bw_free( void * ptr );
CSTDMF_DLL void *    bw_malloc_aligned( size_t size, size_t alignment );
CSTDMF_DLL void      bw_free_aligned( void * ptr );
CSTDMF_DLL void *    bw_realloc( void * ptr, size_t size );
CSTDMF_DLL char *    bw_strdup( const char * s );
CSTDMF_DLL wchar_t * bw_wcsdup( const wchar_t * s );
CSTDMF_DLL size_t    bw_memsize( void * p );

#endif // BW_MEMORY_HPP
