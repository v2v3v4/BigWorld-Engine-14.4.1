#ifndef BINARY_FILE_HPP
#define BINARY_FILE_HPP

#include <stdio.h>
#include "cstdmf/bw_map.hpp"

#include "stdmf.hpp"
#include "bw_memory.hpp"
#include "bw_string.hpp"
#include "debug.hpp"

#include <climits>

BW_BEGIN_NAMESPACE


class BinaryFile;

template <class T>
BinaryFile & operator<<( BinaryFile & bf, const T & data );

BinaryFile & operator<<( BinaryFile & bf, const char * s );

BinaryFile & operator<<( BinaryFile & bf, const BW::string & s );

template <class T>
BinaryFile & operator>>( BinaryFile & bf, T & data );

BinaryFile & operator>>( BinaryFile & bf, char * s );
BinaryFile & operator>>( BinaryFile & bf, BW::string & s );


/**
 *	This class provides a wrapper to FILE * which makes it easy
 *	to read and write to a binary format. It can also wrap (or copy)
 *	an existing buffer in memory, making it easy to read from or
 *	write to it.
 */
class CSTDMF_DLL BinaryFile
{
public:
	static FILE * const FILE_INTERNAL_BUFFER;

	BinaryFile( FILE * bf );
	~BinaryFile();

	operator const void*() const			{ return error() ? 0 : this; }
	bool error() const						{ return error_; }
	void error( bool error )				{ error_ = error; }

	FILE * file()						{ return file_; }
	const FILE * file() const			{ return file_; }

	/**
	 *	@return The current buffer pointer for a memory-backed object (NULL
	 *		otherwise). Note that this pointer is only guaranteed to remain
	 *		valid until the next file operation is performed.
	 */
	const void * data() const			{ return data_; }
	/**
	 *	@return The size of the buffer for data(). See bytesAvailable to query
	 *		the current cursor position in the buffer.
	 */
	long dataSize() const				{ return bytesCached_; }

	long cache();

	void wrap( void * data, int len, bool makeCopy );

	void * reserve( int len );
	
	template <class MAP>
	BinaryFile & readMap( MAP & map )
	{
		map.clear();
	
		// number of elements is uint32 to preserve 64bit compatibility
		uint32 sz = 0;
		(*this) >> sz;

		//Assert a maximum sane sequence size to pick up data errors
		//in debug builds.
		const std::size_t MAX_EXPECTED_SEQUENCE_ELEMENTS = 100 * 1000 * 1000;
		MF_ASSERT( sz <= MAX_EXPECTED_SEQUENCE_ELEMENTS );

		for (typename MAP::size_type i=0; i < sz && !error(); i++)
		{
			typename MAP::value_type	vt;
			(*this) >> const_cast< typename MAP::key_type & >( vt.first ) >>
				vt.second;

			map.insert( vt );
		}

		return *this;
	}

	template <class MAP>
	BinaryFile & writeMap( const MAP & map )
	{
		// number of elements is uint32 to preserve 64bit compatibility
		(*this) << (uint32)map.size();

		for (typename MAP::const_iterator it = map.begin();
			it != map.end();
			it++)
		{
			(*this) << (*it).first << (*it).second;
		}
		return *this;
	}

	template <class VEC>
	BinaryFile & readSequence( VEC & vec )
	{
		vec.clear();

		// number of elements is uint32 to preserve 64bit compatibility
		uint32 sz = 0;
		(*this) >> sz;

		//Assert a maximum sane sequence size to pick up data errors
		//in debug builds.
		const std::size_t MAX_EXPECTED_SEQUENCE_ELEMENTS = 100 * 1000 * 1000;
		MF_ASSERT( sz <= MAX_EXPECTED_SEQUENCE_ELEMENTS );

		vec.reserve( sz );

		for (typename VEC::size_type i=0; i < sz && !error(); i++)
		{
			typename VEC::value_type	vt;
			(*this) >> vt;

			vec.push_back( vt );
		}

		return *this;
	}

	template <class VEC>
	BinaryFile & writeSequence( const VEC & vec )
	{
		// number of elements is uint32 to preserve 64bit compatibility
		(*this) << (uint32)vec.size();

		for (typename VEC::const_iterator it = vec.begin();
			it != vec.end();
			++it)
		{
			(*this) << (*it);
		}

		return *this;
	}

	size_t cursorPosition();
	size_t bytesAvailable();
	void rewind();

	size_t read( void * buffer, size_t nbytes );
	size_t write( const void * buffer, size_t nbytes );

private:
	FILE * file_;
	void * data_;
	char * ptr_;
	long bytesCached_;
	bool error_;
};



template <class T>
inline BinaryFile & operator<<( BinaryFile & bf, const T & data )
{
	bf.write( &data, sizeof(T) );

	return bf;
}

inline BinaryFile & operator<<( BinaryFile & bf, const char * s )
{
	size_t fullLen = strlen( s );
	MF_ASSERT( fullLen <= INT_MAX );
	int	len = ( int )fullLen;
	bf.write( &len, sizeof(int) );
	bf.write( s, len );
	return bf;
}

inline BinaryFile & operator<<( BinaryFile & bf, const BW::string & s )
{
	size_t fullLen = s.length();
	MF_ASSERT( fullLen <= INT_MAX );
	int	len = ( int )fullLen;
	bf.write( &len, sizeof(int) );
	bf.write( s.data(), len );
	return bf;
}


template <class T>
inline BinaryFile & operator>>( BinaryFile & bf, T & data )
{
	// read bytes
	const size_t	size= sizeof(T);
	size_t			n	= bf.read( &data, size );
	
	// check length
	if (n != size)
	{
		bf.error( true );
		WARNING_MSG( "BinaryFile& operator>> expected to read %d bytes, got %d.\n",
					 (int)size, (int)n );
	}

	return bf;
}

inline BinaryFile & operator>>( BinaryFile & bf, char * s )
{
	// read string length.
	const size_t	size= sizeof(int);
	size_t			len = 0;
	size_t			n	= bf.read( &len, size );

	if ( n == size )
	{
		if( len <= bf.bytesAvailable() )
		{
			// if length is ok and non-zero try to read string.
			if ( len > 0 )
			{
				n = bf.read( s, len );

				// check actual amount read.
				if ( n != len)
				{
					WARNING_MSG( "BinaryFile& operator>> expected string of"
						"length %zd, got %zd.\n", len, n );
					bf.error( true );
				}
			}
		}
		else
		{
			bf.error( true );
			WARNING_MSG( "BinaryFile& operator>> invalid string length %zd.\n", len );
		}
	}
	else
	{
		bf.error( true );
		WARNING_MSG( "BinaryFile& operator>> couldn't read string length.\n" );
	}

	return bf;
}

inline BinaryFile & operator>>( BinaryFile & bf, BW::string & s )
{
	// read string length.
	const size_t	size= sizeof(int);
	size_t			len = 0;
	size_t			n	= bf.read( &len, size );

	if ( n == size )
	{
		if( len <= bf.bytesAvailable() )
		{
			// if length is ok and non-zero try to read string.
			if ( len > 0 )
			{
				// resize string to match incoming.
				s.resize( len );
				n = bf.read( const_cast<char*>( s.data() ), len );
				
				// check actual amount read - bf.read wouldn't have read more bytes,
				// only less or equal. Assert the "impossible" condition and handle
				// other condition - we couldn't read enough.
				MF_ASSERT( !( n > len ) && "Overwrote buffer while reading!" );
				if ( n < len)
				{
					// resize back.
					s.resize( n );
					bf.error( true );
					WARNING_MSG( "BinaryFile& operator>> expected string of"
								 "length %zu, got %zu.\n", len, n );
				}
			}
		}
		else
		{
			bf.error( true );
			WARNING_MSG( "BinaryFile& operator>> invalid string length %zd.\n", len );
		}
	}
	else
	{
		bf.error( true );
		WARNING_MSG( "BinaryFile& operator>> couldn't read string length.\n" );
	}

	return bf;
}

BW_END_NAMESPACE

#endif // BINARY_FILE_HPP
