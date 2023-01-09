#include "pch.hpp"
#include "bw_hash64.hpp"

BW_BEGIN_NAMESPACE

namespace Hash64
{
	static const uint64 FNV_prime = 1099511628211UL;
	static const uint64 FNV_offset_basis = 14695981039346656037UL;

	uint64 compute( const void * data, size_t length )
	{
		const char * input = static_cast< const char* >( data );
		uint64 result = FNV_offset_basis;
		for (size_t i = 0; i < length; ++i)
		{
			result = ( result ^ input[ i ] ) * FNV_prime;
		}
		return result;
	}

	uint64 compute( const char * value )
	{
		const char * input = value;
		uint64 result = FNV_offset_basis;
		while (*input)
		{
			result = ( result ^ *input ) * FNV_prime;
			++input;
		}
		return result;
	}

	uint64 compute( const BW::string & value )
	{
		return compute( value.c_str(), value.length() );
	}

	uint64 compute( const BW::StringRef & value )
	{
		return compute( value.data(), value.length() );
	}

	uint64 compute( const int & value )
	{
		return compute( static_cast< __int64 >( value ) );
	}

	uint64 compute( const unsigned int & value )
	{
		return compute( static_cast< unsigned __int64 >( value ) );
	}

	uint64 compute( const __int64 & value )
	{
		return compute( ( const void* )&value, sizeof( __int64 ) );
	}

	uint64 compute( const unsigned __int64 & value )
	{
		return compute( ( const void* )&value, sizeof( unsigned __int64 ) );
	}
}

BW_END_NAMESPACE
