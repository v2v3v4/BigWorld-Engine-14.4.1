#ifndef BW_STL_FIXED_SIZED_ALLOCATOR
#define BW_STL_FIXED_SIZED_ALLOCATOR

#include "cstdmf/config.hpp"
#include "cstdmf/cstdmf_dll.hpp"
#include "cstdmf/bw_memory.hpp"
#include "cstdmf/i_allocator.hpp"

#include <cstddef>

#if (__cplusplus >= 201103L)
#include <utility> // for std::forward
#endif

namespace BW
{

template < typename T >
void allocator_construct( T * p, const T & t )
{
	new( p ) T( t );
}


template <typename T>
void allocator_destroy( T * p )
{
	p->~T();
}

namespace Detail {
template < class T >
struct StlAllocatorBase
{ // remove top level const qualifier
	typedef T value_type;
};
#if defined(_MSC_VER)
template < class T >
struct StlAllocatorBase< const T >
{ // remove top level const qualifier
	typedef T value_type;
};
#endif
}

template < typename T >
class StlAllocator : public Detail::StlAllocatorBase< T >
{
	typedef Detail::StlAllocatorBase< T > base_type;
public:
	typedef typename base_type::value_type value_type;
	typedef value_type *       pointer;
	typedef const value_type * const_pointer;
	typedef value_type &       reference;
	typedef const value_type & const_reference;
	typedef std::size_t        size_type;
	typedef std::ptrdiff_t     difference_type;

	template < typename Other >
	struct rebind
	{
		typedef StlAllocator< Other > other;
	};

	StlAllocator()
#ifdef ALLOW_STACK_CONTAINER
		: allocator_( 0 )
#endif // ALLOW_STACK_CONTAINER
	{
	}

#ifdef ALLOW_STACK_CONTAINER
	explicit StlAllocator( IAllocator * allocator )
		: allocator_( allocator )
	{
	}
#endif // ALLOW_STACK_CONTAINER

	template <typename Other> 
	StlAllocator( const StlAllocator< Other > & otherAllocator )
#ifdef ALLOW_STACK_CONTAINER
		: allocator_( 0 )
#endif // ALLOW_STACK_CONTAINER
	{
	}

	StlAllocator(const StlAllocator< T >& rhs)
#ifdef ALLOW_STACK_CONTAINER
		: allocator_(
			rhs.allocator_ && rhs.allocator_->canAllocate()
				? rhs.allocator_ : NULL )
#endif // ALLOW_STACK_CONTAINER
	{
	}

	template < typename Other >
	StlAllocator & operator=( const StlAllocator< Other > & otherAllocator )
	{
#ifdef ALLOW_STACK_CONTAINER
		if (otherAllocator.allocator_ &&
			otherAllocator.allocator_->canAllocate())
		{
			allocator_ = otherAllocator.allocator_;
		}
		else
		{
			allocator_ = NULL;
		}
#endif // ALLOW_STACK_CONTAINER
		return *this;
	}


	template < typename Other >
	bool operator==( const StlAllocator< Other > & ) const
	{
		return true;
	}

	template < typename Other >
	bool operator!=( const StlAllocator< Other > & ) const
	{
		return false;
	}

	~StlAllocator()
	{
	}

	pointer address( reference x ) { return &x; }

	const_pointer address( const_reference x ) const { return &x; }

	pointer allocate(size_type n, const void* hint = 0)
	{
#ifdef ALLOW_STACK_CONTAINER
		if (allocator_)
		{
			return reinterpret_cast< pointer >(
				allocator_->intAllocate( n, hint ) );
		}
#endif // ALLOW_STACK_CONTAINER
		return reinterpret_cast< pointer >( bw_new( n * sizeof( value_type ) ) );
	}

	void deallocate( pointer p, size_type n )
	{ 
#ifdef ALLOW_STACK_CONTAINER
		if (allocator_)
		{
			allocator_->intDeallocate( p, n );
			return;
		}
#endif // ALLOW_STACK_CONTAINER
		bw_delete( reinterpret_cast< void * >( p ) );
	}

	size_type max_size() const
	{
#ifdef ALLOW_STACK_CONTAINER
		if (allocator_)
		{
			return allocator_->getMaxSize();
		}
#endif // ALLOW_STACK_CONTAINER
		return size_t( -1 ) / sizeof( value_type );
	}

#if (__cplusplus >= 201103L)
	template< typename Up, typename... Args >
	void construct( Up* p, Args &&... args )
	{
		::new( (void *)p ) Up( std::forward< Args >( args )... );
	}
#else
	void construct( pointer p, const T & val ) const 
	{
		allocator_construct( p, val );
	}
#endif


	void destroy( pointer p ) const
	{
		allocator_destroy( p );
	}
#ifdef ALLOW_STACK_CONTAINER
private:
	IAllocator * allocator_;
#endif // ALLOW_STACK_CONTAINER

};

CSTDMF_EXPORTED_TEMPLATE_CLASS StlAllocator< char >;
CSTDMF_EXPORTED_TEMPLATE_CLASS StlAllocator< wchar_t >;

} // namespace BW


#endif // BW_STL_FIXED_SIZED_ALLOCATOR

