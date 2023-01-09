#ifndef BW_STRING_REF_HPP
#define BW_STRING_REF_HPP

#include <cctype>
#include <cwctype>
#include <cstddef>
#include <cstring>
#include "bw_hash.hpp"
#include "stl_fixed_sized_allocator.hpp"
#include "debug.hpp"
#include "cstdmf/stdmf.hpp"

namespace BW
{

///
/// A BasicStringRef represents a read only reference to string data.
/// Internally this is stored as a const char pointer and a length.
/// The BasicStringRef class does not own the data and the lifetime of the
/// data passed to the string ref must exceed the lifetime of the 
/// BasicStringRef.
/// 
/// The BasicStringRef is compatible with std algorithms and containers.
/// 
/// The primary purpose of a BasicStringRef is efficiency, it can be used to
/// avoid the cost of allocating a string class.
///
/// The down side is that BasicStringRef data is not null terminated, to get
/// a null terminated string you must use the to_string() or copy_to()
/// methods.
/// 
/// Inspired by http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3442.html
///
template< typename CharT, typename Traits = std::char_traits< CharT > >
class BasicStringRef
{
	const CharT * str_;
	size_t length_;
	CSTDMF_DLL static const CharT s_nullTerminator_;

	static inline char toLower( char ch )
	{
		return std::tolower( ch );
	}

	static inline wchar_t toLower( wchar_t ch )
	{
		return std::towlower( ch );
	}

public:
	typedef BasicStringRef< CharT > type;
	typedef Traits                  traits_type;
	typedef size_t                  size_type;
	typedef const CharT *           const_iterator;

	CSTDMF_DLL static const size_type npos;

	BasicStringRef() :
		str_( &s_nullTerminator_ ),
		length_( 0 )
	{}

	BasicStringRef( const BasicStringRef & rhs ) :
		str_( rhs.str_ ),
		length_( rhs.length_ )
	{}

	BasicStringRef & operator=( const BasicStringRef & rhs )
	{
		str_ = rhs.str_;
		length_ = rhs.length_;
		return *this;
	}

	BasicStringRef( const CharT* str ) :
		str_( str ),
		length_( traits_type::length( str ) )
	{
		MF_ASSERT( str != NULL );
	}

	BasicStringRef( const CharT * str, size_t length ) :
		str_( str ),
		length_( length )
	{
		MF_ASSERT( str != NULL );
	}

	template < typename Alloc >
	BasicStringRef( const std::basic_string< CharT, Traits, Alloc > & str ) :
		str_( str.c_str() ),
		length_( str.length() )
	{}

	bool empty() const
	{
		return length_ == 0;
	}

	size_type length() const
	{
		return length_;
	}

	size_type size() const
	{
		return length_;
	}

	const CharT * data() const
	{
		return str_;
	}

	const_iterator begin() const
	{
		MF_ASSERT( str_ );
		return str_;
	}

	const_iterator end() const
	{
		MF_ASSERT( str_ );
		return str_ + length_;
	}

	std::basic_string< CharT, std::char_traits< CharT >, StlAllocator< CharT > >
		to_string() const
	{
		MF_ASSERT( str_ );
		return std::basic_string< CharT, Traits, StlAllocator< CharT > >( str_,
			length_ );
	}

	template < typename Alloc >
	void copy_to( std::basic_string< CharT, Traits, Alloc > & str ) const
	{
		MF_ASSERT( str_ );
		str.assign( str_, length_ );
	}

	CharT at( size_type index ) const
	{
		MF_ASSERT( index < length_ );
		return str_[ index ];
	}

	int compare( const BasicStringRef< CharT > & str ) const
	{
		size_type lhs_sz = length_;
		size_type rhs_sz = str.length_;
		int result = traits_type::compare( str_, str.str_,
			std::min( lhs_sz, rhs_sz ) );
		if (result != 0)
		{
			return result;
		}
		if (lhs_sz < rhs_sz)
		{
			return -1;
		}
		if (lhs_sz > rhs_sz)
		{
			return 1;
		}
		return 0;
	}

	int compare_lower( const BasicStringRef< CharT > & str ) const
	{
		size_type lhs_sz = length_;
		size_type rhs_sz = str.length_;
		for (size_t i = 0, min_sz = std::min( lhs_sz, rhs_sz ); i != min_sz; ++i) 
		{
			CharT lhc = toLower(str_[i]);
			CharT rhc = toLower(str.str_[i]);
			if (lhc != rhc)
			{
				return lhc < rhc ? -1 : 1;
			}
		}
		if (lhs_sz < rhs_sz)
		{
			return -1;
		}
		if (lhs_sz > rhs_sz)
		{
			return 1;
		}
		return 0;
	}

	bool equals( const BasicStringRef< CharT > & str ) const
	{
		return (compare( str ) == 0);
	}

	bool equals_lower( const BasicStringRef< CharT > & str ) const
	{
		return (compare_lower( str ) == 0);
	}

	BasicStringRef< CharT > substr( size_type start = 0,
		size_type length = npos ) const
	{
		return BasicStringRef< CharT >( str_ + std::min( start, length_ ), 
			std::min( length, length_ - start ) );
	}

	size_type find( CharT ch, size_type pos = 0 ) const
	{
		for (size_type i = std::min( pos, length_ ); i < length_; ++i )
		{
			if (str_[i] == ch) return i;
		}
		return npos;
	}

	size_type find( const BasicStringRef< CharT > & str,
		size_type pos = 0 ) const
	{
		if ( str.length_ < length_ )
		{
			for (size_type i = std::min( pos, length_ ), 
				end = length_ - str.length_; i <= end; ++i)
			{
				if (substr( i, str.length_ ) == str) return i;
			}
		}
		return npos;
	}

	size_type rfind( CharT ch, size_type pos = npos ) const
	{
		for (size_type i = std::min( pos, length_ ); i > 0; --i)
		{
			if (str_[i - 1] == ch) return i - 1;
		}
		return npos;
	}

	size_type rfind( const BasicStringRef< CharT > & str,
		size_type pos = npos ) const
	{
		for (size_type i = std::min( pos, length_ ); i >= str.length(); --i)
		{
			if (substr( i - str.length(), str.length() ) == str)
				return i - str.length();
		}
		return npos;
	}

	size_type find_first_of( CharT ch, size_type pos = 0 ) const
	{
		for (size_type i = std::min( pos, length_ ); i < length_; ++i)
		{
			if (str_[i] == ch) return i;
		}
		return npos;	
	}

	size_type find_first_of( const BasicStringRef< CharT > & str,
		size_type pos = 0 ) const
	{
		size_type result = npos;
		size_type length = length_;
		pos = std::min( pos, length_ );
		for (const_iterator itr = str.begin(), itrEnd = str.end(); 
			itr < itrEnd; ++itr)
		{
			for (size_type i = pos; i < length; ++i )
			{
				if (str_[i] == *itr)
				{
					result = std::min( result, i );
					length = result;
					break;
				}
			}
		}
		return result;
	}

	size_type find_first_of( const CharT * str, size_type pos,
		size_type count ) const
	{
		return find_first_of( BasicStringRef< CharT >( str, count ), pos );
	}

	size_type find_first_not_of( CharT ch, size_type pos = 0 ) const
	{
		for (size_type i = std::min( pos, length_ ); i < length_; ++i)
		{
			if (str_[i] != ch) return i;
		}
		return npos;
	}

	size_type find_first_not_of( const BasicStringRef< CharT > & str,
		size_type pos = 0 ) const
	{
		if (pos < length_)
		{
			const CharT * itrEnd = str_ + length_;
			for (const CharT * itr = str_ + pos; itr < itrEnd; ++itr)
				if (str.find( *itr ) == npos)
					return (itr - str_);
		}
		return npos;
	}

	size_type find_first_not_of( const CharT * str, size_type pos,
		size_type count ) const
	{
		return find_first_not_of( BasicStringRef< CharT >( str, count ), pos );
	}

	size_type find_last_of( CharT ch, size_type pos = npos ) const
	{
		for (size_type i = std::min( pos, length_ ); i > 0; --i)
		{
			if (str_[i - 1] == ch) return i - 1;
		}
		return npos;
	}

	size_type find_last_of( const BasicStringRef< CharT> & str,
		size_type pos = npos ) const
	{
		size_type result = npos;
		size_type start = 0;
		pos = std::min( pos, length_ );
		for (const_iterator itr = str.begin(), itrEnd = str.end();
			itr < itrEnd; ++itr)
		{
			for (size_type i = pos; i > start; --i)
			{
				if (str_[i - 1] == *itr)
				{
					start = std::max( start, i - 1 );
					result = std::max( result + 1, i ) - 1;
					break;
				}
			}
		}
		return result;
	}

	size_type find_last_of( const CharT * str, size_type pos,
		size_type count ) const
	{
		return find_last_of( BasicStringRef< CharT >( str, count ), pos );
	}

	size_type find_last_not_of( const BasicStringRef< CharT> & str,
		size_type pos = npos ) const
	{
		if (length_ > 0)
		{
			const CharT * itrEnd = str_;
			const CharT * itr = str_ + std::min( pos, length_ - 1 );
			for (; ; --itr)
			{
				if (Traits::find( str.str_, str.length_, *itr ) == 0)
				{
					return (itr - str_);
				}
				else if (itr == itrEnd)
				{
					break;
				}
			}
		}
		return npos;
	}

	size_type find_last_not_of( const CharT * str, size_type pos,
		size_type count ) const
	{
		return find_last_not_of( BasicStringRef< CharT >( str, count ), pos );
	}

	size_type find_last_not_of( CharT ch, size_type pos = npos) const
	{
		return find_last_not_of( BasicStringRef< CharT >( &ch, 1 ), pos );
	}

	CharT operator[]( size_type index ) const
	{
		return at( index );
	}

	bool operator==( const BasicStringRef< CharT > & rhs ) const
	{
		return equals( rhs );
	}

	bool operator!=( const BasicStringRef< CharT > & rhs ) const
	{
		return !equals( rhs );
	}

	bool operator<( const BasicStringRef< CharT > & rhs ) const
	{
		return compare( rhs ) < 0;
	}

	bool operator<=( const BasicStringRef< CharT > & rhs ) const
	{
		return compare( rhs ) <= 0;
	}

	bool operator>( const BasicStringRef< CharT > & rhs ) const
	{
		return compare( rhs ) > 0;
	}

	bool operator>=( const BasicStringRef< CharT > & rhs ) const
	{
		return compare( rhs ) >= 0;
	}
};

typedef BasicStringRef< char, std::char_traits< char > > StringRef;

typedef BasicStringRef< wchar_t, std::char_traits< wchar_t > > WStringRef;

template< typename CharT, typename Traits, typename Alloc >
inline bool operator==( 
	const std::basic_string< CharT, Traits, Alloc > & lhs, 
	const BasicStringRef< CharT, Traits > & rhs )
{
	return rhs.equals( lhs );
}

template< typename CharT, typename Traits, typename Alloc >
inline bool operator!=( 
	const std::basic_string< CharT, Traits, Alloc > & lhs, 
	const BasicStringRef< CharT, Traits > & rhs )
{
	return !rhs.equals( lhs );
}

template< typename CharT, typename Traits, typename Alloc >
inline bool operator==( const BasicStringRef< CharT, Traits > & lhs, 
	const std::basic_string< CharT, Traits, Alloc > & rhs )
{
	return lhs.equals( rhs );
}

template< typename CharT, typename Traits, typename Alloc >
inline bool operator!=( const BasicStringRef< CharT, Traits > & lhs, 
	const std::basic_string< CharT, Traits, Alloc > & rhs )
{
	return !lhs.equals( rhs );
}

// It would be preferable to not do this, but lots of existing string code uses +
inline string operator+( const StringRef & lhs, const StringRef & rhs )
{
	BW::string result;
	result.reserve( lhs.size() + rhs.size() + 1 );
	result.assign( lhs.data(), lhs.size() );
	result.append( rhs.data(), rhs.size() );
	return result;
}

inline string operator+( const string & lhs, const StringRef & rhs )
{
	return StringRef( lhs ) + rhs;
}

inline string operator+( const StringRef & lhs, const char * rhs )
{
	return lhs + StringRef( rhs );
}

inline string operator+( const char * lhs, const StringRef & rhs )
{
	return StringRef( lhs ) + rhs;
}

inline wstring operator+( const WStringRef & lhs, const WStringRef & rhs )
{
	BW::wstring result;
	result.reserve( lhs.size() + rhs.size() + 1 );
	result.assign( lhs.data(), lhs.size() );
	result.append( rhs.data(), rhs.size() );
	return result;
}

inline wstring operator+( const wstring & lhs, const WStringRef & rhs )
{
	return WStringRef( lhs ) + rhs;
}

inline wstring operator+( const WStringRef & lhs, const wchar_t * rhs )
{
	return lhs + WStringRef( rhs );
}

inline wstring operator+( const wchar_t * lhs, const WStringRef & rhs )
{
	return WStringRef( lhs ) + rhs;
}

namespace detail 
{
template< class CharT, class Traits >
inline void insert_fill_chars( std::basic_ostream< CharT, Traits> & os, 
	std::size_t n)
{
	enum { chunk_size = 8 };
	CharT fill_chars[chunk_size];
	std::fill_n( fill_chars, static_cast< std::size_t >(chunk_size), 
		os.fill() );
	for (; n >= chunk_size && os.good(); n -= chunk_size)
	{
		os.write( fill_chars, static_cast< std::size_t >( chunk_size ) );
	}
	if (n > 0 && os.good())
	{
		os.write( fill_chars, n );
	}
}

template< class CharT, class Traits >
void insert_aligned( std::basic_ostream< CharT, Traits > & os,
	const BasicStringRef< CharT, Traits> & str ) 
{
	const std::size_t size = str.size();
	const std::size_t alignment_size = 
		static_cast< std::size_t >( os.width() ) - size;
	const bool align_left = 
		( os.flags() & std::basic_ostream< CharT, Traits >::adjustfield) == 
		std::basic_ostream< CharT, Traits >::left;
	if (!align_left) 
	{
		detail::insert_fill_chars( os, alignment_size );
		if (os.good())
		{
			os.write( str.data(), size );
		}
	}
	else
	{
		os.write( str.data(), size );
		if (os.good())
		{
			detail::insert_fill_chars( os, alignment_size );
		}
	}
}

} // namespace detail

// Inserter
template< typename CharT, typename Traits >
inline std::basic_ostream< CharT, Traits > & operator<<(
	std::basic_ostream< CharT, Traits > & os,
	const BasicStringRef< CharT, Traits > & str)
{
	if (os.good())
	{
		const std::size_t size = str.size();
		const std::size_t w = static_cast< std::size_t >( os.width() );
		if (w <= size)
		{
			os.write( str.data(), size );
		}
		else
		{
			detail::insert_aligned( os, str );
		}
		os.width(0);
	}
	return os;
}

} // namespace BW

BW_HASH_NAMESPACE_BEGIN

template<>
struct hash< BW::StringRef > : 
	public std::unary_function< BW::StringRef, std::size_t >
{
	std::size_t operator()(const BW::StringRef & s) const
	{
		return BW::hash_string( s.data(), s.size() );
	}
};

template<>
struct hash< BW::WStringRef > : 
	public std::unary_function< BW::WStringRef, std::size_t >
{
	std::size_t operator()(const BW::WStringRef & s) const
	{
		return BW::hash_string( s.data(), s.size() * sizeof(wchar_t) );
	}
};

BW_HASH_NAMESPACE_END

#endif // #ifndef BW_STRING_HPP
