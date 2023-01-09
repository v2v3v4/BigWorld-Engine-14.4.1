#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include "stdmf.hpp"
#include "bw_string.hpp"
#include "bw_string_ref.hpp"
#include "guard.hpp"
#include "debug.hpp"

#include <iterator>


BW_BEGIN_NAMESPACE

/**
 * Determines the number of bytes specified by the given utf8 lead byte. If
 * it is not a valid lead byte, then 0 is returned. NULL is treated as a 1 byte
 * utf8 character (i.e. will return 1, so NULL terminators must be handled by
 * the caller).
 *
 * Supports utf8 encodings up to 4 bytes long.
 */
CSTDMF_DLL size_t utf8ParseLeadByte( char c );


/**
 * Determines if the given character is a utf8 continuation character.
 */
inline bool utf8IsContinuationByte( const char c )
{
	return (c & 0xc0) == 0x80;
}

/**
 * Number of utf8 characters in the given null terminated buffer.
 * Assumes given pointer is a valid lead byte.
 */
inline size_t utf8StringLength( const char* str )
{
	size_t len = 0;
	while (*str)
	{
		if (!utf8IsContinuationByte( *str ))
		{
			++len;
		}
		++str;
	}
	return len;
}


/*
 * Find next non-UTF8 character in the given '\0' ended string. Return NULL if
 * no non-UTF8 character is found. '\0' is treated as valid UTF8 character.
 */
const char* findNextNonUtf8Char( const char * str );


/*
 * Check if the given string is valid utf8 string or in other words only
 * contains valid utf8 characters
 */
CSTDMF_DLL bool isValidUtf8Str( const char * str );


/*
 * Convert the string which may contain non-utf8 characters to a string which
 * only contain valid UTF8 characters, by converting non-utf8 char to its hex
 * string format
 */
CSTDMF_DLL void toValidUtf8Str( const char * srcStr, BW::string & dstStr );


/**
 * String utility functions
 */
CSTDMF_DLL BW::string bw_format( const char * format, ... );

#ifdef _WIN32
/**
 * Converts from ACP (active code page) to UTF8, which is good for converting
 *  results from ANSI versions of win32 API to ResMgr stuff.
 */
CSTDMF_DLL bool bw_acptoutf8( const char * s, BW::string& output );
inline bool bw_acptoutf8( const BW::string & s, BW::string& output )
{
	return bw_acptoutf8( s.c_str(), output );
}
inline BW::string bw_acptoutf8( const BW::string & s )
{
	BW::string ret;
	bw_acptoutf8( s, ret );
	return ret;
}


/**
 * Converts from UTF8 to ACP (active code page), which is good for converting
 *  results from ResMgr to ANSI versions of win32 API.
 */
CSTDMF_DLL bool bw_utf8toacp( const char * s, BW::string& output );
inline bool bw_utf8toacp( const BW::string & s, BW::string& output )
{
	return bw_utf8toacp( s.c_str(), output );
}


/**
 *
 */
inline BW::string bw_utf8toacp( const BW::string & s )
{
	BW::string ret;
	bw_utf8toacp( s, ret );
	return ret;
}


/**
 *
 *	Converts the given narrow string to the wide representation, using the 
 *  active code page on this system. Returns true if it succeeded, otherwise
 *	false if there was a decoding error.
 *
 */
CSTDMF_DLL bool bw_acptow( const char * s, BW::wstring& output );
inline bool bw_acptow( const BW::string & s, BW::wstring& output )
{
	return bw_acptow( s.c_str(), output );
}
inline BW::wstring bw_acptow( const BW::string & s )
{
	BW::wstring ret;
	bw_acptow( s, ret );
	return ret;
}


/**
 *
 *	Converts the given wide string to the narrow representation, using the 
 *  active code page on this system. Returns true if it succeeded, otherwise
 *	false if there was a decoding error.
 *
 */
CSTDMF_DLL bool bw_wtoacp( const wchar_t * s, BW::string& output );
inline bool bw_wtoacp( const BW::wstring & s, BW::string& output )
{
	return bw_wtoacp( s.c_str(), output );
}
inline BW::string bw_wtoacp( const BW::wstring & s )
{
	BW::string ret;
	bw_wtoacp( s, ret );
	return ret;
}


#endif

/**
 *
 *	Converts the given utf-8 string to the wide representation. Returns true 
 *  if it succeeded, otherwise false if there was a decoding error.
 *
 */
CSTDMF_DLL bool bw_utf8tow( const char * s, BW::wstring& output );
inline bool bw_utf8tow( const BW::string & s, BW::wstring& output )
{
	return bw_utf8tow( s.c_str(), output );
}
inline bool bw_utf8towSW( const BW::string & s, BW::wstring& output )
{
	return bw_utf8tow( s.c_str(), output );
}
inline BW::wstring bw_utf8tow( const BW::string & s )
{
	BW::wstring ret;
	bw_utf8tow( s, ret );
	return ret;
}


/**
 *
 *	Converts the given wide string to the utf-8 representation. Returns true 
 *  if it succeeded, otherwise false if there was a decoding error.
 *
 */
CSTDMF_DLL bool bw_wtoutf8( const wchar_t * s, BW::string& output );
inline bool bw_wtoutf8( const BW::wstring & s, BW::string& output )
{
	return bw_wtoutf8( s.c_str(), output );
}
inline bool bw_wtoutf8WS( const BW::wstring & s, BW::string& output )
{
	return bw_wtoutf8( s.c_str(), output );
}
inline BW::string bw_wtoutf8( const BW::wstring & s )
{
	BW::string ret;
	bw_wtoutf8( s, ret );
	return ret;
}


/**
 * This function will convert from UTF-8 to wide char in-place, it won't 
 * allocate any new buffers. At most, it will write dSize-1 chars plus a
 * trailing \\0. If total conversion is not possible, it will return false
 * 
 */
CSTDMF_DLL bool bw_utf8tow( const char * src, size_t srcSize, wchar_t * dst, size_t dSize );
inline bool bw_utf8tow( const BW::string & src, wchar_t * dst, size_t dSize )
{
	return bw_utf8tow( src.c_str(), src.length()+1, dst, dSize );
}

/**
 * This function will convert from wide char to UTF-8 in-place, it won't 
 * allocate any new buffers. At most, it will write dSize-1 chars plus a
 * trailing \\0. If total conversion is not possible, it will return false
 * 
 */
CSTDMF_DLL bool bw_wtoutf8( const wchar_t * src, size_t srcSize, char * dst, size_t dstSize );
inline bool bw_wtoutf8( const BW::wstring & src, char * dst, size_t dstSize )
{
	return bw_wtoutf8( src.c_str(), src.length()+1, dst, dstSize );
}


/**
 * This converts the given utf8 character at src, and places it into
 * the given wide character pointer. Returns the number of bytes that
 * made up this utf8 character.
 * 
 */
CSTDMF_DLL size_t bw_utf8tow_incremental( const char* src, wchar_t* dest );


/**
 * This function converts a null-terminated ansi c-string into a wide char 
 *  string. It does this just by promoting char to wchar_t. Always succeeds.
 */
CSTDMF_DLL void bw_ansitow( const char * src, BW::wstring & dst );
inline void bw_ansitow( const BW::string & src, BW::wstring & dst )
{
	return bw_ansitow( src.c_str(), dst );
}

/**
 * These functions do naive comparison between narrow & wide strings. They 
 * assume that the wide string will only really contain ASCII characters,
 * but if it doesn't, the comparison will still work.
 */
CSTDMF_DLL int bw_MW_strcmp ( const wchar_t * lhs, const char * rhs );
CSTDMF_DLL int bw_MW_strcmp ( const char * lhs, const wchar_t * rhs );
CSTDMF_DLL int bw_MW_stricmp ( const wchar_t * lhs, const char * rhs );
CSTDMF_DLL int bw_MW_stricmp ( const char * lhs, const wchar_t * rhs );

/**
 * This function takes a string source (or wstring or whatever), and a string of
 *  delimiters. It will tokenise the string according to the list of delimiters
 *  and append each found token into the output container. It will tokenise at
 *  most a_MaxTokens tokens, unless it is 0, then it will consume all the input
 *  string.  Note that this function does NOT clear the container, making it
 *  possible to append to a container that already contains elements.
 */
template< typename T1, typename T2, typename T3 >
void bw_tokenise( const T1 & a_Str,
				  const T2 & a_Delimiters, 
				  T3 & o_Tokens,
				  size_t a_MaxTokens = 0 )
{
	typename T1::size_type lastPos = a_Str.find_first_not_of( a_Delimiters, 0 );
	typename T1::size_type pos = a_Str.find_first_of( a_Delimiters, lastPos );

	while ((T1::npos != pos) || (T1::npos != lastPos))
	{
		if ((a_MaxTokens > 0) && (o_Tokens.size() == (a_MaxTokens - 1)))
		{
			std::inserter( o_Tokens, o_Tokens.end() ) = a_Str.substr( lastPos );
			return;
		}

		typename T1::size_type sublen = ((pos == T1::npos) ?
				a_Str.length() : pos) -
			lastPos;

		std::inserter( o_Tokens, o_Tokens.end() ) =
			a_Str.substr( lastPos, sublen );

		lastPos = a_Str.find_first_not_of( a_Delimiters, pos );
		pos = a_Str.find_first_of( a_Delimiters, lastPos );
	}
}


/**
 * This function joins together a container of strings using the specified
 * separator.
 */
template< typename T1, typename T2, typename T3 >
void bw_stringify( const T1 & a_Tokens, 
				   const T2 & a_Sep,
				   T3 & o_Str )
{
	for ( typename T1::const_iterator it = a_Tokens.begin(); it != a_Tokens.end(); ++it )
	{
		if ( o_Str.length() > 0 )
			o_Str += a_Sep;
		o_Str += (*it);
	}
}


template< typename T1, typename T2, class F >
void bw_containerConversion( const T1 & src, T2 & dst, F functor )
{
	for ( typename T1::const_iterator it = src.begin() ; it != src.end() ; ++it )
	{
		const typename T1::value_type & srcItem = *it;
		typename T2::value_type dstItem;
		functor( srcItem, dstItem );
		dst.push_back( dstItem );
	}
}

void bw_searchAndReplace( BW::string & subject, const char * search,
	const char * replace );

/**
 * This generates names in the form root 0, root 1...
 */

class IncrementalNameGenerator
{
public:
	IncrementalNameGenerator( const BW::string& rootName );
	~IncrementalNameGenerator() {};

	BW::string nextName();

private:
	BW::string rootName_;
	int			nameCount_;
};

inline IncrementalNameGenerator::IncrementalNameGenerator( const BW::string& rootName )
:	rootName_( rootName ),
	nameCount_( 0 )
{
}

inline BW::string IncrementalNameGenerator::nextName()
{
	char name[80];
	bw_snprintf( name, sizeof(name), "%s %d", rootName_.c_str(), nameCount_++ );
	return BW::string( name );
}

CSTDMF_DLL size_t bw_str_len( const char* str );
CSTDMF_DLL size_t bw_str_len( const wchar_t* str );

int bw_hextodec( char c );
int bw_whextodec( wchar_t c );

/**
 *	This method return if the first len length of
 *	2 strings are equal.
 *	@param len	the length of the string to be
 *				compared, it doesn't have to be
 *				the length of the whole string.
 */
template< typename T >
bool bw_str_equal( const T* str1, const T* str2, size_t len )
{
	BW_GUARD;
	size_t i = 0;
	const T* p1 = str1;
	const T* p2 = str2;
	while( i++ < len )
	{
		if (*p1++ != *p2++)
		{
			return false;
		}
	}
	return true;
}


/**
 *	This method search a string from left to right.
 *	@param str			null-terminated string to be searched.
 *	@param search		null-terminated string to look for
 *	@param retPos		return index of found string.
 *	@return				true if found.
 */
template< typename T >
bool bw_str_find_first_of( const T* str, const T* search, size_t& retPos )
{
	BW_GUARD;
	size_t lenSearch = bw_str_len( search );
	const T* pEnd = str + bw_str_len( str ) - lenSearch;
	const T* pStart = str;
	if (pStart > pEnd)
	{
		return false;
	}
	const T* p = pStart;
	while (p <= pEnd)
	{
		if (bw_str_equal( p, search, lenSearch ))
		{
			retPos = p - pStart;
			return true;
		}
		++p;
	}
	return false;
}


/**
 *	This method search a string from left to right.
 *	@param str			null-terminated string to be searched.
 *	@param search		a char to look for
 *	@param retPos		return index of found string.
 *	@return				true if found.
 */
template< typename T >
bool bw_str_find_first_of( const T* str, const T search, size_t& retPos )
{
	BW_GUARD;
	size_t lenSearch = 1;
	const T* pEnd = str + bw_str_len( str ) - lenSearch;
	const T* pStart = str;
	if (pStart > pEnd)
	{
		return false;
	}
	const T* p = pStart;
	while (p <= pEnd)
	{
		if (bw_str_equal( p, &search, lenSearch ))
		{
			retPos = p - pStart;
			return true;
		}
		++p;
	}
	return false;
}


/**
 *	This method search a string from right to left.
 *	@param str			null-terminated string to be searched.
 *	@param search		null-termanated string to look for
 *	@param retPos		return index of found string.
 *	@return				true if found.
 */
template< typename T >
bool bw_str_find_last_of( const T* str, const T* search, size_t& retPos )
{
	BW_GUARD;
	size_t lenSearch = bw_str_len( search );
	const T* pEnd = str + bw_str_len( str ) - lenSearch;
	const T* pStart = str;
	if (pStart > pEnd)
	{
		return false;
	}
	const T* p = pEnd;
	while (p >= pStart)
	{
		if (bw_str_equal( p, search, lenSearch ))
		{
			retPos = p - pStart;
			return true;
		}
		--p;
	}
	return false;
}


/**
 *	This method search a string from right to left.
 *	@param str			null-terminated string to be searched.
 *	@param search		a character to look for
 *	@param retPos		return index of found string.
 *	@return				true if found.
 */
template< typename T >
bool bw_str_find_last_of( const T* str, T search, size_t& retPos )
{
	BW_GUARD;
	size_t lenSearch = 1;
	const T* pEnd = str + bw_str_len( str ) - lenSearch;
	const T* pStart = str;
	if (pStart > pEnd)
	{
		return false;
	}
	const T* p = pEnd;
	while (p >= pStart)
	{
		if (bw_str_equal( p, &search, lenSearch ))
		{
			retPos = p - pStart;
			return true;
		}
		--p;
	}
	return false;
}



/**
 *	Append string to a string. If there is insufficient space the result is
 *	truncated.
 *	@param dst		pointer to destination buffer
 *	@param dstSize	size of destination buffer
 *	@param src		pointer to source buffer
 *	@param count	number of characters to append from source
 *	@returns		pointer to the destination buffer
 */
template< typename CharT >
CharT * bw_str_append( CharT * dst, size_t dstSize, const CharT * src,
	std::size_t count )
{
	BW_GUARD;
	MF_ASSERT( dst && dstSize );
	const size_t dstLen = bw_str_len( dst );
	if (dstLen == dstSize)
	{
		// no space remaining
		return dst;
	}
	const std::size_t size = std::min( dstSize - dstLen - 1, count );
	CharT * dstOffset = dst + dstLen;
	std::copy( src, src + size, 
#if _ITERATOR_DEBUG_LEVEL > 0
		stdext::checked_array_iterator< CharT * >( dstOffset, dstSize - dstLen )
#else
		dstOffset
#endif
		);
	dstOffset[size] = 0;
	return dst;
}


template< typename CharT >
CharT * bw_str_append( CharT * dst, std::size_t dstSize, const CharT * appendStr )
{
	return bw_str_append( dst, dstSize, appendStr, bw_str_len( appendStr ) );
}


template < typename CharT, typename Traits >
CharT * bw_str_append( CharT * dst, std::size_t dstSize,
	const BW::BasicStringRef< CharT, Traits > & src )
{
	return bw_str_append( dst, dstSize, src.data(), src.size() );
}


template< typename CharT, typename Traits, typename Alloc >
CharT * bw_str_append( CharT * dst, std::size_t dstSize,
					const std::basic_string< CharT, Traits, Alloc > & src )
{
	return bw_str_append( dst, dstSize, src.data(), src.size() );
}

/**
 *	Returns a substring [pos, pos+count). 
 *	If the requested substring extends past the end of the string, 
 *	the returned substring is [pos, dstSize]
 *	@param dst		pointer to destination buffer
 *	@param dstSize	size of destination buffer
 *	@param src		pointer to source buffer
 *	@param pos		position of the first character to include 
 *	@param count	length of the substring
 *	@returns		pointer to the destination buffer
 */
template< typename CharT >
CharT * bw_str_substr( CharT * dst, std::size_t dstSize, const CharT * src,
	std::size_t pos, std::size_t count )
{
	BW_GUARD;
	MF_ASSERT( dst && dstSize );
	const CharT * srcOffset = src + pos;
	count = std::min( count, dstSize - 1 );
	std::copy( srcOffset, srcOffset + count,
#if _ITERATOR_DEBUG_LEVEL > 0
		stdext::checked_array_iterator< CharT * >( dst, dstSize )
#else
		dst
#endif
		);
	dst[count] = 0;
	return dst;
}

/**
 *	Copies source string to the dest buffer.
 *	@param dst		pointer to destination buffer
 *	@param dstSize	size of destination buffer
 *	@param src		pointer to source buffer
 *	@param count	number of characters to copy
 *	@returns		pointer to destination buffer
 */
template < typename CharT >
CharT * bw_str_copy( CharT * dst, std::size_t dstSize,
	const CharT * src, std::size_t count )
{
	return bw_str_substr( dst, dstSize, src, 0, count );
}


template < typename CharT >
CharT * bw_str_copy( CharT * dst, std::size_t dstSize, const CharT * src )
{
	return bw_str_copy( dst, dstSize, src, bw_str_len( src ) );
}


template < typename CharT, typename Traits >
CharT * bw_str_copy( CharT * dst, std::size_t dstSize,
	const BW::BasicStringRef< CharT, Traits > & src )
{
	return bw_str_copy( dst, dstSize, src.data(), src.size() );
}


template< typename CharT, typename Traits, typename Alloc >
CharT * bw_str_copy( CharT * dst, std::size_t dstSize,
	const std::basic_string< CharT, Traits, Alloc > & src )
{
	return bw_str_copy( dst, dstSize, src.data(), src.size() );
}

BW_END_NAMESPACE

#endif // STRING_UTILS_HPP
