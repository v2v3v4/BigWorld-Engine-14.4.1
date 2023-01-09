#include "pch.hpp"
#include "bw_hash.hpp"
#include "stdmf_minimal.hpp"

namespace BW
{

//-----------------------------------------------------------------------------

// FNV-1a hash from http://www.isthe.com/chongo/tech/comp/fnv/#FNV-1a
namespace 
{
#if defined( _LP64 ) || defined( _WIN64 )
static const size_t FNV_prime = 1099511628211UL;
static const size_t FNV_offset_basis = 14695981039346656037UL;
#else // _LP64
static const size_t FNV_prime = 16777619UL;
static const size_t FNV_offset_basis = 2166136261UL;
#endif // _LP64
}

//-----------------------------------------------------------------------------

size_t hash_string( const void* ptr, size_t size )
{
	BW_STATIC_ASSERT( sizeof(size_t) == 4 || sizeof(size_t) == 8, not_32_or_64_bit_platform );

	size_t result = FNV_offset_basis;
	const unsigned char * const & rData = reinterpret_cast< const unsigned char * >( ptr );
	for (size_t i = 0; i < size; ++i)
	{
		result = (result ^ rData[ i ]) * FNV_prime;
	}
	return result;
}

//-----------------------------------------------------------------------------

size_t hash_string( const char* str )
{
	BW_STATIC_ASSERT( sizeof(size_t) == 4 || sizeof(size_t) == 8, not_32_or_64_bit_platform );
	size_t result = FNV_offset_basis;
	const unsigned char * data = reinterpret_cast< const unsigned char * >( str );
	while (*data)
	{
		result = (result ^ *data) * FNV_prime;
		++data;
	}
	return result;
}

} // namespace BW
