#ifndef BW_VECTOR_HPP
#define BW_VECTOR_HPP

#include "stdmf_minimal.hpp"
#include "stl_fixed_sized_allocator.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace BW
{
template < class T, class Allocator = StlAllocator< T > >
class vector : public std::vector< T, Allocator >
{
	typedef std::vector< T, Allocator > vector_base;
public:
	typedef typename vector_base::size_type size_type;

	vector() :
		vector_base() {}

	explicit vector( const Allocator& alloc ) :
		vector_base( alloc ) {}

	explicit vector( size_type count ) :
		vector_base( count ) {}

	vector( size_type count, const T& value, const Allocator& alloc = Allocator() ) :
		vector_base( count, value, alloc ) {}

	template < class InputIt >
	vector( InputIt first, InputIt last, const Allocator& alloc = Allocator() ) :
		vector_base( first, last, alloc ) {}

	vector( const vector& other ) :
		vector_base( other ) {}

#if (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
	vector & operator=( const vector & other )
	{
		this->vector_base::operator=( other );
		return *this;
	}

	vector( vector&& other ) :
		vector_base( std::move( other ), other.get_allocator() ) {}

	vector( vector&& other, const Allocator& alloc ) :
		vector_base( std::move( other ), alloc ) {}
#endif // (__cplusplus >= 201103L) || (_MSC_VER >= 1700)

};

/**
 *	Specialisation of isEqual template equality comparison for
 *	BW::vector.
 */
template< typename T >
inline bool isEqual( const BW::vector< T > & v1, const BW::vector< T > & v2 )
{
	typedef bool (*comparator)( const T &, const T & );
	return v1.size() == v2.size() &&
		std::equal( v1.begin(), v1.end(), v2.begin(),
				static_cast< comparator >( isEqual< T > ) );
}


} // namespace BW

#endif // BW_VECTOR_HPP
