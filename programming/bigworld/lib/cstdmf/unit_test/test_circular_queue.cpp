#include "pch.hpp"

#include "cstdmf/circular_queue.hpp"

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/static_array.hpp"

#include <numeric>


BW_BEGIN_NAMESPACE

struct RefCounted : public ReferenceCount {};
typedef SmartPointer< RefCounted > RefPtr;


// Very basic CircularQueue test
TEST( CircularQueue_testEmpty )
{
	CircularQueue< int >	intCQueue;

	CHECK_EQUAL( 0U, intCQueue.size() );
	CHECK_EQUAL( 0U, intCQueue.capacity() );
	CHECK_EQUAL( true, intCQueue.empty() );
	CHECK_EQUAL( true, intCQueue.full() );
}


// cqTest are the tests that came with the original version received from Martin
TEST( CircularQueue_cqTest_Resizeable )
{
	CircularQueue< int > buf;

	buf.resize( 10 );

	CHECK_EQUAL( 10U, buf.capacity() );
	buf.push( 1 );
	CHECK_EQUAL( 1U, buf.size() );
	CHECK_EQUAL( 1, buf.peek() );
	CHECK_EQUAL( 1, *buf.begin() );
	CHECK( buf.begin() != buf.end() );

	buf.pop();
	CHECK_EQUAL( 0U, buf.size() );

	buf.resize( 3 );
	CHECK_EQUAL( 3U, buf.capacity() );
	CHECK_EQUAL( 0U, buf.size() );
	CHECK( buf.begin() == buf.end() );

	buf.push( 0 );
	buf.push( 1 );

	CHECK_EQUAL( 1, buf.peek_back() );

	buf.pop();

	CHECK_EQUAL( 1U, buf.size() );
	CHECK_EQUAL( 1, *buf.begin() );

	buf.push( 2 );
	buf.push( 3 );

	buf.pop();
	buf.pop();
	buf.pop();

	CHECK_EQUAL( 0U, buf.size() );

	buf.push( 4 );

	CHECK_EQUAL( 4, buf.peek() );
}


// The cqTest version used a different fixed-size array class
TEST( CircularQueue_cqTest_StaticArray )
{
	CircularQueue< int, StaticArray< int, 4 > > buf;

	buf.resize( 4 );

	CHECK_EQUAL( 4U, buf.capacity() );
	buf.push( 1 );
	CHECK_EQUAL( 1U, buf.size() );
	CHECK_EQUAL( 1, buf.peek() );
	CHECK_EQUAL( 1, *buf.begin() );
	CHECK( buf.begin() != buf.end() );

	buf.pop();
	CHECK_EQUAL( 0U, buf.size() );

	buf.clear();
	CHECK_EQUAL( 0U, buf.size() );
	CHECK( buf.begin() == buf.end() );

	buf.push( 0 );
	buf.push( 1 );

	CHECK_EQUAL( 1, buf.peek_back() );

	buf.pop();

	CHECK_EQUAL( 1U, buf.size() );
	CHECK_EQUAL( 1, *buf.begin() );

	buf.push( 2 );
	buf.push( 3 );

	buf.pop();
	buf.pop();
	buf.pop();

	CHECK_EQUAL( 0U, buf.size() );

	buf.push( 4 );

	CHECK_EQUAL( 4, buf.peek() );
}


// peek_back was broken in the code originally received
TEST( CircularQueue_testWrapping )
{
	CircularQueue< int >	intCQueue( 4 );

	CHECK_EQUAL( 0U, intCQueue.size() );
	CHECK_EQUAL( 4U, intCQueue.capacity() );
	CHECK_EQUAL( true, intCQueue.empty() );
	CHECK_EQUAL( false, intCQueue.full() );

	int i = 0;
	while (!intCQueue.full())
	{
		intCQueue.push( i++ );
	}

	CHECK_EQUAL( 4, i );

	CHECK_EQUAL( 4U, intCQueue.size() );
	CHECK_EQUAL( false, intCQueue.empty() );
	CHECK_EQUAL( true, intCQueue.full() );

	CHECK_EQUAL( 0, intCQueue.peek() );
	CHECK_EQUAL( 3, intCQueue.peek_back() );

	intCQueue.pop();

	CHECK_EQUAL( 3U, intCQueue.size() );
	CHECK_EQUAL( false, intCQueue.empty() );
	CHECK_EQUAL( false, intCQueue.full() );

	CHECK_EQUAL( 1, intCQueue.peek() );
	CHECK_EQUAL( 3, intCQueue.peek_back() );

	intCQueue.push( i++ );

	CHECK_EQUAL( 4U, intCQueue.size() );
	CHECK_EQUAL( false, intCQueue.empty() );
	CHECK_EQUAL( true, intCQueue.full() );

	CHECK_EQUAL( 1, intCQueue.peek() );
	CHECK_EQUAL( 4, intCQueue.peek_back() );
}


// This is from the synopsis of boost::circular_buffer
// and highlights differences in operation...
TEST( CircularQueue_test_boost_circular_buffer_synopsis )
{
	CircularQueue< int > cb( 3 );
	// boost::circular_buffer calls this "push_back"
	cb.push( 1 );
	cb.push( 2 );
	cb.push( 3 );

	CHECK_EQUAL( 1, cb[ 0 ] );
	CHECK_EQUAL( 2, cb[ 1 ] );
	CHECK_EQUAL( 3, cb[ 2 ] );

	CHECK_EQUAL( true, cb.full() );

	// boost::circular_buffer::push_back implies this test
	if (cb.full())
	{
		cb.pop();
	}
	cb.push( 4 );

	if (cb.full())
	{
		cb.pop();
	}
	cb.push( 5 );

	CHECK_EQUAL( 3, cb[ 0 ] );
	CHECK_EQUAL( 4, cb[ 1 ] );
	CHECK_EQUAL( 5, cb[ 2 ] );

	// There's no equivalent to boost::circular_buffer::pop_back()

	// boost::circular_buffer calls this pop_front()
	cb.pop();

	CHECK_EQUAL( 4, cb[ 0 ] );
}


// Test moving back-and-forth between forward and reverse iterators
TEST( CircularQueue_iterator_reversion_and_back )
{
	typedef CircularQueue< int > IntCQueue;
	IntCQueue intCQueue( 3 );

	intCQueue.push( 0 );
	intCQueue.push( 1 );
	intCQueue.push( 2 );

	CHECK(
		IntCQueue::reverse_iterator( intCQueue.begin() ) == intCQueue.rend() );
	CHECK(
		IntCQueue::reverse_iterator( intCQueue.end() ) == intCQueue.rbegin() );

	CHECK( intCQueue.rbegin().base() == intCQueue.end() );
	CHECK( intCQueue.rend().base() == intCQueue.begin() );

	IntCQueue::iterator iInt;

	iInt = intCQueue.begin();
	iInt++;

	CHECK_EQUAL( 1, *iInt );

	IntCQueue::reverse_iterator riInt( iInt );
	riInt--;

	CHECK_EQUAL( 1, *riInt );

	iInt = riInt.base();
	iInt--;

	CHECK_EQUAL( 1, *iInt );
}


// Run the forward and reverse iterators up and down the queue
TEST( CircularQueue_iterator_up_and_down )
{
	typedef CircularQueue< int > IntCQueue;
	IntCQueue intCQueue( 3 );

	intCQueue.push( 0 );
	intCQueue.push( 1 );
	intCQueue.push( 2 );

	IntCQueue::iterator iInt;
	int value;

	for (iInt = intCQueue.begin(), value = 0;
		iInt != intCQueue.end(); ++iInt, ++value )
	{
		CHECK_EQUAL( value, *iInt );
	}

	CHECK( iInt == intCQueue.end() );
	CHECK_EQUAL( 3, value );

	while( iInt-- != intCQueue.begin() )
	{
		CHECK_EQUAL( --value, *iInt );
	}

	CHECK_EQUAL( 0, value );
}


TEST( CircularQueue_reverse_iterator_up_and_down )
{
	typedef CircularQueue< int > IntCQueue;
	IntCQueue intCQueue( 3 );

	intCQueue.push( 2 );
	intCQueue.push( 1 );
	intCQueue.push( 0 );

	IntCQueue::reverse_iterator iInt;
	int value;

	for (iInt = intCQueue.rbegin(), value = 0;
		iInt != intCQueue.rend(); ++iInt, ++value )
	{
		CHECK_EQUAL( value, *iInt );
	}

	CHECK( iInt == intCQueue.rend() );
	CHECK_EQUAL( 3, value );

	while( iInt-- != intCQueue.rbegin() )
	{
		CHECK_EQUAL( --value, *iInt );
	}

	CHECK_EQUAL( 0, value );

}


// Repeat the last two, but from a const CircularQueue
TEST( CircularQueue_iterator_up_and_down_from_const )
{
	typedef CircularQueue< int > IntCQueue;
	IntCQueue intCQueue( 3 );

	intCQueue.push( 0 );
	intCQueue.push( 1 );
	intCQueue.push( 2 );

	const IntCQueue & constIntCQueue = intCQueue;

	IntCQueue::const_iterator iInt;
	int value;

	for (iInt = constIntCQueue.begin(), value = 0;
		iInt != constIntCQueue.end(); ++iInt, ++value )
	{
		CHECK_EQUAL( value, *iInt );
	}

	CHECK( iInt == constIntCQueue.end() );
	CHECK_EQUAL( 3, value );

	while( iInt-- != constIntCQueue.begin() )
	{
		CHECK_EQUAL( --value, *iInt );
	}

	CHECK_EQUAL( 0, value );

}


TEST( CircularQueue_reverse_iterator_up_and_down_from_const )
{
	typedef CircularQueue< int > IntCQueue;
	IntCQueue intCQueue( 3 );

	intCQueue.push( 2 );
	intCQueue.push( 1 );
	intCQueue.push( 0 );

	const IntCQueue & constIntCQueue = intCQueue;

	IntCQueue::const_reverse_iterator iInt;
	int value;

	for (iInt = constIntCQueue.rbegin(), value = 0;
		iInt != constIntCQueue.rend(); ++iInt, ++value )
	{
		CHECK_EQUAL( value, *iInt );
	}

	CHECK( iInt == constIntCQueue.rend() );
	CHECK_EQUAL( 3, value );

	while( iInt-- != constIntCQueue.rbegin() )
	{
		CHECK_EQUAL( --value, *iInt );
	}

	CHECK_EQUAL( 0, value );

}


// Iterators are where we see that a queue is different from a buffer
// This is also from the boost::circular_buffer documentation
TEST( CircularQueue_iterator_invalidation_versus_boost_circular_buffer )
{
	CircularQueue< int > cb( 3 );

	cb.push( 1 );
	cb.push( 2 );
	cb.push( 3 );

	CircularQueue< int >::iterator it = cb.begin();

	CHECK_EQUAL( 1, *it );

	// This _should_ invalidate that iterator
	if (cb.full())
	{
		cb.pop();
	}
	cb.push( 4 );

	// boost::circular_buffer gives "4" for this, because the iterator still
	// points at the same time. CircularQueue points at the same _location_ in
	// the queue.
	CHECK_EQUAL( 2, *it );
}


// Another demonstration from boost::circular_buffer documentation
TEST( CircularQueue_boost_circular_buffer_accumulation_example )
{
	CircularQueue< int > cb( 3 );

	cb.push( 1 );
	cb.push( 2 );

	CHECK_EQUAL( 1, cb[ 0 ] );
	CHECK_EQUAL( 2, cb[ 1 ] );
	CHECK_EQUAL( false, cb.full() );
	CHECK_EQUAL( 2U, cb.size() );
	CHECK_EQUAL( 3U, cb.capacity() );

	cb.push( 3 );
	if (cb.full())
	{
		cb.pop();
	}
	cb.push( 4 );

	int sum = std::accumulate( cb.begin(), cb.end(), 0 );

	CHECK_EQUAL( 2, cb[ 0 ] );
	CHECK_EQUAL( 3, cb[ 1 ] );
	CHECK_EQUAL( 4, cb[ 2 ] );
	CHECK_EQUAL( 2, *cb.begin() );
	// This is front() in circular_buffer
	CHECK_EQUAL( 2, cb.peek() );
	// This is back() in circular_buffer
	CHECK_EQUAL( 4, cb.peek_back() );
	CHECK_EQUAL( 9, sum );
	CHECK_EQUAL( true, cb.full() );
	CHECK_EQUAL( 3U, cb.size() );
	CHECK_EQUAL( 3U, cb.capacity() );
}


// Unit test showing element destruction is delayed, which is why this is not
// safe for non-POD types
TEST( CircularQueue_element_destruction )
{
	typedef CircularQueue< RefPtr > RefPtrCQ;

	SmartPointer< RefCounted > pRef( new RefCounted() );
	RefPtrCQ refCQ( 3 );

	refCQ.push( pRef );

	CHECK_EQUAL( 2, pRef->refCount() );

	refCQ.pop();

	// This would be expected to be 1
	CHECK_EQUAL( 2, pRef->refCount() );

	refCQ.push( new RefCounted() );

	CHECK_EQUAL( 2, pRef->refCount() );

	refCQ.clear();

	CHECK_EQUAL( 2, pRef->refCount() );

	refCQ.push( new RefCounted() );

	CHECK_EQUAL( 1, pRef->refCount() );
}

// Test some of the deep C++ template magic that lets iterators and
// const_iterators lie together like lions and lambs
TEST( CircularQueue_iterators_and_const_iterators )
{
	typedef CircularQueue< int > IntCQueue;

	IntCQueue intCQueue(3);
	intCQueue.push( 1 );
	intCQueue.push( 2 );
	intCQueue.push( 3 );

	IntCQueue::const_iterator iConstBegin( intCQueue.begin() );
	IntCQueue::iterator iBegin( intCQueue.begin() );

	CHECK( iBegin == iBegin );
	CHECK( iConstBegin == iConstBegin );

	CHECK( iConstBegin == iBegin );
	CHECK( iBegin == iConstBegin );

	iConstBegin = iBegin;

	CHECK( iConstBegin == intCQueue.begin() );

	// This fails to compile, 'cause you're not allowed to do that.
	//iBegin = iConstBegin;

	typedef const CircularQueue< int > ConstIntCQueue;

	ConstIntCQueue & rCIntCQueue = intCQueue;

	ConstIntCQueue::const_iterator iCConstBegin = rCIntCQueue.begin();
	// This fails to compile, 'cause you're not allowed to do that.
	//ConstIntCQueue::iterator iCBegin = rCIntCQueue.begin();
}

// Iterators are for slots in the queue, so as you pop things, they are
// not invalidated, but the value they dereference to changes.
TEST( CircularQueue_iterators_are_slot_references_not_value_references )
{
	typedef CircularQueue< int > IntCQueue;

	IntCQueue intCQueue(3);
	intCQueue.push( 1 );
	intCQueue.push( 2 );
	intCQueue.push( 3 );

	IntCQueue::const_iterator iBegin( intCQueue.begin() );
	IntCQueue::const_iterator iLast( intCQueue.end() - 1 );
	IntCQueue::const_reverse_iterator iRBegin( intCQueue.rbegin() );
	IntCQueue::const_reverse_iterator iRLast( intCQueue.rend() - 1 );

	CHECK_EQUAL( 1, *iBegin );
	CHECK_EQUAL( 3, *iLast );
	CHECK_EQUAL( 1, *iRLast );
	CHECK_EQUAL( 3, *iRBegin );

	// Pop implicitly decrements all existing iterators, if you're thinking
	// of them as value-pointers.
	// If you're thinking of them as slot-pointers, then it just slides all the
	// values down by one.
	intCQueue.pop();

	// There's no named iterator for past-the-beginning
	CHECK( iRBegin == intCQueue.rbegin() - 1 );

	// This moves us back to rbegin(), and probably should be done _before_
	// the pop.
	iRBegin++;

	CHECK_EQUAL( 2, *iBegin );
	// end() is defined as a past-the-end iterator.
	CHECK( iLast == intCQueue.end() );
	CHECK_EQUAL( 2, *iRLast );
	CHECK_EQUAL( 3, *iRBegin );

	// This has the unusual semantic that pop() removes the first element, but
	// does not invalidate iterators that pointed to that element. In fact, no
	// iterators are "invalidated" but previously-dereferencable iterators
	// may no longer be.
}

BW_END_NAMESPACE

// test_circular_queue.cpp
