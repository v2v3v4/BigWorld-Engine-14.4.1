#ifndef BW_HASH64_HPP
#define BW_HASH64_HPP

#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/cstdmf_dll.hpp"
#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

namespace Hash64
{
	CSTDMF_DLL uint64 compute( const void * data, size_t length );
	CSTDMF_DLL uint64 compute( const char * value );
	CSTDMF_DLL uint64 compute( const BW::string & value );
	CSTDMF_DLL uint64 compute( const BW::StringRef & value );
	CSTDMF_DLL uint64 compute( const int & value );
	CSTDMF_DLL uint64 compute( const unsigned int & value );
	CSTDMF_DLL uint64 compute( const __int64 & value );
	CSTDMF_DLL uint64 compute( const unsigned __int64 & value );

	template< typename T >
	void combine( uint64 & seed, const T & value )
	{
		seed ^= compute( value ) +
			//2^64/phi.
			0x9E3779B97F4A7C15 +
			//make sure bits spread across the output even if input hashes
			//have a small output range.
			(seed << 5) + (seed >> 3);
	}
}

BW_END_NAMESPACE

#endif // BW_HASH_HPP
