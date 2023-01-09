#ifndef STRING_BUILDER_HPP
#define STRING_BUILDER_HPP

#include "cstdmf/debug.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_string_ref.hpp"

#include <stdarg.h>

namespace BW
{

/// wrapper to simplify piecemeal construction of a string 
class CSTDMF_DLL StringBuilder
{
public:
	/// use user supplied memory, caller is responsible for freeing
	/// this memory if necessary.
	StringBuilder( char * buffer, size_t bufferSize );

	/// constructor, allocates memory off the heap
	StringBuilder( size_t bufferSize );

	/// destructor
	~StringBuilder();

	/// \returns null terminated string
	const char * string() const;

	/// append a single character to the string
	void append( char c );

	/// append an integer to the string
	void append( int c );

	/// append a null terminated string
	void append( const char * str );

	/// append a string ref
	void append( const StringRef& str );

	/// append a printf style formatted string
	void appendf( const char * format, ... );

	/// append printf style formatted string
	void vappendf( const char * format, va_list args );

	/// copy to the destination string, truncating if it is too small.
	void copyTo( char * dst, size_t len ) const;

	/// \returns free space left in internal buffer
	size_t numFree() const
	{
		MF_ASSERT( pos_ < bufferSize_ );
		size_t numFree = bufferSize_ - pos_ - 1;
		return numFree;
	}

	/// length of the string currently in the buffer
	size_t length() const
	{
		return pos_;
	}

	/// \returns true if the buffer is full
	bool isFull() const
	{ 
		return pos_ == bufferSize_ - 1; 
	}

	/// reset the buffer
	void clear() 
	{ 
		pos_ = 0;
		*str_ = '\0';
	}

	char operator[](size_t index) const;

	char& operator[](size_t index);

	void replace( char oldVal, char newVal );

private:
	// noncopyable
	void operator=( const StringBuilder & );

private:
	// needs to be writable in the string() function so it can be null
	// terminated
	mutable char *str_;
	size_t pos_;
	size_t bufferSize_;
	bool shouldDelete_;
}; 


} // end namespace BW

#endif // STRING_BUILDER_HPP
