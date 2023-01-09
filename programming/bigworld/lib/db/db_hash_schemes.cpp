#include "db_hash_schemes.hpp"

#include "server/murmur_hash.hpp"

namespace // (anonymous)
{

// Just a arbitrary uint.
const unsigned int MURMUR_HASH_SEED = 0xf99cd35e;

} // end namespace (anonymous)


BW_BEGIN_NAMESPACE


/**
 *	This function is the hash function used by the database layer's hash
 *	scheme.
 */
uint64 DBHashSchemes::hashFunction( void * data, size_t size )
{
	return MurmurHash::hash( data, size, MURMUR_HASH_SEED );
}


BW_END_NAMESPACE


// db_hash_schemes.cpp
