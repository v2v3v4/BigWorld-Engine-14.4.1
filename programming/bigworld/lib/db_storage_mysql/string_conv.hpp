#ifndef MYSQL_STRING_CONV_HPP
#define MYSQL_STRING_CONV_HPP

#include "helper_types.hpp"

#include "cstdmf/stdmf.hpp"

#include <sstream>
#include <stdexcept>


BW_BEGIN_NAMESPACE

namespace StringConv
{
	inline void toValue( float& value, const char * str )
	{
		char* remainder;
		value = strtof( str, &remainder );
		if (*remainder)
			throw std::runtime_error( "not a number" );
	}

	inline void toValue( double& value, const char * str )
	{
		char* remainder;
		value = strtod( str, &remainder );
		if (*remainder)
			throw std::runtime_error( "not a number" );
	}

	inline void toValue( int32& value, const char * str )
	{
		char* remainder;
		value = strtol( str, &remainder, 10 );
		if (*remainder)
			throw std::runtime_error( "not a number" );
	}


	inline void toValue( int8& value, const char * str )
	{
		int32 i;
		toValue( i, str );
		value = int8(i);
		if (value != i)
			throw std::runtime_error( "out of range" );
	}

	inline void toValue( int16& value, const char * str )
	{
		int32 i;
		toValue( i, str );
		value = int16(i);
		if (value != i)
			throw std::runtime_error( "out of range" );
	}

	inline void toValue( uint32& value, const char * str )
	{
		char* remainder;
		value = strtoul( str, &remainder, 10 );
		if (*remainder)
			throw std::runtime_error( "not a number" );
	}

	inline void toValue( uint8& value, const char * str )
	{
		uint32 ui;
		toValue( ui, str );
		value = uint8(ui);
		if (value != ui)
			throw std::runtime_error( "out of range" );
	}

	inline void toValue( uint16& value, const char * str )
	{
		uint32 ui;
		toValue( ui, str );
		value = uint16(ui);
		if (value != ui)
			throw std::runtime_error( "out of range" );
	}

	inline void toValue( int64& value, const char * str )
	{
		char* remainder;
		value = strtoll( str, &remainder, 10 );
		if (*remainder)
			throw std::runtime_error( "not a number" );
	}

	inline void toValue( uint64& value, const char * str )
	{
		char* remainder;
		value = strtoull( str, &remainder, 10 );
		if (*remainder)
			throw std::runtime_error( "not a number" );
	}

	inline void toValue( MySqlTimestampNull& value, const char * str )
	{
		MYSQL_TIME& timestamp = value.getBuf();

		int numAssigned = sscanf( str, "%d-%d-%d %d:%d:%d", &timestamp.year,
				&timestamp.month, &timestamp.day, &timestamp.hour,
				&timestamp.minute, &timestamp.second );
		if (numAssigned == 6)
		{
			timestamp.second_part = 0;
			value.valuefy();
		}
		else
		{
			throw std::runtime_error( "not a timestamp" );
		}
	}

	template < class TYPE >
	inline void addToStream( std::ostream& strm, const TYPE& value )
	{
		strm << value;
	}

	// Special version for int8 and uint8 because they are characters but
	// we want them formatted as a number.
	inline void addToStream( std::ostream& strm, int8 value )
	{
		strm << int( value );
	}
	inline void addToStream( std::ostream& strm, uint8 value )
	{
		strm << int( value );
	}

	inline void addToStream( std::ostream& strm, float value )
	{
		strm.precision( 8 );
		strm << value;
	}

	inline void addToStream( std::ostream& strm, double value )
	{
		strm.precision( 17 );
		strm << value;
	}

	inline void addToStream( std::ostream & strm, bool value )
	{
		strm << (value ? "TRUE" : "FALSE" );
	}

	template <class TYPE>
	inline BW::string toStr( const TYPE& value )
	{
		BW::stringstream ss;
		StringConv::addToStream( ss, value );
		return ss.str();
	}

} // namespace StringConv

BW_END_NAMESPACE

#endif // MYSQL_STRING_CONV_HPP
