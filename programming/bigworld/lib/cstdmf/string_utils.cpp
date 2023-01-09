#include "pch.hpp"
#include "string_utils.hpp"

#include "stdmf.hpp"
#include "smartpointer.hpp"

#include <stdarg.h>

BW_BEGIN_NAMESPACE


/**
 * A look up table for converting byte to its hex string representation. This
 * only contains the bytes between [128, 255]. To get the string of a byte
 * between this range from this table, you need to subtract the byte by 128
 * and use the remaining as the index to get the corresponding string.
 */
static const char * byteToString[128] = {
		"0x80", "0x81", "0x82", "0x83", "0x84", "0x85", "0x86", "0x87",
		"0x88",	"0x89", "0x8A", "0x8B", "0x8C", "0x8D", "0x8E", "0x8F",
		"0x90", "0x91", "0x92", "0x93", "0x94", "0x95", "0x96", "0x97",
		"0x98",	"0x99", "0x9A", "0x9B", "0x9C", "0x9D", "0x9E", "0x9F",
		"0xA0", "0xA1", "0xA2", "0xA3", "0xA4", "0xA5", "0xA6", "0xA7",
		"0xA8",	"0xA9", "0xAA", "0xAB", "0xAC", "0xAD", "0xAE", "0xAF",
		"0xB0", "0xB1", "0xB2", "0xB3", "0xB4", "0xB5", "0xB6", "0xB7",
		"0xB8",	"0xB9", "0xBA", "0xBB", "0xBC", "0xBD", "0xBE", "0xBF",
		"0xC0", "0xC1", "0xC2", "0xC3", "0xC4", "0xC5", "0xC6", "0xC7",
		"0xC8",	"0xC9", "0xCA", "0xCB", "0xCC", "0xCD", "0xCE", "0xCF",
		"0xD0", "0xD1", "0xD2", "0xD3", "0xD4", "0xD5", "0xD6", "0xD7",
		"0xD8",	"0xD9", "0xDA", "0xDB", "0xDC", "0xDD", "0xDE", "0xDF",
		"0xE0", "0xE1", "0xE2", "0xE3", "0xE4", "0xE5", "0xE6", "0xE7",
		"0xE8",	"0xE9", "0xEA", "0xEB", "0xEC", "0xED", "0xEE", "0xEF",
		"0xF0", "0xF1", "0xF2", "0xF3", "0xF4", "0xF5", "0xF6", "0xF7",
		"0xF8",	"0xF9", "0xFA", "0xFB", "0xFC", "0xFD", "0xFE", "0xFF"
};


//------------------------------------------------------------------------------
int bw_hextodec( char c )
{
	if (c >= '0' && c <= '9')
	{
		return (int)(c - L'0');
	}
	else if (c >= 'A' && c <= 'F')
	{
		return 10 + (int)(c - 'A');
	}
	else if (c >= 'a' && c <= 'f')
	{
		return 10 + (int)(c - 'a');
	}

	return -1;
}

//------------------------------------------------------------------------------
int bw_whextodec( wchar_t c )
{
	if (c >= L'0' && c <= L'9')
	{
		return (int)(c - L'0');
	}
	else if (c >= L'A' && c <= L'F')
	{
		return 10 + (int)(c - L'A');
	}
	else if (c >= L'a' && c <= L'f')
	{
		return 10 + (int)(c - L'a');
	}

	return -1;
}

//------------------------------------------------------------------------------
size_t utf8ParseLeadByte( char c )
{
	// 1-byte:  0xxxxxxx (0x00)
	// 2-bytes: 110xxxxx (0xc0)
	// 3-bytes: 1110xxxx (0xe0)
	// 4-bytes: 11110xxx (0xf0)
	// continuation bytes: 10xxxxxx (0x80)
	
	// unrolled for explicitness
	if ((c & 0x80) == 0)
	{
		return 1;
	}
	else if ((c & 0xe0) == 0xc0)
	{
		return 2;
	}
	else if ((c & 0xf0) == 0xe0)
	{
		return 3;
	}
	else if ((c & 0xf8) == 0xf0)
	{
		return 4;
	}

	// must be continuation byte
	return 0; 
}

//------------------------------------------------------------------------------
const char* findNextNonUtf8Char( const char * str )
{
	const char * pos = str;
	size_t byteCount = 0;
	size_t i = 1;

	while (*pos != '\0')
	{
		byteCount = utf8ParseLeadByte( *pos );

		if (byteCount == 0)
		{
			// found invalid leading byte
			return pos;
		}
		else if (byteCount > 1)
		{
			// found multiple bytes sequence
			i = 1;
			while (i < byteCount)
			{
				if (!utf8IsContinuationByte( *( pos + i ) ))
				{
					return pos;
				}

				++i;
			}
		}

		pos += byteCount;
	}

	return NULL;
}


//------------------------------------------------------------------------------
bool isValidUtf8Str( const char * str )
{
	return ( findNextNonUtf8Char( str ) == NULL );
}


//------------------------------------------------------------------------------
void toValidUtf8Str( const char * srcStr, BW::string & dstStr )
{
	if (srcStr == NULL || *srcStr == '\0')
	{
		return;
	}

	const char * currPos = srcStr;
	const char * nonUtf8Pos;

	while (( nonUtf8Pos = findNextNonUtf8Char( currPos ) ) != NULL)
	{
		if (currPos != nonUtf8Pos)
		{
			// copy valid UTF8 characters
			dstStr.append( currPos, nonUtf8Pos );
		}

		unsigned char value = ((unsigned char)*nonUtf8Pos) ;

		MF_ASSERT( value >= 128 );

		unsigned char index = value - 128;

		dstStr.append( byteToString[ index ] );

		// move currPos to after nonUtf8Pos
		currPos = nonUtf8Pos + 1;
	}

	// copy remaining valid UTF8 characters, maybe only '\0' left
	dstStr.append( currPos );
}


//------------------------------------------------------------------------------
BW::string bw_format( const char * format, ... )
{
	char buffer[ BUFSIZ << 2 ];

	va_list argPtr;
	va_start( argPtr, format );
	bw_vsnprintf( buffer, sizeof(buffer), format, argPtr );
	va_end( argPtr );

	return BW::string( buffer );
}

#ifdef _WIN32

//------------------------------------------------------------------------------
size_t bw_utf8tow_incremental( const char* src, wchar_t* dest )
{
	if (!src[0])
	{
		return 0;
	}

	size_t numBytes = utf8ParseLeadByte( src[0] );
	if (!MultiByteToWideChar( CP_UTF8, 0, src, 
			static_cast<int>(numBytes), dest, 1 ))
	{
		return 0;
	}

	return numBytes;
}

//------------------------------------------------------------------------------
bool bw_acptoutf8( const char * src, BW::string& output )
{
	int len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
	if (len <= 0)
	{
		return false;
	}
	else
	{
		// MultiByteToWideChar will process the \0 at the end of the source,
		//  so len will contain that too, meaning that the output size WILL
		//  include the \0 at the end, which breaks string concatenation,
		//  since they will be put AFTER the \0. So, after processing the string
		//  we remove the \0 of the output.

		BW::wstring temp;
		temp.resize( len );
		int res = MultiByteToWideChar( CP_ACP, 0, src, -1, &temp[0], len );
		temp.resize( temp.length() - 1 );
		return bw_wtoutf8( temp, output );
	}
}


//------------------------------------------------------------------------------
bool bw_utf8toacp( const char * src, BW::string& output )
{
	int len = MultiByteToWideChar( CP_UTF8, 0, src, -1, NULL, 0 );
	if (len <= 0)
	{
		return false;
	}
	else
	{
		// MultiByteToWideChar will process the \0 at the end of the source,
		//  so len will contain that too, meaning that the output size WILL
		//  include the \0 at the end, which breaks string concatenation,
		//  since they will be put AFTER the \0. So, after processing the string
		//  we remove the \0 of the output.

		BW::wstring temp;
		temp.resize( len );
		int res = MultiByteToWideChar( CP_UTF8, 0, src, -1, &temp[0], len );
		temp.resize( temp.length() - 1 );
		return bw_wtoacp( temp, output );
	}
}


//------------------------------------------------------------------------------
bool bw_acptow( const char * src, BW::wstring& output )
{
	int len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
	if (len <= 0)
	{
		return false;
	}
	else
	{
		// MultiByteToWideChar will process the \0 at the end of the source,
		//  so len will contain that too, meaning that the output size WILL
		//  include the \0 at the end, which breaks string concatenation,
		//  since they will be put AFTER the \0. So, after processing the string
		//  we remove the \0 of the output.

		output.resize( len );
		int res = MultiByteToWideChar( CP_ACP, 0, src, -1, &output[0], len );
		output.resize( output.length() - 1 );
		return res != 0;
	}
}

//------------------------------------------------------------------------------
bool bw_wtoacp( const wchar_t * wsrc, BW::string& output )
{
	int len = WideCharToMultiByte( CP_ACP, 0, wsrc, -1, NULL, 0, NULL, NULL );
	if (len <= 0)
	{
		return false;
	}
	else
	{
		output.resize( len );
		int res = WideCharToMultiByte( CP_ACP, 0, wsrc, -1, &output[0], len, NULL, NULL );
		output.resize( output.length() - 1 );
		return res != 0;
	}
}

//------------------------------------------------------------------------------
bool bw_utf8tow( const char * src, BW::wstring& output )
{
	int len = MultiByteToWideChar( CP_UTF8, 0, src, -1, NULL, 0 );
	if (len <= 0)
	{
		return false;
	}
	else
	{
		output.resize( len );
		int res = MultiByteToWideChar( CP_UTF8, 0, src, -1, &output[0], len );
		output.resize( output.length() - 1 );
		return res != 0;
	}
}

//------------------------------------------------------------------------------
bool bw_wtoutf8( const wchar_t * wsrc, BW::string& output )
{
	int len = WideCharToMultiByte( CP_UTF8, 0, wsrc, -1, NULL, 0, NULL, NULL );
	if (len <= 0)
	{
		return false;
	}
	else
	{
		output.resize( len );
		int res = WideCharToMultiByte( CP_UTF8, 0, wsrc, -1, &output[0], len, NULL, NULL );
		output.resize( output.length() - 1 );
		return res != 0;
	}
}

//------------------------------------------------------------------------------
bool bw_utf8tow( const char * src, size_t srcSize, wchar_t * dst, size_t dstSize )
{
	if (dstSize)
	{
		MF_ASSERT( srcSize <= INT_MAX );
		MF_ASSERT( dstSize <= INT_MAX );
		int res = MultiByteToWideChar(
			CP_UTF8, 0, src, ( int ) srcSize, dst, ( int ) dstSize );

		if (res >= 0)
		{
			dst[ res ] = L'\0';

			return true;
		}
	}

	return false;
}


//------------------------------------------------------------------------------
bool bw_wtoutf8( const wchar_t * src, size_t srcSize, char * dst, size_t dstSize )
{
	if (dstSize)
	{
		MF_ASSERT( srcSize <= INT_MAX );
		MF_ASSERT( dstSize <= INT_MAX );
		int res = WideCharToMultiByte(
			CP_UTF8, 0, src, ( int ) srcSize, dst, ( int ) dstSize, NULL, NULL );

		if (res >= 0)
		{
			dst[ res ] = '\0';

			return true;
		}
	}

	return false;
}

#else // ! _WIN32


BW_END_NAMESPACE
#include <iconv.h>
#include <errno.h>
#include <string.h>
BW_BEGIN_NAMESPACE


class IConv;
typedef SmartPointer< IConv > IConvPtr;


/**
 *	Wrapper class around the glibc iconv() family of functions.
 *
 *	Ensures that iconv_t handles are released when objects of this class go out
 *	of scope.
 */
class IConv : public ReferenceCount
{
public:
	~IConv() { iconv_close( iConvDesc_ ); }

	/**
	 *	Wrapper around the iconv_open() function.
	 */
	static IConvPtr create( const char * toEncoding, const char * fromEncoding );


	/**
	 *	Wrapper around the iconv() function.
	 */
	size_t convert( const char ** inBytes, size_t * inBytesLeft,
			char ** outBytes, size_t * outBytesLeft )
	{
		// glibc iconv doesn't support const char ** type for inbytes parameter.
		return iconv( iConvDesc_, const_cast< char ** >( inBytes ), inBytesLeft, 
			outBytes, outBytesLeft );
	}
	
private:
	IConv( iconv_t iConvDesc ):
		ReferenceCount(),
		iConvDesc_( iConvDesc )
	{}

private:
	iconv_t iConvDesc_;
};

IConvPtr IConv::create( const char * toEncoding, const char * fromEncoding )
{
	iconv_t iconvDesc = iconv_open( toEncoding, fromEncoding );
	if (iconvDesc == iconv_t( -1 ))
	{
		return NULL;
	}
	return new IConv( iconvDesc );
}


/**
 *	Convert a UTF8 string to a STL wide character string.
 *
 *	@param src 		The source UTF8 string buffer.
 *	@param output 	The output string.
 *
 *	@return 		Whether the conversion succeeded or not.
 */
bool bw_utf8tow( const char * src, BW::wstring& output )
{
	size_t bufSize = BUFSIZ;
	bool finished = false;
	bool success = false;

	IConvPtr pIConv = IConv::create( "WCHAR_T", "UTF-8" );
	MF_ASSERT( pIConv.get() != NULL );

	size_t srcLen = strlen( src ) + 1;
	char * buf = NULL;
	do
	{
		bw_safe_delete_array(buf);
		buf = new char[bufSize];

		const char * inBuf = src;
		size_t inBytesLeft = srcLen;
		char * outBuf = buf;
		size_t outBytesLeft = bufSize;

		size_t res = pIConv->convert( &inBuf, &inBytesLeft, 
			&outBuf, &outBytesLeft );

		if (res == size_t( -1 ))
		{
			if (errno != E2BIG)
			{
				// Invalid conversion.
				finished = true;
				success = false;
			}

			// Not enough buffer to convert into, resize.
			bufSize *= 2;
		}
		else
		{
			// We're done!
			finished = true;
			success = true;
		}

	} while (!finished);

	if (success)
	{
		wchar_t * wbuf = reinterpret_cast< wchar_t * >( buf );
		output.assign( wbuf, wcslen( wbuf ) );
	}

	bw_safe_delete_array(buf);

	return success;
}

/**
 *	Convert a wide character string to a UTF8 string.
 *	
 *	@param wsrc 		The source wide character string.
 *	@param output 		A string to be filled with the corresponding UTF-8 string.
 *
 *	@return 			Whether the conversion succeeded.
 */
bool bw_wtoutf8( const wchar_t * wsrc, BW::string& output )
{
	size_t bufSize = BUFSIZ;
	bool finished = false;
	bool success = false;

	IConvPtr pIConv = IConv::create( "UTF-8", "WCHAR_T" );
	size_t srcLen = (wcslen( wsrc ) + 1) * sizeof( wchar_t );
	char * buf = NULL;
	
	do
	{
		bw_safe_delete_array(buf);
		buf = new char[bufSize];
		const char *inBuf = reinterpret_cast< const char * >( wsrc );
		size_t inBytesLeft = srcLen;
		char *outBuf = buf;
		size_t outBytesLeft = bufSize;
		
		size_t res = pIConv->convert( &inBuf, &inBytesLeft, 
				&outBuf, &outBytesLeft );
		if (res == size_t( -1 ))
		{
			if (errno != E2BIG)
			{
				// Invalid conversion.
				finished = true;
				success = false;
			}

			// Not enough buffer to convert into, resize.
			bufSize *= 2;
		}
		else
		{
			// We're done!
			finished = true;
			success = true;
		}

	} while (!finished);

	if (success)
	{
		output.assign( buf, strlen( buf ) );
	}

	bw_safe_delete_array(buf);

	return success;
}


/**
 *	Convert a wide character string to the corresponding UTF-8 string.
 *
 *	@param src 		The source wide character string.
 *	@param srcSize 	The size of the wide character string, in bytes.
 *	@param wDst 	The destination character buffer. This is filled with the
 *					UTF-8 string.
 *	@param dstSize 	The size of the destination character buffer, in bytes.
 */
bool bw_utf8tow( const char * src, size_t srcSize, wchar_t * wDst, 
		size_t dstSize )
{
	IConvPtr pIConv = IConv::create( "WCHAR_T", "UTF-8" );
	
	char * dst = reinterpret_cast< char * >( wDst );
	return pIConv->convert( &src, &srcSize, &dst, &dstSize ) != size_t( -1 );
}


/**
 *	Convert a wide character string to the corresponding UTF-8 string.
 *
 *	@param wSrc 	The source wide character string.
 *	@param srcSize 	The size of the wide character string, in bytes.
 *	@param dst 		The destination character buffer. This is filled with the
 *					UTF-8 string.
 *	@param dstSize 	The size of the destination character buffer, in bytes.
 */
bool bw_wtoutf8( const wchar_t * wSrc, size_t srcSize, char * dst, 
		size_t dstSize )
{
	IConvPtr pIConv = IConv::create( "UTF-8", "WCHAR_T" );

	const char * src = reinterpret_cast< const char * >( wSrc );
	return pIConv->convert( &src, &srcSize, &dst, &dstSize ) != size_t( -1 );
}
#endif


//------------------------------------------------------------------------------
void bw_ansitow( const char * src, BW::wstring & dst )
{
	size_t len = strlen( src );
	dst.resize( len );
	for ( size_t idx = 0 ; idx < len ; ++idx )
		dst[ idx ] = src[ idx ];
}


//------------------------------------------------------------------------------
int bw_MW_strcmp ( const wchar_t * lhs, const char * rhs )
{
	int ret = 0;

	while( ! (ret = (int)(*lhs - *rhs)) && *rhs)
	{
		++lhs, ++rhs;
	}

	if ( ret < 0 )
		ret = -1 ;
	else if ( ret > 0 )
		ret = 1 ;

	return( ret );
}

//------------------------------------------------------------------------------
int bw_MW_strcmp ( const char * lhs, const wchar_t * rhs )
{
	int ret = 0;

	while( ! (ret = (int)(*lhs - *rhs)) && *rhs)
	{
		++lhs, ++rhs;
	}

	if ( ret < 0 )
		ret = -1 ;
	else if ( ret > 0 )
		ret = 1 ;

	return( ret );
}

//-----------------------------------------------------------------------------
int bw_MW_stricmp ( const char * lhs, const wchar_t * rhs )
{
	int f, l;

	do
	{
		if ( ((f = (unsigned char)(*(lhs++))) >= 'A') && (f <= 'Z') )
			f -= 'A' - 'a';
		if ( ((l = (unsigned char)(*(rhs++))) >= 'A') && (l <= 'Z') )
			l -= 'A' - 'a';
	}
	while ( f && (f == l) );

	return(f - l);
}

//-----------------------------------------------------------------------------
int bw_MW_stricmp ( const wchar_t * lhs, const char * rhs )
{
	int f, l;

	do
	{
		if ( ((f = (unsigned char)(*(lhs++))) >= 'A') && (f <= 'Z') )
			f -= 'A' - 'a';
		if ( ((l = (unsigned char)(*(rhs++))) >= 'A') && (l <= 'Z') )
			l -= 'A' - 'a';
	}
	while ( f && (f == l) );

	return(f - l);
}


/**
 *	Search and replace an occurance of a string within another string.
 *
 *	@param subject 	The string to search and replace within.
 *	@param search 	The string to search for.
 *	@param replace	The string to replace the search value with.
 */
void bw_searchAndReplace( BW::string & subject, const char * search,
	const char * replace )
{
	BW::string::size_type pos = 0;
	while ((pos = subject.find( search, pos )) != 
		BW::string::npos)
	{
		subject.replace( pos, strlen( search ), replace );
		pos += strlen( replace );
	}
}

/**
 *	This method return the length of char string.
 */
size_t bw_str_len( const char* str )
{
	return strlen( str );
}

/**
 *	This method return the length of wchar_t string.
 */

size_t bw_str_len( const wchar_t* str )
{
	return wcslen( str );
}





BW_END_NAMESPACE

// string_utils.cpp
