#ifndef BW_UNORDERED_MAP_HPP
#define BW_UNORDERED_MAP_HPP

#include "stl_fixed_sized_allocator.hpp"

#if _MSC_VER || defined( EMSCRIPTEN )
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif

#include "bw_hash.hpp"


namespace BW
{

#if defined( EMSCRIPTEN ) || (defined( _MSC_VER ) && _MSC_VER >= 1910)

#define STD_UNORDERED_MAP std::unordered_map
#define STD_UNORDERED_MULTIMAP std::unordered_multimap

#else // !defined( EMSCRIPTEN )

#define STD_UNORDERED_MAP std::tr1::unordered_map
#define STD_UNORDERED_MULTIMAP std::tr1::unordered_multimap

#endif


template < class Key, class T, class Hash = BW::hash< Key >, class KeyEqual = std::equal_to< Key >, class Allocator = BW::StlAllocator< std::pair< const Key, T > > >
class unordered_map : public STD_UNORDERED_MAP< Key, T, Hash, KeyEqual, Allocator >
{
	typedef STD_UNORDERED_MAP< Key, T, Hash, KeyEqual, Allocator > type_base;
public:
	typedef typename type_base::size_type size_type;

	unordered_map() :
		type_base() {}

	explicit unordered_map( size_type bucket_count, const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator() ) :
		type_base( bucket_count, hash, equal, alloc ) {}

	explicit unordered_map( const Allocator& alloc ) :
#if (_MSC_VER <= 1600)
		type_base( type_base::_Min_buckets, Hash(), KeyEqual(), alloc ) {}
#else
		type_base( alloc ) {}
#endif

	template < class InputIt >
	unordered_map( InputIt first, InputIt last ) :
		type_base( first, last ) {}

	template < class InputIt >
	unordered_map( InputIt first, InputIt last, size_type bucket_count, const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator() ) :
		type_base( first, last, bucket_count, hash, equal, alloc ) {}

	unordered_map( const unordered_map& other ) :
		type_base( other ) {}

	unordered_map( const unordered_map& other, const Allocator& alloc ) :
		type_base( other, alloc ) {}

	unordered_map & operator=( const unordered_map & other )
	{
		type_base::operator=( other );
		return *this;
	}

#if (__cplusplus >= 201103L) && __GLIBCXX__
	unordered_map( unordered_map && other ) :
		type_base( std::move( other ) ) {}
#elif (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
	unordered_map( unordered_map&& other ) :
		type_base( std::move( other ), other.get_allocator() ) {}

	unordered_map( unordered_map&& other, const Allocator& alloc ) :
		type_base( std::move( other ), alloc ) {}
#endif
};

template < class Key, class T, class Hash = BW::hash< Key >, class KeyEqual = std::equal_to< Key >, class Allocator = BW::StlAllocator< std::pair< const Key, T > > >
class unordered_multimap : public STD_UNORDERED_MULTIMAP< Key, T, Hash, KeyEqual, Allocator >
{
	typedef STD_UNORDERED_MULTIMAP< Key, T, Hash, KeyEqual, Allocator > type_base;
public:
	typedef typename type_base::size_type size_type;

	unordered_multimap() :
		type_base() {}

	explicit unordered_multimap( size_type bucket_count, const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator() ) :
		type_base( bucket_count, hash, equal, alloc ) {}

	explicit unordered_multimap( const Allocator& alloc ) :
		type_base( alloc ) {}

	template < class InputIt >
	unordered_multimap( InputIt first, InputIt last ) :
		type_base( first, last ) {}

	template < class InputIt >
	unordered_multimap( InputIt first, InputIt last, size_type bucket_count, const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator() ) :
		type_base( first, last, bucket_count, hash, equal, alloc ) {}

	unordered_multimap( const unordered_multimap& other ) :
		type_base( other ) {}

	unordered_multimap( const unordered_multimap& other, const Allocator& alloc ) :
		type_base( other, alloc ) {}

	unordered_multimap & operator=( const unordered_multimap & other )
	{
		type_base::operator=( other );
		return *this;
	}

#if (__cplusplus >= 201103L) && __GLIBCXX__
	unordered_multimap( unordered_multimap && other ) :
		type_base( std::move( other ) ) {}
#elif (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
	unordered_multimap( unordered_multimap&& other ) :
		type_base( std::move( other ), other.get_allocator() ) {}

	unordered_multimap( unordered_multimap&& other, const Allocator& alloc ) :
		type_base( std::move( other ), alloc ) {}
#endif // (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
};

} // namespace BW

#endif // BW_UNORDERED_MAP_HPP
