#pragma once
#ifndef CIRCULAR_QUEUE_HPP
#define CIRCULAR_QUEUE_HPP

#include "cstdmf/guard.hpp"
#include "cstdmf/static_array.hpp"
#include "cstdmf/bw_namespace.hpp"

#include "debug.hpp"

#include <iterator>
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE


namespace
{
// Compile-time ?: from
// http://www.drdobbs.com/the-standard-librarian-defining-iterato/184401331?pgno=3
template <bool flag, class IsTrue, class IsFalse>
struct choose;

template <class IsTrue, class IsFalse>
struct choose<true, IsTrue, IsFalse>
{
	typedef IsTrue type;
};

template <class IsTrue, class IsFalse>
struct choose<false, IsTrue, IsFalse>
{
	typedef IsFalse type;
};
}


/**
 *	A Circular Queue for POD types which may be supplied with its own container.
 *	
 *	As this is a queue, it's worth noting that its iterators deal with queue
 *	positions, not specific items in the queue, so iterators are never
 *	invalidated except that when pop() occurs, end()-1 becomes end() and end()
 *	is invalidated.
 *	
 *	Note that element destruction may not happen immediately, as the container
 *	may not know that we are no longer interested in an element until it is
 *	overwritten.
 */
template<class T, class CONTAINER = BW::vector<T> >
class CircularQueue
{
public:
	typedef CircularQueue<T, CONTAINER> This;

	CircularQueue();
	CircularQueue( size_t size );

	void resize( size_t size );
	void clear();

	T & push();
	T & push( const T & v );

	void pop();

	T & peek();
	const T & peek() const;

	T & peek_back();
	const T & peek_back() const;

	size_t size() const;

	size_t capacity() const;

	T & operator[]( size_t p );
	const T & operator[]( size_t p ) const;

	bool full()  const;
	bool empty() const;

	template <bool ISCONST = false>
	struct IteratorBase :
		public std::iterator<std::random_access_iterator_tag,
								typename choose<ISCONST, const T, T>::type >
	{
		typedef typename IteratorBase::reference reference;
		typedef typename IteratorBase::pointer pointer;
		typedef typename IteratorBase::difference_type difference_type;

		typedef typename choose<ISCONST, const This, This>::type owner_type;

		// For CircularQueue use only
		IteratorBase( owner_type & owner, size_t position );

		// Random-access iterator:

		// Default-constructible
		IteratorBase();

		// Copyable and copy-constructable
		// This also takes care of converting an iterator into a const_iterator
		friend struct IteratorBase<true>;
		IteratorBase( const IteratorBase<false> & other );

		// This and later member operators are inlined because I can't work out
		// how to get gcc 4.1.2 and VS2008 to both compile them.
		/**
		 *	Assignment operator
		 */
		IteratorBase & operator=( IteratorBase other )
		{
			swap(*this, other);
			return *this;
		}

		// Equality and inequality operators
		// All friend functions are inlined to make the compiler generate
		// non-template functions so that user-defined conversions work.
		// This allows comparing const and non-const iterators.
		/**
		 *	Equality operator
		 */
		friend inline bool operator==( const IteratorBase & lhs,
			const IteratorBase & rhs )
		{
			MF_ASSERT( lhs.pOwner_ == rhs.pOwner_ );
			return (lhs.pOwner_ == rhs.pOwner_) && (lhs.position_ == rhs.position_);
		}

		/**
		 *	Inequality operator
		 */
		friend inline bool operator!=( const IteratorBase & lhs,
			const IteratorBase & rhs )
		{
			return !(lhs == rhs);
		}

		// Can be dereferenced
		/**
		 *	Dereference operator
		 */
		reference operator*() const { return (*pOwner_)[ position_ ]; }

		/**
		 *	Dereference operator
		 */
		pointer operator->() const { return &(*this); };

		// Can be incremented and decremented
		/**
		 *	Pre-increment operator
		 */
		IteratorBase & operator++()
		{
			++position_;
			return *this;
		}

		/**
		 *	Post-increment operator
		 */
		IteratorBase operator++( int )
		{
			IteratorBase tmp( *this );
			++(*this);
			return tmp;
		}

		/**
		 *	Pre-decrement operator
		 */
		IteratorBase & operator--()
		{
			--position_;
			return *this;
		}

		/**
		 *	Post-decrement operator
		 */
		IteratorBase operator--( int )
		{
			IteratorBase tmp( *this );
			--(*this);
			return tmp;
		}

		// Arithmetic with integers, and difference of two iterators
		/**
		 *	Arithmetic + operator with integer value
		 */
		friend inline IteratorBase operator+( IteratorBase lhs,
			difference_type rhs )
		{
			lhs += rhs;
			return lhs;
		}

		/**
		 *	Arithmetic - operator with integer value
		 */
		friend inline IteratorBase operator-( IteratorBase lhs,
			difference_type rhs )
		{
			lhs -= rhs;
			return lhs;
		}

		/**
		 *	Arithmetic - operator of two IteratorBases
		 */
		friend inline difference_type operator-(
			const IteratorBase & lhs, const IteratorBase & rhs )
		{
			MF_ASSERT( lhs.pOwner_ == rhs.pOwner_ );
			return lhs.position_ - rhs.position_;
		}

		// Inequality comparisons
		/**
		 *	Inequality comparison operator <
		 */
		friend inline bool operator<( const IteratorBase & lhs,
			const IteratorBase & rhs )
		{
			MF_ASSERT( lhs.pOwner_ == rhs.pOwner_ );
			return (lhs.position_ < rhs.position_);
		}

		/**
		 *	Inequality comparison operator >
		 */
		friend inline bool operator>( const IteratorBase & lhs,
			const IteratorBase & rhs )
		{
			return (rhs < lhs);
		}

		/**
		 *	Inequality comparison operator <=
		 */
		friend inline bool operator<=( const IteratorBase & lhs,
			const IteratorBase & rhs )
		{
			return !(lhs > rhs);
		}

		/**
		 *	Inequality comparison operator >=
		 */
		friend inline bool operator>=( const IteratorBase & lhs,
			const IteratorBase & rhs )
		{
			return !(lhs < rhs);
		}


		// Compound assignment operators
		/**
		 *	Compound assignment and addition operator
		 */
		IteratorBase & operator+=( difference_type rhs )
		{
			position_ += rhs;
			return *this;
		}

		/**
		 *	Compound assignment and subtraction operator
		 */
		IteratorBase & operator-=( difference_type rhs )
		{
			position_ -= rhs;
			return *this;
		}

		// Offset dereference operator
		/**
		 *	Offset dereference operator
		 */
		reference operator[]( size_t idx ) const
		{
			return *(this + idx);
		}

		/**
		 *	This function swaps the values of two IteratorBases
		 */
		friend inline void swap( IteratorBase & lhs, IteratorBase & rhs )
		{
			std::swap( lhs.pOwner_, rhs.pOwner_ );
			std::swap( lhs.position_, rhs.position_ );
		}

	private:
		owner_type * pOwner_;
		size_t position_;
	};

	typedef IteratorBase<> iterator;
	typedef IteratorBase<true> const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	iterator begin();
	const_iterator begin() const;

	iterator end();
	const_iterator end() const;

	reverse_iterator rbegin();
	const_reverse_iterator rbegin() const;

	reverse_iterator rend();
	const_reverse_iterator rend() const;

private:
	CONTAINER container_;

	size_t start_;
	size_t count_;
};

// Template method implementations
#include "circular_queue.ipp"

BW_END_NAMESPACE

#endif // CIRCULAR_QUEUE_HPP
