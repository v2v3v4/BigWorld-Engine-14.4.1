#ifndef BW_SET_HPP
#define BW_SET_HPP

#include "stl_fixed_sized_allocator.hpp"
#include <set>

namespace BW
{
template < class Key, class Compare = std::less< Key >, class Allocator = BW::StlAllocator< Key > >
class set : public std::set< Key, Compare, Allocator >
{
	typedef std::set< Key, Compare, Allocator > set_base;
public:

	explicit set( const Compare& compare = Compare(), const Allocator& alloc = Allocator() ) :
		set_base( compare, alloc ) {}

	explicit set( const Allocator& alloc ) :
		set_base( alloc ) {}

	template < class InputIt >
	set( InputIt first, InputIt last, const Compare& compare = Compare(), const Allocator& alloc = Allocator() ) :
		set_base( first, last, compare, alloc ) {}

	set( const set& other ) :
		set_base( other ) {}

	set( const set& other, const Allocator& alloc ) :
		set_base( other, alloc ) {}

	set & operator=( const set & other )
	{
		set_base::operator=( other );
		return *this;
	}

#if (__cplusplus >= 201103L) && __GLIBCXX__
	set( set && other ) :
		set_base( std::move( other ) ) {}
#elif (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
	set( set&& other ) :
		set_base( std::move( other ), other.get_allocator() ) {}

	set( set&& other, const Allocator& alloc ) :
		set_base( std::move( other ), alloc ) {}
#endif

};

template < class Key, class Compare = std::less< Key >, class Allocator = BW::StlAllocator< Key > >
class multiset : public std::multiset< Key, Compare, Allocator >
{
	typedef std::multiset< Key, Compare, Allocator > multiset_base;
public:

	explicit multiset( const Compare& compare = Compare(), const Allocator& alloc = Allocator() ) :
		multiset_base( compare, alloc ) {}

	explicit multiset( const Allocator& alloc ) :
		multiset_base( alloc ) {}

	template < class InputIt >
	multiset( InputIt first, InputIt last, const Compare& compare = Compare(), const Allocator& alloc = Allocator() ) :
		multiset_base( first, last, compare, alloc ) {}

	multiset( const multiset& other ) :
		multiset_base( other ) {}

	multiset( const multiset& other, const Allocator& alloc ) :
		multiset_base( other, alloc ) {}

	multiset & operator=( const multiset & other )
	{
		multiset_base::operator=( other );
		return *this;
	}

#if (__cplusplus >= 201103L) && __GLIBCXX__
	multiset( multiset && other ) :
		multiset_base( std::move( other ) ) {}
#elif (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
	multiset( multiset&& other ) :
		multiset_base( std::move( other ), other.get_allocator() ) {}

	multiset( multiset&& other, const Allocator& alloc ) :
		multiset_base( std::move( other ), alloc ) {}
#endif

};

} // namespace BW

#endif // BW_SET_HPP
