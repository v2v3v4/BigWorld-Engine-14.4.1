#ifndef BW_HASH_HPP
#define BW_HASH_HPP

#include "cstdmf/cstdmf_dll.hpp"

#if _MSC_VER || defined( EMSCRIPTEN )
// MS Visual Studio's C++ library always puts the struct hash template in std
#include <functional>
#define BW_HASH_NAMESPACE_BEGIN namespace std {
#define BW_HASH_NAMESPACE_END }
#define BW_HASH_NAMESPACE std::
#else
// libstdc++ puts class hash in std or std::tr1 based on which header you use
#include <tr1/functional>
#define BW_HASH_NAMESPACE_BEGIN namespace std { namespace tr1 {
#define BW_HASH_NAMESPACE_END } }
#define BW_HASH_NAMESPACE std::tr1::
#endif

#include <cstddef>
#include <utility>

namespace BW
{
	CSTDMF_DLL std::size_t hash_string( const void* ptr, std::size_t size );
	CSTDMF_DLL std::size_t hash_string( const char* str );

#if _MSC_VER || defined( EMSCRIPTEN )
using std::hash;
#else
using std::tr1::hash;
#endif

///For combining hashes from multiple sources.
template<typename T>
void hash_combine( std::size_t & seed, const T & value )
{
	BW::hash<T> hasher;
	seed ^= hasher( value ) +
		//A random value.
		0xBC6EF372 +
		//make sure bits spread across the output even if input hashes
		//have a small output range.
		(seed << 5) + (seed >> 3);
}
} // namespace BW

BW_HASH_NAMESPACE_BEGIN

///Specialize hashing for std::pair.
template<typename S, typename T>
struct hash< std::pair< S, T > > :
	public unary_function< std::pair< S, T >, std::size_t >
{
public:
	std::size_t operator()( const std::pair< S, T > & v ) const
	{
		std::size_t seed = 0;
		BW::hash_combine( seed, v.first );
		BW::hash_combine( seed, v.second );
		return seed;
	}
};

BW_HASH_NAMESPACE_END

#endif // BW_HASH_HPP
