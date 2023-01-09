/* stdmf.hpp: library wide definitions
 */
#ifndef STDMF_HPP
#define STDMF_HPP

#include <cctype>
#include <cstdarg>

#include "stdmf_minimal.hpp"

#include "bw_namespace.hpp"

#ifdef CODE_INLINE
    #define INLINE    inline
#else
	/// INLINE macro.
    #define INLINE
#endif


#if defined( _WIN32 )
	//Required for _commit function
	#include <io.h>
#else
	#include <stdio.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <string.h>
	#include <pwd.h>
#endif // _WIN32

#include <stdint.h>
#include <limits>
#include <math.h>

#if !defined( PLAYSTATION3 ) && !defined( __APPLE__ ) && !defined( __ANDROID__ )
	#include <malloc.h>
#endif


#if defined( __APPLE__ ) || defined( __ANDROID__ ) || defined( __EMSCRIPTEN__ )
	#define BW_ENFORCE_ALIGNED_ACCESS
#endif

/* --------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------
 */

//#define NT_ENABLED

#ifdef __WATCOMC__
#ifndef false
#define false		0
#define true		1
#endif
#endif // __WATCOMC__

/*
#define ASCII_NUL	0x00
#define ASCII_SOH	0x01
#define ASCII_STX	0x02
#define ASCII_ETX	0x03
#define ASCII_EOT	0x04
#define ASCII_ENQ	0x05
#define ASCII_ACK	0x06
#define ASCII_BEL	0x07
#define ASCII_BS	0x08
#define ASCII_HT	0x09
#define ASCII_LF	0x0a
#define ASCII_VT	0x0b
#define ASCII_FF	0x0c
#define ASCII_CR	0x0d
#define ASCII_SO	0x0e
#define ASCII_SI	0x0f
#define ASCII_DLE	0x10
#define ASCII_DC1	0x11
#define ASCII_DC2	0x12
#define ASCII_DC3	0x13
#define ASCII_DC4	0x14
#define ASCII_NAK	0x15
#define ASCII_SYN	0x16
#define ASCII_ETB	0x17
#define ASCII_CAN	0x18
#define ASCII_EM	0x19
#define ASCII_SUB	0x1a
#define ASCII_ESC	0x1b
#define ASCII_FS	0x1c
#define ASCII_GS	0x1d
#define ASCII_RS	0x1e
#define ASCII_US	0x1f
*/

# if defined( _WIN32 )
#define BW_NEW_LINE "\r\n"
# else//WIN32
#define BW_NEW_LINE "\n"
# endif//WIN32

#include "cstdmf/bw_namespace.hpp"

#if !defined( _WIN32 ) && !defined( PLAYSTATION3 )
// We also set this in script/first_include.hpp
// so it may already be defined.
#if !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS
#endif // defined( __STDC_FORMAT_MACROS )
#include <inttypes.h>
#endif // !defined( _WIN32 ) && !defined( PLAYSTATION3 )


BW_BEGIN_NAMESPACE

/* --------------------------------------------------------------------------
 * Types
 * --------------------------------------------------------------------------
 */

/// This type is an unsigned character.
typedef unsigned char	uchar;
/// This type is an unsigned short.
typedef unsigned short	ushort;
/// This type is an unsigned integer.
typedef unsigned int	uint;
/// This type is an unsigned longer.
typedef unsigned long	ulong;

#if defined( PLAYSTATION3 )

typedef int8_t			int8;
typedef int16_t			int16;
typedef int32_t			int32;
typedef int64_t			int64;
typedef uint8_t			uint8;
typedef uint16_t		uint16;
typedef uint32_t		uint32;
typedef uint64_t		uint64;

typedef intptr_t		intptr;
typedef uintptr_t		uintptr;

#define PRI64 "lld"
#define PRIu64 "llu"
#define PRIx64 "llx"
#define PRIX64 "llX"
#define PRIzu "lu"
#define PRIzd "ld"

#elif defined( _WIN32 )

typedef __int8				int8;
typedef unsigned __int8		uint8;

typedef __int16				int16;
typedef unsigned __int16	uint16;

typedef __int32				int32;
typedef unsigned __int32	uint32;

typedef __int64				int64;
typedef unsigned __int64	uint64;

#if !defined( _XBOX ) && !defined( _XBOX360 )
// This type is an integer with the size of a pointer.
typedef intptr_t			intptr;
// This type is an unsigned integer with the size of a pointer.
typedef uintptr_t        	uintptr;
#else
typedef int32				intptr;
typedef uint32				uintptr;
#endif
#define PRI64 "lld"
#define PRIu64 "llu"

#ifndef PRIx64
#define PRIx64 "llx"
#endif

#ifndef PRIX64
#define PRIX64 "llX"
#endif

#if defined( _WIN64 )
#define PRIzu "llu"
#define PRIzd "lld"
#else
#define PRIzu "u"
#define PRIzd "d"
#endif

#else // C99

/// This type is an integer with a size of 8 bits.
typedef int8_t				int8;
/// This type is an unsigned integer with a size of 8 bits.
typedef uint8_t				uint8;

/// This type is an integer with a size of 16 bits.
typedef int16_t				int16;
/// This type is an unsigned integer with a size of 16 bits.
typedef uint16_t			uint16;

/// This type is an integer with a size of 32 bits.
typedef int32_t				int32;
/// This type is an unsigned integer with a size of 32 bits.
typedef uint32_t			uint32;
/// This type is an integer with a size of 64 bits.
typedef int64_t				int64;
/// This type is an unsigned integer with a size of 64 bits.
typedef uint64_t			uint64;

/// This type is an integer that can hold a pointer.
typedef intptr_t			intptr;
/// This type is an unsignd integer that can hold a pointer.
typedef uintptr_t			uintptr;

#define PRI64 PRId64

#ifndef PRIzd
#define PRIzd "zd"
#endif

#ifndef PRIzu
#define PRIzu "zu"
#endif

#endif // if defined( PLAYSTATION3 )

/// This type is used for a 4 byte file header descriptor.
//typedef uint32			HdrID;		/* 4 byte (file) descriptor */
/// This type is used for a generic 4 byte descriptor.
//typedef uint32			ID;			/* 4 byte generic descriptor */

BW_END_NAMESPACE

/* --------------------------------------------------------------------------
 * Macros
 * --------------------------------------------------------------------------
 */

/**
 * Maximum path defines differ between windows and linux, so use BW_MAX_PATH
 */
#if defined(_WIN32)
#define BW_MAX_PATH MAX_PATH
#else
#include <limits.h>
#define BW_MAX_PATH PATH_MAX
#endif

/* compiler-specific priority
 */

#if defined(__GNUC__) || defined(__clang__)
#define BW_STATIC_INIT_PRIORITY(n) __attribute__ ((init_priority(n)))
#else
#define BW_STATIC_INIT_PRIORITY(n)
#endif

/* array & structure macros
 */
#if 0
#define ARRAYCLR(v)					memset((v), 0x0, sizeof(v))
#define MEMCLR(v)					memset(&(v), 0x0, sizeof(v))
#define MEMCLRP(v)					memset((v), 0x0, sizeof(*v))
#endif

/**
 *	Offset of a field in a class, relative to a static-cast to another class
 *
 *	This is mainly useful when external code needs the offset and will be
 *	dealing with a superclass pointer of the class containing the field.
 *	We use 256 here because GCC sees (TYPE*)(NULL)->MEMBER as being offsetof
 *	and rejects it as described in bw_offsetof.
 */
#define bw_offsetof_from( TYPE, MEMBER, SUPERTYPE ) (						\
	reinterpret_cast< char * >(												\
		&(reinterpret_cast< TYPE * >( 256 )->MEMBER) ) -						\
	reinterpret_cast< char * >(												\
		static_cast< SUPERTYPE * >( reinterpret_cast< TYPE * >( 256 ) ) ))

/**
 *	Replacement for offsetof that won't complain when given a non-POD type.
 *
 *	This is dangerous, if offsetof won't work, bw_offsetof might be wrong too.
 *	That's why the compiler complains, after all...
 *
 *	In C++11, offsetof can be safely used on standard-layout types, of which
 *	POD types are a subset.
 *	GCC 4.1.2 will compile offsetof on non-POD types with the
 *	-Wno-invalid-offsetof compiler flag enabled.
 */
#define bw_offsetof( TYPE, MEMBER ) bw_offsetof_from( TYPE, MEMBER, TYPE )

/**
 *	The TYPE* containing TYPE::MEMBER at MEMBERPTR
 */
#define bw_container_of( MEMBERPTR, TYPE, MEMBER ) (						  \
	reinterpret_cast< TYPE * >(												  \
		reinterpret_cast< char * >( MEMBERPTR ) - bw_offsetof( TYPE, MEMBER ) \
	))

/* string comparing
 */
/// Returns true if two strings are equal.
#define STR_EQ(s, t)		(!strcmp((s), (t)))
/// Returns true if two strings are the same, ignoring case.
#define STR_EQI(s, t)		(!bw_stricmp((s), (t)))
/// Returns true if the first sz byte of the input string are equal, ignoring
/// case.
#define STRN_EQI(s, t, sz)	(!bw_strnicmp((s), (t), (sz)))
/// Returns true if the first sz byte of the input string are equal, ignoring
/// case.
#define STRN_EQ(s, t, sz)	(!strncmp((s), (t), (sz)))
/// Returns true if all three input string are equal.
#define STR_EQ2(s, t1, t2)	(!(strcmp((s), (t1)) || strcmp((s), (t2))))


/**
 * This macro creates a long out of 4 chars, used for all id's: ID, HdrId.
 * @note byte ordering on 80?86 == 4321
 */
#if 0
#define MAKE_ID(a, b, c, d)	\
	(uint32)(((uint32)(d)<<24) | ((uint32)(c)<<16) | ((uint32)(b)<<8) | (uint32)(a))
#endif


/* Intel Architecture is little endian (low byte presented first)
 * Motorola Architecture is big endian (high byte first)
 */
/// The current architecture is Little Endian.
#define MF_LITTLE_ENDIAN
/*#define MF_BIG_ENDIAN*/

#ifdef MF_LITTLE_ENDIAN
/* accessing individual bytes (int8) and words (int16) within
 * words and long words (int32).
 * Macros ending with W deal with words, L macros deal with longs
 */
/// Returns the high byte of a word.
#define HIBYTEW(b)		(((b) & 0xff00) >> 8)
/// Returns the low byte of a word.
#define LOBYTEW(b)		( (b) & 0xff)

/// Returns the high byte of a long.
#define HIBYTEL(b)		(((b) & 0xff000000L) >> 24)
/// Returns the low byte of a long.
#define LOBYTEL(b)		( (b) & 0xffL)

/// Returns byte 0 of a long.
#define BYTE0L(b)		( (b) & 0xffL)
/// Returns byte 1 of a long.
#define BYTE1L(b)		(((b) & 0xff00L) >> 8)
/// Returns byte 2 of a long.
#define BYTE2L(b)		(((b) & 0xff0000L) >> 16)
/// Returns byte 3 of a long.
#define BYTE3L(b)		(((b) & 0xff000000L) >> 24)

/// Returns the high word of a long.
#define HIWORDL(b)		(((b) & 0xffff0000L) >> 16)
/// Returns the low word of a long.
#define LOWORDL(b)		( (b) & 0xffffL)

/**
 *	This macro takes a dword ordered 0123 and reorder it to 3210.
 */
#define SWAP_DW(a)	  ( (((a) & 0xff000000)>>24) |	\
						(((a) & 0xff0000)>>8) |		\
						(((a) & 0xff00)<<8) |		\
						(((a) & 0xff)<<24) )

#else
/* big endian macros go here */
#endif

/// This macro is used to enter the debugger.
#if defined( _XBOX360 )
#define ENTER_DEBUGGER() DebugBreak()
#elif defined( _WIN64 )
#define ENTER_DEBUGGER() __debugbreak()
#elif defined( _WIN32 )
#define ENTER_DEBUGGER() __asm { int 3 }
#elif defined( PLAYSTATION3 )
#define ENTER_DEBUGGER() __asm__ volatile ( ".int 0" )
#elif defined( __APPLE__ ) || defined( __ANDROID__ ) || defined( EMSCRIPTEN )
//TODO: ENTER_DEBUGGER for iOS, Android, Emscripten
#define ENTER_DEBUGGER() 
#else
#define ENTER_DEBUGGER() asm( "int $3" )
#endif


/**
 *	This function returns user id.
 */
inline int getUserId()
{
#ifdef _WIN32
	// VS2005:
	#if _MSC_VER >= 1400
		char uid[16];
		size_t sz;
		return getenv_s( &sz, uid, sizeof( uid ), "UID" ) == 0 ? atoi( uid ) : 0;

	// VS2003:
	#elif _MSC_VER < 1400
		char * uid = getenv( "UID" );
		return uid ? atoi( uid ) : 0;
	#endif
#elif defined( PLAYSTATION3 )
	return 123;
#else
// Linux:
	char * uid = getenv( "UID" );
	return uid ? atoi( uid ) : getuid();
#endif
}


/**
 *	This function returns the process id.
 */
CSTDMF_DLL int mf_getpid();

#if defined( __unix__ ) || defined( PLAYSTATION3 ) || defined( __APPLE__ ) || \
	defined( __ANDROID__ ) || defined( EMSCRIPTEN )

#if !defined( __APPLE__ )
#define bw_isnan isnan
#define bw_isinf isinf
#else // defined( __APPLE__ )
#define bw_isnan std::isnan
#define bw_isinf std::isinf
#endif // !defined( __APPLE__ )

#define bw_snprintf snprintf
#define bw_vsnprintf vsnprintf
#define bw_vsnwprintf vsnwprintf
#define bw_snwprintf swprintf
#define bw_stricmp strcasecmp
#define bw_strnicmp strncasecmp
#define bw_fileno fileno
#define bw_fsync fsync

#if defined( __APPLE__ )
#define bw_va_copy __va_copy
#else
#define bw_va_copy va_copy
#endif

#else

#define bw_isnan _isnan
#define bw_isinf(x) (!_finite(x) && !_isnan(x))
inline int bw_snprintf( char * dest, size_t size, const char * format, ... );
inline int bw_vsnprintf( char * dest, size_t size, const char * format,
	va_list args );
inline int bw_snwprintf( wchar_t * dest, size_t size, const wchar_t * format,
	... );
inline int bw_vsnwprintf( wchar_t * dest, size_t size, const wchar_t * format,
	va_list args );
#define bw_strnicmp _strnicmp
#define bw_fileno _fileno
#define bw_fsync _commit
#define bw_va_copy( dst, src) dst = src


inline int bw_snprintf( char * dest, size_t size, const char * format, ... )
{
	va_list args;
	va_start( args, format );
	int result = bw_vsnprintf( dest, size, format, args );
	va_end( args );
	return result;
}

/**
 * This function is intended to comply with the C99 standard.
 * The msvc version of this function does not.
 *
 * C99 standard states:
 * The snprintf function returns the number of characters that would 
 * have been written had n been sufficiently large, not counting the 
 * terminating null character, or a negative value if an encoding error 
 * occurred. Thus, the null-terminated output has been completely written 
 * if and only if the returned value is nonnegative and less than n.
 *
 * Also:
 * The snprintf function is equivalent to fprintf, except that the output 
 * is written into an array (specified by argument s) rather than to a stream.
 * If n is zero, nothing is written, and s may be a null pointer. Otherwise,
 * output characters beyond the n-1st are discarded rather than being written
 * to the array, and a null character is written at the end of the characters 
 * actually written into the array.
 * 
 * So these functions need to guarantee null termination
 */
inline int bw_vsnprintf( char * dest, size_t size, const char * format,
		va_list args )
{
	int result = -1;
	if (size != 0)
	{
		result = _vsnprintf( dest, size - 1, format, args );
		dest[size - 1] = '\0';
	}

	if (result == -1)
	{
		// If it failed, then return the count, like the standard
		result = _vscprintf( format, args );
	}
	return result;
}

inline int bw_snwprintf( wchar_t * dest, size_t size, const wchar_t * format, 
		... )
{
	va_list args;
	va_start( args, format );
	int result = bw_vsnwprintf( dest, size, format, args );
	va_end( args );
	return result;
}

inline int bw_vsnwprintf( wchar_t * dest, size_t size, const wchar_t * format,
		va_list args )
{
	// See bw_vsnprintf for C99 standard information.
	int result = -1;
	if (size != 0)
	{
		result = _vsnwprintf( dest, size - 1, format, args );
		dest[size - 1] = L'\0';
	}

	if (result == -1)
	{
		// If it failed, then return the count, like the standard
		result = _vscwprintf( format, args );
	}
	return result;
}

inline int bw_stricmp( const char * s1, const char * s2 )
{
	while(*s1 && (tolower(*s1) == tolower(*s2)))
	{
		++s1;
		++s2;
	}
	return (*s1 - *s2);
}

inline void bw_strlwr( char * s1 )
{
	while( *s1 )
	{
		*s1 = static_cast<char>(tolower( *s1 ));
		++s1;
	}
}

#endif // __unix__

inline void bw_zero_memory( void * ptr, size_t len )
{
	memset( ptr, 0, len );
}
BW_BEGIN_NAMESPACE

/* --------------------------------------------------------------------------
 * STL type info
 * --------------------------------------------------------------------------
 */

/**
 *  This class helps with using internal STL implementation details in
 *  different compilers.
 */
template <class MAP> struct MapTypes
{
#ifdef _WIN32
#if _MSC_VER>=1300 // VC7
	typedef typename MAP::mapped_type & _Tref;
#else
	typedef typename MAP::_Tref _Tref;
#endif
#else
	typedef typename MAP::mapped_type & _Tref;
#endif
};

#define MF_FLOAT_EQUAL(value1, value2) \
	(abs(value1 - value2) < std::numeric_limits<float>::epsilon())


/**
 *	Use this if you want to check if two floats are almost equal, but not exact.
 *	@param f1 first float to compare.
 *	@param f2 second float to compare.
 *	@param epsilon how "almost" equal the two floats can be.
 *		Defaults to 0.0004f because most existing functions are using it.
 */
inline bool almostEqual( const float f1, const float f2, const float epsilon = 0.0004f )
{
	return fabsf( f1 - f2 ) < epsilon;
}

inline bool almostEqual( const double d1, const double d2, const double epsilon = 0.0004 )
{
	return fabs( d1 - d2 ) < epsilon;
}

inline bool almostZero( const float f, const float epsilon = 0.0004f )
{
	return -epsilon < f && f < epsilon;
}

inline bool almostZero( const double d, const double epsilon = 0.0004 )
{
	return -epsilon < d && d < epsilon;
}

template<typename T>
inline bool almostEqual( const T& c1, const T& c2, const float epsilon = 0.0004f )
{
	if( c1.size() != c2.size() )
		return false;
	typename T::const_iterator iter1 = c1.begin();
	typename T::const_iterator iter2 = c2.begin();
	for( ; iter1 != c1.end(); ++iter1, ++iter2 )
		if( !almostEqual( *iter1, *iter2, epsilon ) )
			return false;
	return true;
}

BW_END_NAMESPACE

// Raw versions
inline void* raw_malloc( size_t count )
{
	return malloc( count );
}

inline void* raw_realloc( void* p, size_t count )
{
	return realloc( p, count );
}

inline void raw_free( void* p )
{
	free( p );
}

#if defined( WIN32 )
inline char * raw_strdup( const char * s )
{
	return _strdup( s );
}

inline wchar_t * raw_wcsdup( const wchar_t * s )
{
	return _wcsdup( s );
}
#endif // WIN32

/*
 * BW macros for cross-platform alloca.
 */
#if defined( _WIN32 )

#include <malloc.h>
#define bw_alloca _alloca

#else // !defined( _WIN32 )

#include <alloca.h>
#define bw_alloca alloca

#endif // defined( _WIN32 )

#endif // STDMF_HPP

/*end:stdmf.hpp*/
