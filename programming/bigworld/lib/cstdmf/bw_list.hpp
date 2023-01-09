#ifndef BW_LIST_HPP
#define BW_LIST_HPP

#include "stl_fixed_sized_allocator.hpp"
#include <list>

namespace BW
{
template < class T, class Allocator = BW::StlAllocator< T > >
class list : public std::list< T, Allocator >
{
	typedef std::list< T, Allocator > list_base;
public:
	typedef typename list_base::size_type size_type;

	list() :
		list_base() {}

	explicit list( const Allocator& alloc ) :
		list_base( alloc ) {}

	explicit list( size_type count ) :
		list_base( count ) {}

	list( size_type count, const T& value, const Allocator& alloc = Allocator() ) :
		list_base( count, value, alloc ) {}

	template < class InputIt >
	list( InputIt first, InputIt last, const Allocator& alloc = Allocator() ) :
		list_base( first, last, alloc ) {}

	list( const list& other ) :
		list_base( other ) {}
	
	list & operator=( const list & other )
	{
		list_base::operator=( other );
		return *this;
	}

#if (__cplusplus >= 201103L) && __GLIBCXX__
	list( list && other ) :
		list_base( std::move( other ) ) {}
#elif (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
	list( list&& other ) :
		list_base( std::move( other ), other.get_allocator() ) {}

	list( list&& other, const Allocator& alloc ) :
		list_base( std::move( other ), alloc ) {}
#endif

};
} // namespace BW

#endif // BW_LIST_HPP
