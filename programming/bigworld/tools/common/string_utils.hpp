/**
 *	StringUtils: String utility methods.
 */

#ifndef STRING_UTILS_WE_HPP
#define STRING_UTILS_WE_HPP

#include "cstdmf/guard.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

namespace StringUtils
{

	enum IncrementStyle { IS_EXPLORER, IS_END };
//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
	inline size_t strlenT( const char * s )
	{
		return strlen( s );
	}

//------------------------------------------------------------------------------
	inline size_t strlenT( const wchar_t * s )
	{
		return wcslen( s );
	}

//------------------------------------------------------------------------------
	bool PathMatchSpecT( const char * pszFileParam, const char * pszSpec );
	bool PathMatchSpecT( const wchar_t * pszFileParam, const wchar_t * pszSpec );

//------------------------------------------------------------------------------
	inline int stricmpT( const char * lhs, const char * rhs )
	{
		return _stricmp( lhs, rhs );
	}

//------------------------------------------------------------------------------
	inline int stricmpT( const wchar_t * lhs, const wchar_t * rhs )
	{
		return _wcsicmp( lhs, rhs );
	}

//------------------------------------------------------------------------------
	inline const char * strstrT( const char * str, const char * strSearch  )
	{
		return strstr( str, strSearch );
	}

//------------------------------------------------------------------------------
	inline const wchar_t * strstrT( const wchar_t * str, const wchar_t * strSearch  )
	{
		return wcsstr( str, strSearch );
	}

	
//------------------------------------------------------------------------------
// End of helpers
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
	template< typename T >
	T vectorToStringT( const BW::vector< T > & vec, char separator = ';' )
	{
		BW_GUARD;

		T ret;
		for( BW::vector< T >::const_iterator i = vec.begin(); i != vec.end(); ++i )
		{
			if ( ret.length() > 0 )
				ret += separator;
			ret += (*i);
		}

		return ret;
	}


//------------------------------------------------------------------------------
	template< typename T1, typename T2 >
	bool matchExtensionT( const T1 & fname, const T2 & extensions )
	{
		BW_GUARD;

		if ( extensions.empty() )
			return true;

		typename T1::size_type dot = fname.find_last_of( T1::value_type( '.' ) );
		if ( dot == T1::npos )
			return false;

		T1 ext = fname.substr( dot + 1 );

		for( T2::const_iterator i = extensions.begin(); i != extensions.end(); ++i )
			if ( stricmpT( (*i).c_str(), ext.c_str() ) == 0 )
				return true;

		return false;
	}


//------------------------------------------------------------------------------
	/**
	 *	This function tests to see if a string 'fname' matches the DOS-style
	 *	wildcard string 'spec' similar to PathMatchSpec, but case-sensitive.
	 */
	template< typename T >
	bool matchSpecCaseSensitiveT( const T * fname, const T * spec )
	{
		BW_GUARD;

		if ( fname == NULL || spec == NULL )
			return false;

		while ( *fname && *spec )
		{
			// find spec substring between *
			if ( *spec == T( '*' ) )
			{
				// skip stars
				while (*spec && *spec == T( '*' ))
					spec++;

				if ( !*spec )
					return true;

				const T *specSubstr = spec;

				// find end of substring
				unsigned int specSubstrLen = 0;
				while (*spec && *spec != T( '*' ))
				{
					spec++;
					specSubstrLen++;
				}

				const T *fnameSubstr = fname;
				bool match = false;
				while ( *fnameSubstr )
				{
					// if the spec substring is shorter than the fname, no match!
					if ( strlenT( fnameSubstr ) < specSubstrLen )
						return false;

					// compare the spec's substring with fname.
					match = true;
					for ( unsigned int i = 0; i < specSubstrLen; ++i )
					{
						if ( specSubstr[i] != fnameSubstr[i] && specSubstr[i] != T( '?' ) )
						{
							match = false;
							break;
						}
					}

					if ( match )
						break;

					fnameSubstr++;
				}

				if ( !match )
					return false;

				// If everything went well, update the string pointers
				fname = fnameSubstr + specSubstrLen;
				continue;
			}
			if ( *spec != *fname && *spec != T( '?' ) )
				return false;

			spec++;
			fname++;
		}

		// if spec is not at the end, try to skip any trailing stars.
		while (*spec && *spec == T( '*' ))
			spec++;

		// They match if both the fname and the spec where exhausted.
		return !*fname && !*spec;
	}


//------------------------------------------------------------------------------
	/**
	 *	This function tests to see if a string 'fname' matches at least one of the
	 *	DOS-style wildcard strings in the 'specs' vector. In addition, if a spec
	 *	starts with '!', the test for it is done case-sensitive.
	 */
	template< typename T1, typename T2 >
	bool matchSpecT( const T1 & fname, const T2 & specs )
	{
		BW_GUARD;

		if ( specs.empty() )
			return true;

		const T1::value_type * pfname = fname.c_str();

		for( T2::const_iterator i = specs.begin(); i != specs.end(); ++i )
		{
			const T1::value_type * spec = (*i).c_str();
			bool caseSensitive = false;
			if ( spec[0] == T1::value_type( '!' ) )
			{
				spec++;
				caseSensitive = true;
			}
			if ( caseSensitive )
			{
				if ( matchSpecCaseSensitiveT( pfname, spec ) )
					return true;
			}
			else
			{
				if ( PathMatchSpecT( pfname, spec ) )
					return true;
			}
		}

		return false;
	}

//------------------------------------------------------------------------------
	template< typename T1, typename T2 >
	bool findInVectorT( const T1 & str, const T2 & vec )
	{
		BW_GUARD;

		if ( vec.empty() )
			return true;

		for( BW::vector<BW::string>::const_iterator i = vec.begin(); i != vec.end(); ++i )
			if ( stricmpT( (*i).c_str(), str.c_str() ) == 0 )
				return true;

		return false;
	}

//------------------------------------------------------------------------------
	template< typename T >
	void filterSpecVectorT( T & vec, const T & exclude )
	{
		BW_GUARD;

		if ( vec.empty() || exclude.empty() )
			return;

		for( T::iterator i = vec.begin(); i != vec.end(); )
		{
			if ( matchSpecT( (*i), exclude ) )
				i = vec.erase( i );
			else
				++i;
		}
	}

//------------------------------------------------------------------------------
	template< typename T >
	void toLowerCaseT( T & str )
	{
		std::transform( str.begin(), str.end(), str.begin(), tolower );
	}

//------------------------------------------------------------------------------
	template< typename T >
	void toUpperCaseT( T & str )
	{
		std::transform( str.begin(), str.end(), str.begin(), toupper );
	}

//------------------------------------------------------------------------------
	template< typename T >
	T lowerCaseT( const T & str )
	{
		T temp = str;
		toLowerCaseT( temp );
		return temp;
	}

//------------------------------------------------------------------------------
	template< typename T >
	T upperCase( const T & str )
	{
		T temp = str;
		toUpperCaseT( temp );
		return temp;
	}

//------------------------------------------------------------------------------
	template< typename T >
	void toMixedCaseT( T & str )
	{
		BW_GUARD;

		bool lastSpace = true;

		T::iterator it = str.begin();
		T::iterator end = str.end();

		for( ; it != end; ++it )
		{
			if ( lastSpace )
				*it = toupper( *it );
			else
				*it = tolower( *it );
			lastSpace = ( *it == T::value_type( ' ' ) );
		}
	}

//------------------------------------------------------------------------------
	template< typename T >
	void replace( T & str, const T & from, const T & to )
	{
		BW_GUARD;

		if( !from.empty() )
		{
			T newStr;
			while( const T::value_type * p = strstrT( str.c_str(), from.c_str() ) )
			{
				newStr.insert( newStr.end(), str.c_str(), p );
				newStr += to;
				str.erase( str.begin(), str.begin() + ( p - str.c_str() ) + from.size() );
			}
			str = newStr + str;
		}
	}

//------------------------------------------------------------------------------
	// Clipboard helpers
	template< typename T >
	class ClipboardFormatGetter
	{
		public:	static UINT get();
	};
	template<> inline UINT ClipboardFormatGetter< char >::get() { return CF_TEXT; }
	template<> inline UINT ClipboardFormatGetter< wchar_t >::get() { return CF_UNICODETEXT; }

//------------------------------------------------------------------------------
	template< typename T >
	bool copyToClipboardT( const T & str )
	{
		BW_GUARD;

		bool ok = false;
		if (::OpenClipboard(NULL))
		{
			HGLOBAL data = ::GlobalAlloc( GMEM_MOVEABLE, ( str.length() + 1 ) * sizeof( T::value_type ) );
			if ( data != NULL && ::EmptyClipboard() != FALSE )
			{
				T::value_type * cptr = (T::value_type*)::GlobalLock( data );
				memcpy( cptr, str.c_str(), str.length() * sizeof( T::value_type ) );
				cptr[ str.length() ] = T::value_type( '\0' );
				::GlobalUnlock( data );
				::SetClipboardData( ClipboardFormatGetter< T::value_type >::get(), data );
				ok = true;
			}
			::CloseClipboard();
		}
		return ok;
	}

//------------------------------------------------------------------------------
	/**
	 * This one has to be used like this:

	 bool bn = canCopyFromClipboardT< BW::string >();
	 bool bw = canCopyFromClipboardT< BW::wstring >();
	
	 */
	template< typename T >
	bool canCopyFromClipboardT()
	{
		BW_GUARD;

		bool ok = false;
		if (::OpenClipboard(NULL))
		{
			ok = ::IsClipboardFormatAvailable( ClipboardFormatGetter< T::value_type >::get() ) != FALSE;
			::CloseClipboard();
		}
		return ok;
	}

//------------------------------------------------------------------------------
	template< typename T >
	bool copyFromClipboardT( T & str )
	{
		BW_GUARD;

		bool ok = false;
		str.clear();
		if (::OpenClipboard(NULL))
		{
			HGLOBAL data = ::GetClipboardData( ClipboardFormatGetter< T::value_type >::get() );
			if (data != NULL)
			{
				T::value_type * cstr = (T::value_type *)::GlobalLock(data);
				str = cstr;
				::GlobalUnlock(data);
				ok = true;
			}
			::CloseClipboard();
		}
		return ok;
	}


//------------------------------------------------------------------------------
	void incrementT( BW::string & str, IncrementStyle incrementStyle );
	void incrementT( BW::wstring & str, IncrementStyle incrementStyle );

//------------------------------------------------------------------------------
	bool makeValidFilenameT( BW::string & str,
							 char replaceChar = '_',
							 bool allowSpaces = true );
	bool makeValidFilenameT( BW::wstring & str,
							 wchar_t replaceChar = L'_',
							 bool allowSpaces = true );


//------------------------------------------------------------------------------
	/**
	 * The function retrieve a token substring separated by ' ','\t' from a
		string. characters quoted in '""' is treated as one token, the return
		string excludes '"' character.
		Note: source string would be modified after calling.

		@param		a pointer of start position of string, on return it points
					to next-time start position

		@returns	token string (NULL terminated)
	 */
	template< typename T >
	T * retrieveCmdTokenT( T * & cmd )
	{
		BW_GUARD;

		if (!cmd) return NULL;

		T * result = *cmd && *cmd != T( ' ' ) && *cmd != T( '\t' ) ? cmd : NULL;

		while (*cmd)
		{
			switch (*cmd)
			{
			case ' ':
			case '\t':
				while (*cmd && (*cmd == ' ' || *cmd == '\t'))
				{
					*cmd++ = '\0';
				}
				if ( *cmd && !result)
				{
					result = cmd;
					break;
				}
				return result;
			case '"':
				result = ++cmd;
				while (*cmd && *cmd != '"')
				{
					++cmd;
				}
				if (*cmd)
				{
					*cmd++ = '\0';
				}
				return result;
			default:
				++cmd;
			}
		}
		return result;
	}

}

BW_END_NAMESPACE
#endif // STRING_UTILS_WE_HPP
