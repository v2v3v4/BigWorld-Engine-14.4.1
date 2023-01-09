#include "pch.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/concurrency.hpp"

#include <stdio.h>

#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Unit-test all our concurrency primitives:
 *
 *	DummyMutex (Tested as being broken...)
 *	OurThreadID
 *	SimpleMutex
 *	RecursiveMutex
 *	SimpleSemaphore
 *	ReadWriteLock
 *	SimpleThread
 *	THREADLOCAL(type)
 *	SimpleMutexHolder
 *	RecursiveMutexHolder
 *	BWConcurrency::startMainThreadIdling,
 *		endMainThreadIdling and setMainThreadIdleFunctions
 */

// Some non-threaded tests of basic functionality
// DummyMutex: Try and force a deadlock
TEST( Concurrency_DummyMutex_NoThreads )
{
	DummyMutex lock1;
	DummyMutex lock2;
	// Textbook how-not-to-lock-multiple-resources
	lock2.grab();
	lock1.grab();
	lock2.give();
	lock1.give();
}

// SimpleSemaphore: Test that pullTry returns what we expect
// See Bug 32434:
//  MF_SINGLE_THREADED SimpleSemaphore::pullTry has faulty return value
TEST( Concurrency_SimpleSemaphore_NoThreads_pullTry )
{
	SimpleSemaphore semaphore;
	CHECK_EQUAL( false, semaphore.pullTry() );
	semaphore.push();
	semaphore.push();
	semaphore.push();
	CHECK_EQUAL( true, semaphore.pullTry() );
	CHECK_EQUAL( true, semaphore.pullTry() );
	CHECK_EQUAL( true, semaphore.pullTry() );
	CHECK_EQUAL( false, semaphore.pullTry() );
}

// All the rest of our tests expect to be able to create SimpleThread instances
// XXX: The best kind of secs...
// TODO: Why isn't this defined by concurrency.hpp already?
#ifdef _WIN32
#define SLEEPSECS( x ) Sleep( x * 1000 )
#else
#define SLEEPSECS( x ) usleep( x * 1000000 )
#endif

// Check that a value does not change during a SLEEPSECS( 0 )
// Assumes SLEEPSECS( 0 ) gives up the scheduled timeslot
#define CHECK_NOT_CHANGING( var ) { \
	int first = var; \
	SLEEPSECS( 0 ); \
	CHECK_EQUAL( first, var ); \
	}

template <class ThreadArg>
class ConcurrencyTestWrapper
{
public:
	ConcurrencyTestWrapper<ThreadArg>() : threadIndex_( 0 ) {}

	~ConcurrencyTestWrapper<ThreadArg>()
	{
		MF_ASSERT( threads_.empty() );
	}

	ThreadArg data;

	// Run a new thread
	SimpleThread * startThread( const BW::string & name )
	{
		BW::stringstream stream;
		stream << name << " " << threadIndex_++;
		SimpleThread * thread =
			new SimpleThread( &threadStarter, this, stream.str() );
		MF_ASSERT( thread != NULL );
		threads_.push_back( thread );
		return thread;
	}

	// Block until this thread terminates
	void endThread( SimpleThread * thread )
	{
		ThreadList::iterator it =
			std::find( threads_.begin(), threads_.end(), thread );
		MF_ASSERT( it != threads_.end() );
		threads_.erase( it );
		bw_safe_delete(thread);
	}

	// Blocks until all threads terminate
	void endAllThreads()
	{
		// Delete-safe iteration
		ThreadList::iterator it = threads_.begin();
		while (it != threads_.end())
		{
			SimpleThread * thread = *(it++);
			endThread( thread );
		}

		MF_ASSERT( threads_.empty() );
	}

private:
	static void threadStarter( void * arg )
	{
		ConcurrencyTestWrapper<ThreadArg> * instance =
			reinterpret_cast<ConcurrencyTestWrapper<ThreadArg> *>( arg );
		instance->threadTask();
	}

	void threadTask();

	typedef BW::list< SimpleThread * > ThreadList;
	ThreadList threads_;

	int threadIndex_;

};

// SimpleThread: Test this first, as it's the basis of our
// other threading tests using ConcurrencyTestWrapper
struct Concurrency_SimpleThread_ThreadArg
{
	int i;

	Concurrency_SimpleThread_ThreadArg() : i( 0 ) {}
};

template <>
void ConcurrencyTestWrapper<Concurrency_SimpleThread_ThreadArg>::threadTask()
{
	data.i++;
}

TEST( Concurrency_SimpleThread )
{
	ConcurrencyTestWrapper<Concurrency_SimpleThread_ThreadArg> wrapper;
	SimpleThread* thread = wrapper.startThread( "Concurrency_SimpleThread" );
	wrapper.endThread( thread );

	CHECK_EQUAL( 1, wrapper.data.i );
}

// DummyMutex: Try and force a deadlock
struct Concurrency_DummyMutex_ThreadArg
{
	DummyMutex lock1;
	DummyMutex lock2;
};

template <>
void ConcurrencyTestWrapper<Concurrency_DummyMutex_ThreadArg>::threadTask()
{
	// Textbook how-not-to-lock-multiple-resources
	data.lock2.grab();
	data.lock1.grab();
	data.lock2.give();
	data.lock1.give();
}

TEST( Concurrency_DummyMutex )
{
	// Try and force a deadlock
	ConcurrencyTestWrapper<Concurrency_DummyMutex_ThreadArg> wrapper;
	wrapper.data.lock1.grab();
	for (int i = 0; i < 50; ++i)
		wrapper.startThread( "Concurrency_DummyMutex" );
	// Give the threads time to (theoretically) block on lock1
	SLEEPSECS( 1 );
	// If this were a real mutex, this call would block forever
	wrapper.data.lock2.grab();
	wrapper.data.lock1.give();
	wrapper.data.lock2.give();
	wrapper.endAllThreads();
}

// SimpleMutex: Simple test of locking
// Testing out-of-order, as we use SimpleMutex
// for actual thread safety after this
struct Concurrency_SimpleMutex_ThreadArg
{
	SimpleMutex lock;
	int value;

	Concurrency_SimpleMutex_ThreadArg() : value( 0 ) {}
};

template <>
void ConcurrencyTestWrapper<Concurrency_SimpleMutex_ThreadArg>::threadTask()
{
	data.lock.grab();
	data.value++;
	data.lock.give();
}

TEST( Concurrency_SimpleMutex )
{
	ConcurrencyTestWrapper<Concurrency_SimpleMutex_ThreadArg> wrapper;
	wrapper.data.lock.grab();
	for (int i = 0; i < 500; ++i)
		wrapper.startThread( "Concurrency_SimpleMutex" );
	CHECK_NOT_CHANGING( wrapper.data.value );
	CHECK_EQUAL( 0, wrapper.data.value );
	wrapper.data.lock.give();
	wrapper.endAllThreads();

	CHECK_EQUAL( 500, wrapper.data.value );
}

// RecursiveMutex: Simple test of locking
// Testing out-of-order, as we use RecursiveMutex
// for actual thread safety after this
struct Concurrency_RecursiveMutex_ThreadArg
{
	RecursiveMutex lock;
	int value;

	Concurrency_RecursiveMutex_ThreadArg() : value( 0 ) {}
};

template <>
void ConcurrencyTestWrapper<Concurrency_RecursiveMutex_ThreadArg>::threadTask()
{
	data.lock.grab();
	data.lock.grab();
	data.value++;
	data.lock.give();
	data.lock.give();
}

TEST( Concurrency_RecursiveMutex )
{
	ConcurrencyTestWrapper<Concurrency_RecursiveMutex_ThreadArg> wrapper;
	CHECK_EQUAL( 0, wrapper.data.lock.lockCount() );
	wrapper.data.lock.grab();
	CHECK_EQUAL( 1, wrapper.data.lock.lockCount() );
	wrapper.data.lock.grab();
	CHECK_EQUAL( 2, wrapper.data.lock.lockCount() );
	for (int i = 0; i < 500; ++i)
		wrapper.startThread( "Concurrency_RecursiveMutex" );
	CHECK_NOT_CHANGING( wrapper.data.value );
	CHECK_EQUAL( 0, wrapper.data.value );
	wrapper.data.lock.give();
	CHECK_EQUAL( 0, wrapper.data.value );
	CHECK_EQUAL( 1, wrapper.data.lock.lockCount() );
	wrapper.data.lock.give();
	wrapper.endAllThreads();
	// Check after end of threads.
	CHECK_EQUAL( 0, wrapper.data.lock.lockCount() );

	CHECK_EQUAL( 500, wrapper.data.value );
}

// OurThreadID: Check that there's no repeated thread IDs
struct Concurrency_OurThreadID_ThreadArg
{
	SimpleMutex lock;
	BW::map<unsigned long, int> threadIDCount;
};

template <>
void ConcurrencyTestWrapper<Concurrency_OurThreadID_ThreadArg>::threadTask()
{
	data.lock.grab();
	data.threadIDCount[ OurThreadID() ]++;
	data.lock.give();
}

TEST( Concurrency_OurThreadID )
{
	ConcurrencyTestWrapper<Concurrency_OurThreadID_ThreadArg> wrapper;
	for (int i = 0; i < 500; ++i)
		wrapper.startThread( "Concurrency_OurThreadID" );
	wrapper.endAllThreads();

	BW::map<unsigned long, int>::const_iterator it;
	for (it = wrapper.data.threadIDCount.begin();
		it != wrapper.data.threadIDCount.end();
		++it)
	{
		CHECK_EQUAL( 1, it->second );
	}
}

// SimpleSemaphore: Test the degenerate case of a 1-valued semaphore
struct Concurrency_SimpleSemaphore_ThreadArg
{
	SimpleSemaphore semaphore;
	int value;

	Concurrency_SimpleSemaphore_ThreadArg() : value( 0 ){}
};

template <>
void ConcurrencyTestWrapper<Concurrency_SimpleSemaphore_ThreadArg>::threadTask()
{
	data.semaphore.pull();
	data.value++;
	data.semaphore.push();
}

TEST( Concurrency_SimpleSemaphore )
{
	ConcurrencyTestWrapper<Concurrency_SimpleSemaphore_ThreadArg> wrapper;

	CHECK_EQUAL( false, wrapper.data.semaphore.pullTry() );

	for (int i = 0; i < 500; ++i)
		wrapper.startThread( "Concurrency_SimpleSemaphore" );
	CHECK_NOT_CHANGING( wrapper.data.value );
	CHECK_EQUAL( 0, wrapper.data.value );
	wrapper.data.semaphore.push();
	wrapper.endAllThreads();

	CHECK_EQUAL( 500, wrapper.data.value );
	CHECK_EQUAL( true, wrapper.data.semaphore.pullTry() );
	CHECK_EQUAL( false, wrapper.data.semaphore.pullTry() );
}

// ReadWriteLock: Check that writing waits for readers to complete and blocks
// futher readers
struct Concurrency_ReadWriteLock_ThreadArg
{
	volatile int readCount;
	SimpleMutex countLock;
	ReadWriteLock rwlock;

	Concurrency_ReadWriteLock_ThreadArg() : readCount( 0 ) {}
};

template <>
void ConcurrencyTestWrapper<Concurrency_ReadWriteLock_ThreadArg>::threadTask()
{
	data.rwlock.beginRead();
	data.countLock.grab();
	data.readCount++;
	data.countLock.give();
	data.rwlock.endRead();
}

TEST( Concurrency_ReadWriteLock )
{
	ConcurrencyTestWrapper<Concurrency_ReadWriteLock_ThreadArg> wrapper;
	for (int i = 0; i < 250; ++i)
		wrapper.startThread( "Concurrency_ReadWriteLock" );
	SLEEPSECS( 0 );
	wrapper.data.rwlock.beginWrite();
	for (int i = 0; i < 250; ++i)
		wrapper.startThread( "Concurrency_ReadWriteLock" );

	CHECK_NOT_CHANGING( wrapper.data.readCount );
	CHECK( wrapper.data.readCount <= 250 );

	wrapper.data.rwlock.endWrite();
	wrapper.endAllThreads();

	CHECK_EQUAL( 500, wrapper.data.readCount );
}

// Repeat the ReadWriteLock test, with guards
struct Concurrency_ReadWriteLock_Guards_ThreadArg
{
	volatile int readCount;
	SimpleMutex countLock;
	ReadWriteLock rwlock;

	Concurrency_ReadWriteLock_Guards_ThreadArg() : readCount( 0 ) {}
};

template <>
void ConcurrencyTestWrapper<Concurrency_ReadWriteLock_Guards_ThreadArg>::threadTask()
{
	ReadWriteLock::ReadGuard guard( data.rwlock );
	data.countLock.grab();
	data.readCount++;
	data.countLock.give();
}

TEST( Concurrency_ReadWriteLock_Guards )
{
	ConcurrencyTestWrapper<Concurrency_ReadWriteLock_Guards_ThreadArg> wrapper;
	for (int i = 0; i < 250; ++i)
		wrapper.startThread( "Concurrency_ReadWriteLock_Guards" );
	SLEEPSECS( 0 );
	{
		ReadWriteLock::WriteGuard guard( wrapper.data.rwlock );
		for (int i = 0; i < 250; ++i)
			wrapper.startThread( "Concurrency_ReadWriteLock_Guards" );

		CHECK_NOT_CHANGING( wrapper.data.readCount );
		CHECK( wrapper.data.readCount <= 250 );
	}
	wrapper.endAllThreads();

	CHECK_EQUAL( 500, wrapper.data.readCount );
}

// THREADLOCAL: Check that static threadlocal
// variables are not affected by other threads.
struct Concurrency_THREADLOCAL_ThreadArg
{
	static THREADLOCAL( int ) staticval;
};

/* static */ THREADLOCAL( int ) Concurrency_THREADLOCAL_ThreadArg::staticval
	= 0;

template <>
void ConcurrencyTestWrapper<Concurrency_THREADLOCAL_ThreadArg>::threadTask()
{
	data.staticval++;
}

TEST( Concurrency_THREADLOCAL )
{
	ConcurrencyTestWrapper<Concurrency_THREADLOCAL_ThreadArg> wrapper;
	for (int i = 0; i < 500; ++i)
		wrapper.startThread( "Concurrency_THREADLOCAL" );
	CHECK_NOT_CHANGING( wrapper.data.staticval );
	wrapper.endAllThreads();

	CHECK_EQUAL( 0, wrapper.data.staticval );
}


// SimpleMutexHolder: SimpleMutex test, using holders
struct Concurrency_SimpleMutexHolder_ThreadArg
{
	SimpleMutex lock;
	int value;

	Concurrency_SimpleMutexHolder_ThreadArg() : value( 0 ) {}
};

template <>
void ConcurrencyTestWrapper<Concurrency_SimpleMutexHolder_ThreadArg>::threadTask()
{
	SimpleMutexHolder holder( data.lock );
	data.value++;
}

TEST( Concurrency_SimpleMutexHolder )
{
	ConcurrencyTestWrapper<Concurrency_SimpleMutexHolder_ThreadArg> wrapper;
	{ // Scope for holder
		SimpleMutexHolder holder( wrapper.data.lock );
		for (int i = 0; i < 500; ++i)
			wrapper.startThread( "Concurrency_SimpleMutexHolder" );
		CHECK_NOT_CHANGING( wrapper.data.value );
		CHECK_EQUAL( 0, wrapper.data.value );
	}
	wrapper.endAllThreads();

	CHECK_EQUAL( 500, wrapper.data.value );
}

// RecursiveMutexHolder: RecursiveMutex test, using holders
struct Concurrency_RecursiveMutexHolder_ThreadArg
{
	RecursiveMutex lock;
	int value;

	Concurrency_RecursiveMutexHolder_ThreadArg() : value( 0 ) {}
};

template <>
void ConcurrencyTestWrapper<Concurrency_RecursiveMutexHolder_ThreadArg>::threadTask()
{
	RecursiveMutexHolder holder( data.lock );
	{
		RecursiveMutexHolder holder2( data.lock );
		data.value++;
	}
}

TEST( Concurrency_RecursiveMutexHolder )
{
	ConcurrencyTestWrapper<Concurrency_RecursiveMutexHolder_ThreadArg> wrapper;
	{ // Scope for holder
		RecursiveMutexHolder holder( wrapper.data.lock );
		{
			RecursiveMutexHolder holder2( wrapper.data.lock );
			for (int i = 0; i < 500; ++i)
				wrapper.startThread( "Concurrency_RecursiveMutexHolder" );
			CHECK_NOT_CHANGING( wrapper.data.value );
			CHECK_EQUAL( 0, wrapper.data.value );
			CHECK( holder.isOwner() );
			CHECK( !holder2.isOwner() );
		}
	}
	wrapper.endAllThreads();

	CHECK_EQUAL( 500, wrapper.data.value );
}

// BWConcurrency functions: Simulate a collection of background writers
// and main thread reader
struct Concurrency_BWConcurrency_ThreadArg
{
	static ReadWriteLock lock;
	int value;

	Concurrency_BWConcurrency_ThreadArg() : value( 0 ) {}

	static void startMainThreadIdle()
	{
		Concurrency_BWConcurrency_ThreadArg::lock.endRead();
	}

	static void endMainThreadIdle()
	{
		Concurrency_BWConcurrency_ThreadArg::lock.beginRead();
	}
};

/* static */ ReadWriteLock Concurrency_BWConcurrency_ThreadArg::lock;

template <>
void ConcurrencyTestWrapper<Concurrency_BWConcurrency_ThreadArg>::threadTask()
{
	ReadWriteLock::WriteGuard guard(
		Concurrency_BWConcurrency_ThreadArg::lock );
	data.value++;
}

TEST( Concurrency_BWConcurrency )
{
	ConcurrencyTestWrapper<Concurrency_BWConcurrency_ThreadArg> wrapper;
	BWConcurrency::setMainThreadIdleFunctions(
		&Concurrency_BWConcurrency_ThreadArg::startMainThreadIdle, 
		&Concurrency_BWConcurrency_ThreadArg::endMainThreadIdle );
	Concurrency_BWConcurrency_ThreadArg::lock.beginRead();
	for (int i = 0; i < 500; ++i)
		wrapper.startThread( "Concurrency_BWConcurrency" );

	CHECK_NOT_CHANGING( wrapper.data.value );
	CHECK_EQUAL( 0, wrapper.data.value );

	BWConcurrency::startMainThreadIdling();
	BWConcurrency::endMainThreadIdling();

	CHECK_NOT_CHANGING( wrapper.data.value );

	BWConcurrency::startMainThreadIdling();
	SLEEPSECS( 5 );
	wrapper.endAllThreads();
	BWConcurrency::endMainThreadIdling();

	CHECK_EQUAL( 500, wrapper.data.value );
}

BW_END_NAMESPACE

// test_concurrency.cpp
