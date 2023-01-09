#ifndef MURMUR_HASH_HPP
#define MURMUR_HASH_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/stdmf.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This namespace exposes a hash function that uses the MurmurHash3 algorithm.
 */
namespace MurmurHash
{

uint64 hash( const void * data, size_t length, uint seed );

} // end namespace MurmurHash


BW_END_NAMESPACE

#endif // MURMUR_HASH_HPP
