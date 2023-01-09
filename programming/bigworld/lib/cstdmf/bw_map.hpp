#ifndef BW_MAP_HPP
#define BW_MAP_HPP

#include "stl_fixed_sized_allocator.hpp"
#include <map>

namespace BW
{
template < class Key, class T, class Compare = std::less< Key >, class Allocator = BW::StlAllocator< std::pair< Key, T > > >
class map : public std::map< Key, T, Compare, Allocator >
{
	typedef std::map< Key, T, Compare, Allocator > map_base;
public:

	explicit map( const Compare& compare = Compare(), const Allocator& alloc = Allocator() ) :
		map_base( compare, alloc ) {}

	template < class InputIt >
	map( InputIt first, InputIt last, const Compare& compare = Compare(), const Allocator& alloc = Allocator() ) :
		map_base( first, last, compare, alloc ) {}

	map( const map& other ) :
		map_base( other ) {}

#if (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
	explicit map( const Allocator& alloc ) :
		map_base( alloc ) {}

	map( const map& other, const Allocator& alloc ) :
		map_base( other, alloc ) {}

	map & operator=( const map & other )
	{
		this->map_base::operator=( other );
		return *this;
	}

#if (__cplusplus >= 201103L) && __GLIBCXX__
	map( map && other ) :
		map_base( std::move( other ) ) {}
#elif (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
	map( map&& other ) :
		map_base( std::move( other ), other.get_allocator() ) {}

	map( map&& other, const Allocator& alloc ) :
		map_base( std::move( other ), alloc ) {}
#endif
#endif // (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
};

template < class Key, class T, class Compare = std::less< Key >, class Allocator = BW::StlAllocator< std::pair< Key, T > > >
class multimap : public std::multimap< Key, T, Compare, Allocator >
{
	typedef std::multimap< Key, T, Compare, Allocator > multimap_base;
public:

	explicit multimap( const Compare& compare = Compare(), const Allocator& alloc = Allocator() ) :
		multimap_base( compare, alloc ) {}

	template < class InputIt >
	multimap( InputIt first, InputIt last, const Compare& compare = Compare(), const Allocator& alloc = Allocator() ) :
		multimap_base( first, last, compare, alloc ) {}

	multimap( const multimap& other ) :
		multimap_base( other ) {}

	multimap & operator=( const multimap & other )
	{
		multimap_base::operator=( other );
		return *this;
	}

#if (__cplusplus >= 201103L) && __GLIBCXX__
	multimap( multimap && other ) :
		multimap_base( std::move( other ) ) {}
#elif (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
	explicit multimap( const Allocator& alloc ) :
		multimap_base( alloc ) {}

	multimap( const multimap& other, const Allocator& alloc ) :
		multimap_base( other, alloc ) {}

	multimap( multimap&& other ) :
		multimap_base( std::move( other ), other.get_allocator() ) {}

	multimap( multimap&& other, const Allocator& alloc ) :
		multimap_base( std::move( other ), alloc ) {}
#endif

};

} // namespace BW

#endif // BW_MAP_HPP
