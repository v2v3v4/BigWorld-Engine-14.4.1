#ifndef BINARY_STREAM_H
#define BINARY_STREAM_H

#include "stdmf.hpp"

#include "bw_string.hpp"
#include "debug.hpp"

#include "cstdmf/bw_vector.hpp"
#include <cstring>

BW_BEGIN_NAMESPACE

const int BS_BUFF_MAX_SIZE = 2048;

class BinaryIStream;

/**
 *	This is an interface for a binary output stream.
 *	Really simple does-it-all interface.
 *
 *	@ingroup network
 */
class BinaryOStream
{
public:
	virtual ~BinaryOStream() {};

	/**
	 *	This method reserves the input number of bytes on this stream.
	 *
	 *	@param nBytes	Number of bytes to reserve.
	 *
	 *	@return Pointer to the reserved memory.
	 */
	virtual void * reserve( int nBytes ) = 0;

	virtual int size() const	{ return -1; }
	inline void appendString( const char * data, int length );

	void transfer( BinaryIStream & from, int length );
	virtual void addBlob( const void * pBlob, int size );

	template <class TYPE>
	void insertRaw( const TYPE & t );

	inline void writePackedInt( int len );
	void writeStringLength( int len )	{ this->writePackedInt( len ); }

	static int calculatePackedIntSize( int value );
	static int calculateStreamedSizeOfString( int stringSize )
	{
		return calculatePackedIntSize( stringSize ) + stringSize;
	}
};


/**
 *	This is an interface for a binary input stream.
 *	Really simple does-it-all interface.
 *
 *	@ingroup network
 */
class BinaryIStream
{
public:

	BinaryIStream() { error_ = false; }
	virtual ~BinaryIStream() {};

	/**
	 *	This method retrieves the input number of bytes from this stream.
	 *	The returned data is still owned by the stream, so the caller
	 *	is not responsible for freeing it.
	 *
	 *	@param nBytes	Number of bytes to retrieve.
	 *
	 *	@return Pointer to returned data.
	 */
	virtual const void * retrieve( int nBytes ) = 0;
	virtual int remainingLength() const = 0;
	virtual void finish() {}

	virtual char peek() = 0;

	template <class TYPE>
	void extractRaw( TYPE &t );

	inline int readPackedInt();
	int readStringLength()	{ return this->readPackedInt(); }

	bool error() const { return error_; }
	void error( bool error ) { error_ = error; }

	BW::string remainingBytesAsDebugString( int maxBytes = 16,
		bool shouldConsume = true );

	// The buffer returned from retrieve() when someone reads past the end of a
	// stream.
	CSTDMF_DLL static char errBuf[ BS_BUFF_MAX_SIZE ];

protected:
	bool error_;
};


/**
 *	This convenience method is used to transfer data from a BinaryIStream to
 *	this BinaryOStream.
 */
inline void BinaryOStream::transfer( BinaryIStream & from, int length )
{
	this->addBlob( from.retrieve( length ), length );
}


/**
 *	This convenience method is used to add a block of memory to this stream.
 */
inline void BinaryOStream::addBlob( const void * pBlob, int size )
{
	std::memcpy( this->reserve( size ), pBlob, size );
}


#if !defined( BW_ENFORCE_ALIGNED_ACCESS )

/**
 *  Insert something into the stream in raw byte order, bypassing the normal
 *  platform-specific endian conversions.
 */
template <class TYPE>
inline void BinaryOStream::insertRaw( const TYPE & t )
{
	*((TYPE*)this->reserve( sizeof( TYPE ) )) = t;
}


/**
 *  Inverse operation of insertRaw() above, i.e. extract some data from the
 *  stream, bypassing endian conversions.
 */
template <class TYPE>
inline void BinaryIStream::extractRaw( TYPE & t )
{
	t = *((TYPE*)this->retrieve( sizeof( TYPE ) ));
}

#else // !defined( BW_ENFORCE_ALIGNED_ACCESS )

/**
 *  Insert something into the stream in raw byte order, bypassing the normal
 *  platform-specific endian conversions.
 */
template <class TYPE>
inline void BinaryOStream::insertRaw( const TYPE & t )
{
	std::memcpy( this->reserve( sizeof( TYPE ) ), &t, sizeof( TYPE ) );
}


/**
 *  Inverse operation of insertRaw() above, i.e. extract some data from the
 *  stream, bypassing endian conversions.
 */
template <class TYPE>
inline void BinaryIStream::extractRaw( TYPE & t )
{
	std::memcpy( &t, this->retrieve( sizeof( TYPE ) ), sizeof( TYPE ) );
}

#endif // !defined( BW_ENFORCE_ALIGNED_ACCESS )



// This ipp provides all the platform-specific type streaming operations.
#include "binary_stream.ipp"

// -----------------------------------------------------------------------------
// Section: operators
// -----------------------------------------------------------------------------

/**
 *	This method provides output streaming for a string.
 *
 *	@param b	The binary stream.
 *	@param str	The string to be streamed.
 *
 *	@return A reference to the stream that was passed in.
 */
inline BinaryOStream & operator<<( BinaryOStream & b, const BW::string & str )
{
	b.appendString( str.data(), int(str.length()) );

	return b;
}

/**
 * 	This method provides output streaming for a char *
 *
 *	@param b	The binary stream.
 *	@param str	The string to be streamed.
 *
 *	@return A reference to the stream that was passed in.
 */
inline BinaryOStream & operator<<( BinaryOStream & b, char * str )
{
	b.appendString( str, int(strlen( str )) );

	return b;
}


/**
 * 	This method provides output streaming for a const char *
 *
 *	@param b	The binary stream.
 *	@param str	The string to be streamed.
 *
 *	@return A reference to the stream that was passed in.
 */
inline BinaryOStream & operator<<( BinaryOStream & b, const char * str )
{
	return (b << const_cast< char * >( str ) );
}


/**
 * 	This method provides output streaming for a vector.
 *
 *	@param b	The binary stream.
 *	@param data	The vector to be streamed.
 *
 *	@return A reference to the stream that was passed in.
 */
template <class T, class A>
inline BinaryOStream & operator<<( BinaryOStream & b,
		const BW::vector<T,A> & data)
{
	uint32 max = (uint32)data.size();
	b << max;
	for (uint32 i=0; i < max; i++)
		b << data[i];
	return b;
}

/**
 *	This method puts an integer into the stream in a packed format.
 *
 *	Note: The value cannot be larger than +/- 2^23.
 *
 *	@param	value The integer value.
 */
inline void BinaryOStream::writePackedInt( int value )
{
	if (value >= 255)
	{
		// As we can't represent the full 4 bytes, ensure that the value being
		// stored is within the 3 byte range.
		MF_ASSERT( value < (1<<24) );

		(*this) << (uint8)0xff;
		BW_PACK3( (char*)this->reserve( 3 ), value );
	}
	else
	{
		(*this) << (uint8)value;
	}
}


/**
 *	This static method returns the number of bytes this value will be when
 *	packed.
 */
inline int BinaryOStream::calculatePackedIntSize( int value )
{
	return (value >= 0xff) ? 4 : 1;
}


/**
 *	This method gets a packed integer from the stream.
 *
 *	@return	The unpacked integer.
 */
inline int BinaryIStream::readPackedInt()
{
	int	value = *(uint8*)this->retrieve( 1 );

	if (value == 0xFF)
	{	// line below is very slightly dodgy ('tho safe here)
		value = BW_UNPACK3( (const char*)this->retrieve( 3 ) );
		// ASSERT(LITTLE_ENDIAN)
	}

	return value;
}


/**
 * 	This method provides input streaming for a string.
 *
 *	@param b	The binary stream.
 *	@param str	The string to be streamed.
 *
 *	@return A reference to the stream that was passed in.
 */
inline BinaryIStream & operator>>( BinaryIStream & b, BW::string & str )
{
	int	length = b.readPackedInt();
	char *s = (char*)b.retrieve( length );

	if (s != NULL)
		str.assign( s, length );

	return b;
}


/**
 *	This method provides input streaming for a char *.
 *
 *	@param b	The binary stream.
 *	@param str	The string to be streamed.
 *
 *	@return A reference to the stream that was passed in.
 */
inline BinaryIStream & operator>>( BinaryIStream & b, char * str )
{
	int	length = b.readPackedInt();
	char *s = (char*)b.retrieve( length );

	if (s != NULL)
	{
		memcpy( str, s, length );
		str[ length ] ='\0';
	}

	return b;
}


/**
 *	This method provides input streaming for a vector.
 *
 *	@param b	The binary stream.
 *	@param data	The vector to be streamed.
 *
 *	@return A reference to the stream that was passed in.
 */
template <class T, class A>
inline BinaryIStream & operator>>( BinaryIStream & b, BW::vector<T,A> & data)
{
	T elt;
	uint32 max = 0;

	b >> max;

	data.clear();

	if (!b.error())
	{
		data.reserve( max );
		for (uint32 i=0; i < max && !b.error(); i++)
		{
			b >> elt;
			data.push_back( elt );
		}
	}

	return b;
}


/**
 *	This method appends the input string onto this stream.
 *
 *	@param data		A pointer to the character array to put on the stream.
 *	@param length	The length of the data pointed to by <i>data</i>.
 */
inline void BinaryOStream::appendString( const char * data, int length )
{
	/* Note decision taken to NOT inline this fn
		(reversed temporarily to get it to compile) */
	/* Why don't we null-terminate?
		1. Costs in c_str call (altho could be avoided)
		2. BW::strings can contain null characters
	*/
	this->writePackedInt( length );
	this->addBlob( data, length );
}


// the MSVC compiler requires this operator because it doesn't find the binarystream operator<< for vectors

#define TYPEDEF_STREAMING_VECTOR( obj, name )                                  \
typedef BW::vector< obj > name;                                               \
                                                                               \
inline BinaryOStream & operator<<( BinaryOStream & b, const name & data )      \
{                                                                              \
	unsigned int max = (unsigned int)data.size();                              \
	b << max;                                                                  \
	for (unsigned int i=0;i<max;i++) b << data[i];                             \
	return b;                                                                  \
}                                                                              \
inline BinaryIStream & operator>>( BinaryIStream & b, name & data)             \
{                                                                              \
	obj	     		elt;                                                       \
	unsigned int	max;                                                       \
                                                                               \
	b >> max;                                                                  \
                                                                               \
	data.clear();                                                              \
	data.reserve(max);                                                         \
	for (unsigned int i=0;i<max;i++)									\
	{																	\
		b >> elt;														\
		data.push_back(elt);											\
	}																	\
	return b;															\
}																		\



// TODO: Should have this on for debug only
#ifndef MF_USE_TOKENS
#define MF_USE_TOKENS
#endif

#ifdef MF_USE_TOKENS
/**
 * 	This macro performs stream validation. It adds a token to the stream that
 * 	that can later be checked to ensure the contents of the stream is valid.
 */
#define TOKEN_ADD( stream, TOKEN )											\
	stream << *(int*)TOKEN;

/**
 * 	This macro performs stream validation.
 *	It checks that the expected magic number is present,
 *	to ensure that the stream is in sync with what is expected.
 */
#define TOKEN_CHECK( stream, TOKEN )										\
{																			\
	int token;																\
	stream >> token;														\
																			\
	if (token != *(int*)TOKEN)												\
	{																		\
		ERROR_MSG( "%s(%d): TOKEN_CHECK( %s ) failed! %x != %x\n",			\
				__FILE__, __LINE__, TOKEN, token, *(int*)TOKEN );			\
	}																		\
}

#else
/**
 * 	This macro performs stream validation.
 * 	It adds a magic number to the stream that can
 * 	later be checked to ensure the contents of the stream is valid.
 * 	In a release build, this macro does nothing.
 */
#define TOKEN_ADD( stream )

/**
 * 	This macro performs stream validation.
 *	It checks that the expected magic number is present,
 *	to ensure that the stream is in sync with what is expected.
 * 	In a release build, this macro does nothing.
 */
#define TOKEN_CHECK( stream )
#endif

BW_END_NAMESPACE

#endif // BINARY_STREAM_H
