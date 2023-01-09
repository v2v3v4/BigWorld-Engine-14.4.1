#ifndef BW_UNORDERED_SET_HPP
#define BW_UNORDERED_SET_HPP

#include "stl_fixed_sized_allocator.hpp"

#if _MSC_VER
#include <unordered_set>
#else
#include <tr1/unordered_set>
#endif

#include "bw_hash.hpp"

namespace BW
{
template < class Key, class Hash = BW::hash< Key >, class KeyEqual = std::equal_to< Key >, class Allocator = BW::StlAllocator< Key > >
class unordered_set : public std::tr1::unordered_set< Key, Hash, KeyEqual, Allocator >
{
	typedef std::tr1::unordered_set< Key, Hash, KeyEqual, Allocator > type_base;
public:
	typedef typename type_base::size_type size_type;

	unordered_set() :
		type_base() {}

	explicit unordered_set( size_type bucket_count, const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator() ) :
		type_base( bucket_count, hash, equal, alloc ) {}

	explicit unordered_set( const Allocator& alloc ) :
		type_base( alloc ) {}

	template < class InputIt >
	unordered_set( InputIt first, InputIt last ) :
		type_base( first, last ) {}

	template < class InputIt >
	unordered_set( InputIt first, InputIt last, size_type bucket_count, const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator() ) :
		type_base( first, last, bucket_count, hash, equal, alloc ) {}

	unordered_set( const unordered_set& other ) :
		type_base( other ) {}

	unordered_set( const unordered_set& other, const Allocator& alloc ) :
		type_base( other, alloc ) {}

	unordered_set & operator=( const unordered_set & other )
	{
		type_base::operator=( other );
		return *this;
	}

#if (__cplusplus >= 201103L) && __GLIBCXX__
	unordered_set( unordered_set && other ) :
		type_base( std::move( other ) ) {}
#elif (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
	unordered_set( unordered_set&& other ) :
		type_base( std::move( other ), other.get_allocator() ) {}

	unordered_set( unordered_set&& other, const Allocator& alloc ) :
		type_base( std::move( other ), alloc ) {}
#endif
};

template < class Key, class Hash = BW::hash< Key >, class KeyEqual = std::equal_to< Key >, class Allocator = BW::StlAllocator< Key > >
class unordered_multiset : public std::tr1::unordered_multiset< Key, Hash, KeyEqual, Allocator >
{
	typedef std::tr1::unordered_multiset< Key, Hash, KeyEqual, Allocator > type_base;
public:
	typedef typename type_base::size_type size_type;

	unordered_multiset() :
		type_base() {}

	explicit unordered_multiset( size_type bucket_count, const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator() ) :
	type_base( bucket_count, hash, equal, alloc ) {}

	explicit unordered_multiset( const Allocator& alloc ) :
	type_base( alloc ) {}

	template < class InputIt >
	unordered_multiset( InputIt first, InputIt last ) :
		type_base( first, last ) {}

	template < class InputIt >
	unordered_multiset( InputIt first, InputIt last, size_type bucket_count, const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator() ) :
		type_base( first, last, bucket_count, hash, equal, alloc ) {}

	unordered_multiset( const unordered_multiset& other ) :
		type_base( other ) {}

	unordered_multiset( const unordered_multiset& other, const Allocator& alloc ) :
		type_base( other, alloc ) {}

	unordered_multiset & operator=( const unordered_multiset & other )
	{
		type_base::operator=( other );
		return *this;
	}

#if (__cplusplus >= 201103L) && __GLIBCXX__
	unordered_multiset( unordered_multiset && other ) :
		type_base( std::move( other ) ) {}
#elif (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
	unordered_multiset( unordered_multiset&& other ) :
		type_base( std::move( other ), other.get_allocator() ) {}

	unordered_multiset( unordered_multiset&& other, const Allocator& alloc ) :
		type_base( std::move( other ), alloc ) {}
#endif
};

} // namespace BW

#endif // BW_UNORDERED_SET_HPP
