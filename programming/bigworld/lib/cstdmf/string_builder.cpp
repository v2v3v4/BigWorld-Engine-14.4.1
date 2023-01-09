#include "pch.hpp"
#include "string_builder.hpp"
#include "debug.hpp"

#include <stdarg.h>
#include <string.h>

namespace BW
{

//-----------------------------------------------------------------------------

StringBuilder::StringBuilder( char * buffer, size_t bufferSize ) :
	str_( buffer ),
	pos_( 0 ),
	bufferSize_( bufferSize ),
	shouldDelete_( false )
{
	MF_ASSERT( bufferSize_ > 0);
	memset( buffer, 0, bufferSize_ );
}

//-----------------------------------------------------------------------------

StringBuilder::StringBuilder( size_t bufferSize ) : 
	pos_( 0 ),
	bufferSize_( bufferSize ),
	shouldDelete_( true )
{
	MF_ASSERT( bufferSize_ > 0);
	str_ = new char[ bufferSize_ ];
}

//-----------------------------------------------------------------------------

StringBuilder::~StringBuilder()
{
	if (shouldDelete_)
	{
		bw_safe_delete_array( str_ );
	}
}

//-----------------------------------------------------------------------------

const char * StringBuilder::string() const 
{
	str_[pos_] = 0;
	return str_; 
}

//-----------------------------------------------------------------------------

void StringBuilder::replace( char oldVal, char newVal )
{
	std::replace( str_, str_ + pos_, oldVal, newVal );
}

//-----------------------------------------------------------------------------

char StringBuilder::operator[]( size_t index ) const
{
	MF_ASSERT( index < pos_ );
	return str_[index];
}

//-----------------------------------------------------------------------------

char& StringBuilder::operator[]( size_t index )
{
	MF_ASSERT( index < pos_ );
	return str_[index];
}

//-----------------------------------------------------------------------------

void StringBuilder::append( char c )
{
	if (this->isFull())
	{
		return;
	}

	str_[ pos_++ ] = c;
}

//-----------------------------------------------------------------------------

void StringBuilder::append( const char * str )
{
	while (*str && !this->isFull())
	{
		str_[ pos_++ ] = *str;
		++str;
	}
}

//-----------------------------------------------------------------------------

void StringBuilder::append( const StringRef& str )
{
	StringRef::const_iterator itr = str.begin();
	StringRef::const_iterator end = str.end();
	while ( itr < end && !this->isFull() )
	{
		str_[ pos_++ ] = *itr;
		++itr;
	}
}

//-----------------------------------------------------------------------------

void StringBuilder::append( int c )
{
	char buf[64] = { 0 };
	size_t len = bw_snprintf( buf, 63, "%d", c );
	size_t free_space = this->numFree();
	len = (len < free_space) ? len : free_space;
	strncpy( &str_[pos_], buf, len );
	pos_ += len;
}

//-----------------------------------------------------------------------------

void StringBuilder::appendf( const char * format, ... )
{
	va_list args;
	va_start( args, format );

	size_t remaining = this->numFree();
	int nc = bw_vsnprintf( &str_[pos_], remaining, format, args );

	if (nc > 0)
	{
		pos_ += std::min< size_t >( nc, remaining );
	}

	va_end( args );
}

//-----------------------------------------------------------------------------

void StringBuilder::vappendf( const char * format, va_list args )
{
	size_t remaining = this->numFree();
#ifndef _WIN32
	// We va_copy to not corrupt the existing args list
	va_list args2;
	bw_va_copy( args2, args );
	int nc = bw_vsnprintf( &str_[ pos_ ], remaining, format, args2 );
	va_end( args2 );
#else
	int nc = bw_vsnprintf( &str_[ pos_ ], remaining, format, args );
#endif

	if (nc > 0)
	{
		pos_ += std::min< size_t >( nc, remaining );
	}
}

//-----------------------------------------------------------------------------

void StringBuilder::copyTo( char * dst, size_t len ) const
{
	IF_NOT_MF_ASSERT_DEV( len > 0 )
	{
		// Destination buffer is size 0!
		return;
	}
	
	size_t copyCount = std::min( len - 1, length() );
	memcpy( dst, str_,copyCount );
	dst[ copyCount ] = 0;
}

//-----------------------------------------------------------------------------

} // end namespace BW

// string_builder.cpp
