#ifndef BW_DEQUE_HPP
#define BW_DEQUE_HPP

#include "stl_fixed_sized_allocator.hpp"
#include <cstddef>
#include <deque>

namespace BW
{
template < class T, class Allocator = StlAllocator< T > >
class deque : public std::deque< T, Allocator >
{
	typedef std::deque< T, Allocator > deque_base;
public:
	typedef typename deque_base::size_type size_type;

	deque() : 
		deque_base() {}

	explicit deque( const Allocator& alloc ) :
		deque_base( alloc ) {}

	deque( size_type count, const T& value, const Allocator& alloc = Allocator() ) :
		deque_base( count, value, alloc ) {}

	template < class InputIt >
	deque( InputIt first, InputIt last, const Allocator& alloc = Allocator() ) :
		deque_base( first, last, alloc ) {}

	deque( const deque & other ) :
		deque_base( other ) {}

	deque & operator=( const deque & other )
	{
		deque_base::operator=( other );
		return *this;
	}

#if (__cplusplus >= 201103L) && __GLIBCXX__
	deque( deque && other ) :
		deque_base( std::move( other ) );
#elif (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
	deque( deque && other ) :
		deque_base( std::move( other ), other.get_allocator() ) {}

	deque( deque && other, const Allocator& alloc ) :
		deque_base( std::move( other ), alloc ) {}
#endif

};
} // namespace BW

#endif // BW_DEQUE_HPP
