#ifndef __CSTDMF_STATIC_ARRAY_HP___
#define __CSTDMF_STATIC_ARRAY_HP___

#include "stdmf.hpp"
#include "debug.hpp"

#if ENABLE_STACK_TRACKER
#include "cstdmf/stack_tracker.hpp"
#endif // ENABLE_STACK_TRACKER


BW_BEGIN_NAMESPACE

/**
 * This is a static storage policy for ArrayBase.
 * It provides a fixed size array to work with, and asserts if you trying to grow
 *
 * Space overhead: 1 size_t and 1 pointer
 *   No runtime allocations.
 */
template< typename TYPE, size_t INITIAL_COUNT>
class StaticStoragePolicy
{
public:
	enum { ARRAY_SIZE = INITIAL_COUNT };

	/**
	 * This method returns fixed array size
	 */
	inline size_t	capacity() const
	{
		return ARRAY_SIZE;
	}

protected:
	inline StaticStoragePolicy()
	{
	}

	/**
	 * Due to template issues in gcc all constructors have to be empty,
	 * initDefault does constructor work
	 */
	inline void initDefault( size_t initialSize )
	{
		size_ = initialSize;
		MF_ASSERT(size_ <= ARRAY_SIZE);
	}
	/**
	 * Static storage doesn't support runtime grow
	 */
	inline void	grow( size_t requestedSize )
	{
		MF_ASSERT( 0 );
	}

	size_t	size_;
	TYPE	data_[INITIAL_COUNT];
};

/**
 * This is a dynamic storage policy for ArrayBase.
 * It always allocates memory from heap and is very similar to BW::vector
 *
 * Space overhead: 2 * size_t and one pointer
 */
template <typename TYPE, size_t COUNT = 0>
class DynamicStoragePolicy
{
public:
	/**
	 * This method returns storage capacity
	 */
	inline size_t	capacity() const
	{
		return capacity_;
	}

	/**
	 * This method reserves memory for requested capacity
	 * allocating memory from heap if necessary
	 */
	inline void	reserve( size_t newCapacity )
	{
		if (newCapacity <= this->capacity())
		{
			// no need to allocate from heap, we are fine
			return;
		}
		TYPE* newArray = this->allocateNewArrayAndCopyData( newCapacity );
		this->freeMem();
		data_ = newArray;
		capacity_ = newCapacity;
	}

protected:
	inline DynamicStoragePolicy() :
		data_( NULL )
	{
	}

	inline ~DynamicStoragePolicy()
	{
		freeMem();
		capacity_ = 0;
		size_ = 0;
	}

	/**
	 * Due to template issues in gcc all constructors have to be empty,
	 * initDefault does constructor work
	 */
	inline void initDefault( size_t initialSize )
	{
		capacity_ = initialSize;
		size_ = initialSize;
		if (initialSize > 0)
		{
			data_ = new TYPE[initialSize];
		}
		else
		{
			data_ = NULL;
		}
	}

	/**
	 * This internal method allocates memory from heap and copies existing data
	 * to the allocated array
	 */
	inline TYPE*	allocateNewArrayAndCopyData( size_t newCapacity )
	{
		TYPE* newArray = new TYPE[newCapacity];

		// copy data from the old array to the new one
		TYPE* newPtr = newArray;
		TYPE* oldPtr = data_;
		TYPE* oldEndPtr = data_ + size_;

		while (oldPtr != oldEndPtr)
		{
			*newPtr++ = *oldPtr++;
		}
		return newArray;
	}

	/**
	 * Calculate grow size using factor 1.5 if possible
	 */
	inline size_t calcGrowSize( size_t requestedSize )
	{
		// current grow strategy is to increase capacity by 1.5 factor
		// or grow to requested size if 1.5 factor is not enough
		return std::max(requestedSize, size_ + size_ / 2 );
	}

	/**
	 * Grow array allocating memory from heap
	 */
	inline void	grow( size_t requestedSize )
	{
		this->reserve( this->calcGrowSize( requestedSize ) );
	}
	/**
	 * Free allocated memory
	 */
	inline void	freeMem()
	{
		if (data_)
		{
			bw_safe_delete_array( data_ );
		}
	}

	size_t	size_;
	TYPE*	data_;
	size_t	capacity_;
};
/**
 * This is a embedded dynamic storage policy for ArrayBase.
 * It provides a fixed size array to work with,
 * and allocates memory from heap if array has to grow
 *
 * Space overhead:  2 size_t and 1 pointer + embedded space
 * Possible runtime allocations if run out of given fixed size.
 *
 * Note: It's possible to reduce space overhead by storing capacity
 *       in fixedSize array using union if we are allocated memory from heap.
 *       This introduces a restriction on TYPE, with the current version
 *       of C++ standard it's impossible to store objects with non-trivial
 *       constructors in union.
 *       This means if space optimisation is done, it will only be possible
 *       to use POD types. If we do this optimisation we can also switch from
 *       using operator new to direct bw_malloc and bw_realloc calls
 */
template< typename TYPE, size_t INITIAL_COUNT>
class DynamicEmbeddedStoragePolicy : public DynamicStoragePolicy<TYPE>
{
public:
	/**
	 * This method reserves memory for requested capacity
	 * allocating memory from heap if necessary
	 */
	inline void	reserve( size_t newCapacity )
	{
		if (newCapacity <= this->capacity())
		{
			// no need to allocate from heap, we are fine
			return;
		}

		TYPE* newArray = this->allocateNewArrayAndCopyData( newCapacity );
		this->freeMem();
		this->data_ = newArray;
		this->capacity_ = newCapacity;
	}

protected:
	inline DynamicEmbeddedStoragePolicy()
	{
	}

	inline ~DynamicEmbeddedStoragePolicy()
	{
		this->freeMem();
		this->capacity_ = 0;
		this->size_ = 0;
	}

	/**
	 * Due to template issues in gcc all constructors have to be empty,
	 * initDefault does constructor work
	 */
	inline void initDefault( size_t initialSize )
	{
		this->capacity_ = INITIAL_COUNT;
		this->data_ = fixedArray_;
		MF_ASSERT( initialSize <= INITIAL_COUNT );
		this->size_ = initialSize;
	}

	/**
	 * Free allocated memory
	 */
	inline void	freeMem()
	{
		if (this->data_ != fixedArray_)
		{
			bw_safe_delete_array( this->data_ );
		}
		else
		{
			this->data_ = NULL;
		}
	}
	/**
	 * Grow array allocating memory from heap if necessary
	 */
	inline void	grow( size_t requestedSize )
	{
		this->reserve( this->calcGrowSize( requestedSize ) );
	}

private:
	TYPE	fixedArray_[INITIAL_COUNT];
};

/**
 * This is a storage policy for ArrayBase
 * It behaves the same way as ExpandableEmbeddedStoragePolicy, the only difference is
 * it displays warning on each heap allocation
 * This is to help tweak INITIAL_COUNT values for ExpandableEmbeddedStoragePolicy
 * to avoid unnecessary heap allocations in runtime.
 * Heap allocation suppose to be non-frequent operation, just to keep
 * program going instead of crashing hard. This is especially important for the server.
 *
 */
template< typename TYPE, size_t INITIAL_COUNT>
class DynamicEmbeddedStoragePolicyWithWarning :
	public DynamicEmbeddedStoragePolicy<TYPE, INITIAL_COUNT>
{
public:
	/**
	 * Grow array allocating memory from heap if necessary
	 * Display runtime warning when growing
	 * to help tweak INITIAL_COUNT for better performance
	 */
	inline void	grow( size_t requestedSize )
	{
		// current grow strategy is to increase capacity by 1.5 factor
		// or grow to requested size if 1.5 factor is not enough
		size_t reserveSize = this->calcGrowSize( requestedSize );

		if (this->capacity() < reserveSize)
		{
			char callstack[2048];
			callstack[0] = '\0';
#if ENABLE_STACK_TRACKER
			// grab callstack
			StackTracker::buildReport( callstack, ARRAY_SIZE(callstack) );
#endif // ENABLE_STACK_TRACKER
			// display warning on grow
			WARNING_MSG("Expanding embedded array with the initial COUNT %"
					PRIzu " to %" PRIzu " in\n\"%s\"\n"
					"Consider increasing INITIAL_COUNT to improve runtime "
					"performance\n",
				INITIAL_COUNT, reserveSize, callstack );
		}

		this->reserve( reserveSize );
	}

protected:
	/**
	 * Due to template issues in gcc all constructors have to be empty,
	 * initDefault does constructor work
	 */
	inline DynamicEmbeddedStoragePolicyWithWarning()
	{
	}
};


/**
 *  Storage policy which maps an externally allocated memory block 
 *  as the static array. This does not allow resizing.
 */
template< typename TYPE, size_t COUNT=0>
class ExternalStoragePolicy
{
public:	
	size_t capacity() const
	{
		return capacity_;
	}

protected:
	inline ExternalStoragePolicy()
	{
	}

	void init( TYPE* data, size_t capacity, size_t size )
	{
		MF_ASSERT( size <= capacity );
		size_ = size;
		capacity_ = capacity;
		data_ = data;
	}

	void grow( size_t requestedSize )
	{
		// Size is fixed.
		MF_ASSERT( 0 );
	}

	size_t	size_;
	size_t capacity_;
	TYPE*	data_;
};


/**
 * This is a simple constant size array
 * It supports two storage policies.
 * - PureStatic storage is just a checked wrapper around a C style array
 * - ExpandableEmbeddedStorage storage uses embedded fixed size array,
 *   but can grow in runtime if required
 *
 * It also provides quick way to remove an element from array
 * by swapping it with the last one. This is very fast and useful operation
 * if you don't need to preserve element's order
 *
 * Range checking is only done in DEBUG configurations.
 *
 * This container can also replace BW::set for small sets of data,
 * when using linear search is acceptable or even faster
 * and provide an extra benefit of completely avoiding runtime memory allocations.
 *
 * See find(...) and add_unique(...) methods.
 *
 * Example usage: 
 *	See unit tests.
 */
template
< 
	typename TYPE,
	size_t COUNT,
	template <typename, size_t> class StoragePolicy
>
class ArrayBase : public StoragePolicy<TYPE, COUNT>
{
public:
	typedef TYPE ElementType ;
	typedef TYPE value_type;
	typedef TYPE* iterator;
	typedef const TYPE * const_iterator;
	typedef TYPE& reference;
	typedef const TYPE& const_reference;

	/**
	 * Due to template issues in gcc all constructors have to be empty,
	 * initDefault does constructor work
	 */
	ArrayBase()
	{
	}

	/**
	 * These methods provide indexed access.
	 */
	inline ElementType& operator[] (const size_t i )
	{
		MF_ASSERT_DEBUG( i < this->size_ );
		return this->data_[i];
	}

	inline const ElementType& operator[] (const size_t i ) const
	{
		MF_ASSERT_DEBUG( i < this->size_ );
		return this->data_[i];
	}

	/**
	 * This method returns the number of elements in this array.
	 */
	inline size_t size() const
	{
		return this->size_;
	}

	/**
	 * This method changes the size
	 */
	inline void resize( size_t size )
	{
		if (this->capacity() < size)
		{
			this->grow( size );
		}
		this->size_ = size;
	}

	/**
	 * This method returns if the array is empty
	 */
	inline bool empty() const
	{
		return this->size_ == 0;
	}

	/**
	 * This method appends item to the array
	 */
	inline void push_back( const TYPE& t )
	{
		if (this->capacity() == this->size_)
		{
			this->grow( this->size_ + 1 );
		}

		this->data_[ this->size_ ] = t;

		++this->size_;
	}
	/**
	 * This method appends an empty item to the array
	 * and returns reference to it
	 */
	inline ElementType& push_back( )
	{
		if (this->capacity() == this->size_)
		{
			this->grow( this->size_ + 1 );
		}

		return this->data_[ this->size_++ ];
	}

	/**
	 * This method removes the last item from the array
	 */
	inline void pop_back()
	{
		if (this->size_ > 0)
		{
			--this->size_;
		}
	}

	/**
	 * This method returns the last item in the array
	 */
	inline const TYPE& back() const
	{
		MF_ASSERT_DEBUG( this->size_ != 0 );

		return this->data_[ this->size_ - 1 ];
	}

	/**
	 * This method fills all elements of this array with a given value.
	 */
	inline void assign( ElementType assignValue )
	{
		for ( size_t i = 0; i < this->size_; i++ )
		{
			this->data_[i] = assignValue;
		}
	}

	/**
	 * This method clears the array
	 */
	inline void	clear()
	{
		this->size_ = 0;
	}

	/**
	 * These 2 methods removes an element from the array
	 * using swap-with-last trick
	 * Note: use it if you don't need to preserve element's order
	 * This is O(1) operation
	 */
	inline void	erase_swap_last( size_t index )
	{
		MF_ASSERT_DEBUG( index < this->size_ );
		this->data_[index] = this->data_[--this->size_];
	}

	inline void	erase_by_iterator( iterator it )
	{
		MF_ASSERT_DEBUG( it >= begin() && it < end());

		(*it) = this->data_[--this->size_];
	}
	/**
	 * This method searches for the element and then removes it
	 * from the array using swap-with-last trick
	 * Note: use it if you don't need to preserve element's order
	 * Have to run search, which makes this operation O(n)
	 */
	inline void	erase_by_value( const TYPE& val )
	{
		iterator it = this->find( val );
		if (it != this->end())
		{
			this->erase_by_iterator( it );
		}
	}

	/**
	 * Erase item, preserving order. Returns iterator to
	 * the next item in the array.
	 */
	inline iterator	erase( iterator it )
	{
		MF_ASSERT_DEBUG( it >= begin() && it < end() );
		iterator p = it;
		iterator last = end()-1;
		while (p < last)
		{
			*p = *(p+1);
			++p;
		}
		--this->size_;
		return it;
	}

	/**
	 * This methods finds an element in array using linear search
	 * and returns iterator to it.
	 * If nothing is found, it returns end() iterator
	 * This is O(n) operation
	 */
	inline iterator find( const TYPE& val )
	{
		iterator it = this->begin();
		iterator endIt = this->end();
		while (it != endIt)
		{
			if ((*it) == val)
			{
				return it;
			}
			++it;
		}
		return this->end();
	}

	/**
	 * This methods appends element to array if it's not in thea array already.
	 * Returns iterator to inserted element if it was successfully inserted
	 * and end() otherwise.
	 * This is O(n) operation
	 */
	inline iterator add_unique( const TYPE& val )
	{
		if (find( val) == this->end())
		{
			this->push_back( val );
			return this->end() - 1;
		}
		return this->end();
	}

	/**
	 * This method provides direct access to the array
	 */
	inline TYPE*	data()
	{ 
		return this->data_;
	}

	/**
	 * Iterator methods
	 */
	inline iterator begin()
	{
		return this->data_;
	}
	inline const_iterator begin() const
	{
		return this->data_;
	}

	inline
	typename ArrayBase<TYPE, COUNT, StoragePolicy>::iterator end() 
	{
		return this->data_ + this->size_;
	}

	inline
	typename ArrayBase<TYPE, COUNT, StoragePolicy>::const_iterator end() const 
	{
		return this->data_ + this->size_;
	}
};

/**
 * C style fixed size static array wrapped to behave similarly to BW::vector
 * Neven allocates from heap
 * Default constructor behavior is identical to BW::vector 
 *  i.e. construct an empty container, with no elements and a size of zero
 * StaticArray( size_t ) constructor initialises array with the size n elements
 * Note that n can't be bigger than template COUNT parameter
 */
template < typename TYPE, size_t COUNT >
class StaticArray :
	public ArrayBase<TYPE, COUNT, StaticStoragePolicy >
{
public:
	/**
	 * Due to template issues in gcc all constructors have to be empty,
	 * initDefault does constructor work
	 */
	StaticArray( size_t initialSize = 0 )
	{
		this->initDefault( initialSize );
	}
};

/**
 * Expandable static array
 * User provides initial size of embedded array as a template parameter,
 * if array has to grow it starts allocating memory from heap
 *
 * Default constructor behavior is identical to BW::vector 
 *  i.e. construct an empty container, with no elements and a size of zero
 * DynamicArray( size_t n ) constructor initialises array with the size n elements
 * Note that n can't be bigger than template COUNT parameter
 *
 * See ExpandableStaticArrayWithWarning below if you need to tweak COUNT values
 * for best runtime performance
 */
template < typename TYPE, size_t COUNT> 
class DynamicEmbeddedArray :
	public ArrayBase<TYPE, COUNT, DynamicEmbeddedStoragePolicy >
{
public:
	/**
	 * Due to template issues in gcc all constructors have to be empty,
	 * initDefault does constructor work
	 */
	DynamicEmbeddedArray( size_t initialSize = 0 )
	{
		this->initDefault( initialSize );
	}
};

/**
 * Template specialization for the dynamic array which displays warning
 * on each heap allocation.
 * This is to help tweak INITIAL_COUNT values for ExpandableEmbeddedStoragePolicy
 * to avoid unnecessary heap allocations in runtime.
 *
 * Default constructor behavior is identical to BW::vector 
 *  i.e. construct an empty container, with no elements and a size of zero
 * DynamicArray( size_t n ) constructor initialises array with the size n elements
 * Note that n can't be bigger than template COUNT parameter
 *
 */
template < typename TYPE, size_t COUNT> 
class DynamicEmbeddedArrayWithWarning :
	public ArrayBase<TYPE, COUNT,DynamicEmbeddedStoragePolicyWithWarning >
{
public:
	/**
	 * Due to template issues in gcc all constructors have to be empty,
	 * initDefault does constructor work
	 */
	DynamicEmbeddedArrayWithWarning( size_t initialSize = 0 )
	{
		this->initDefault( initialSize );
	}
};

/**
 * Pure dynamic array, very similar to BW::vector
 * Always allocates memory from heap
 *
 * Default constructor behavior is identical to BW::vector 
 *  i.e. construct an empty container, with no elements and a size of zero
 * DynamicArray( size_t n ) constructor initialises array with the size n elements
 */
template < typename TYPE > 
class DynamicArray :
	public ArrayBase<TYPE, 0, DynamicStoragePolicy >
{
public:
	/**
	 * Due to template issues in gcc all constructors have to be empty,
	 * initDefault does constructor work
	 */
	DynamicArray( size_t initialSize = 0 )
	{
		this->initDefault( initialSize );
	}
};

/**
 * Array initialized using externally allocated memory.
 * 
 * The default constructor and reset() will initialize to a NULL pointer,
 * so use valid() to check if array is pointing to valid data.
 *
 * The assign() function can be used to reseat to point to different
 * external data.
 *
 */
template < typename TYPE > 
class ExternalArray :
	public ArrayBase<TYPE, 0, ExternalStoragePolicy>
{
public:
	ExternalArray()
	{
		this->init( NULL, 0, 0 );
	}

	ExternalArray( TYPE* p, size_t capacity, size_t size )
	{
		this->init( p, capacity, size );
	}

	void reset()
	{
		this->init( NULL, 0, 0 );
	}

	void assignExternalStorage( TYPE* p,
			size_t capacity, size_t size )
	{
		this->init( p, capacity, size );
	}

	bool valid() const
	{
		return this->data_ != NULL;
	}
};

template < typename IndexType >
class IndexSpan
{
	IndexType first_,
	          last_;
public:
	typedef IndexType value_type;

	IndexType first() const { return first_; }
	void first( IndexType idx ) { first_ = idx; }

	IndexType last() const { return last_; }
	void last( IndexType idx ) { last_ = idx; }

	IndexType range() const { return last_ - first_ + 1; }
};

BW_END_NAMESPACE


#endif
