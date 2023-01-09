#ifndef CONCURRENCY_HPP
#define CONCURRENCY_HPP

#include "stdmf.hpp"
#include "bw_string.hpp"
#include "debug.hpp"

#if defined( WIN32 )
#include "cstdmf_windows.hpp"
#endif
#if defined( PLAYSTATION3 )
#include <cell/atomic.h>
#endif

//-----------------------------------------------------------------------------
// Cross platform atomic macros. Windows requires that atomic variables are
// volatile and 32 bit aligned.

/// Assert if address is unaligned. Alignment byte value must be a power of 2!
#define MF_ASSERT_ALIGNED( addr, align ) \
	MF_ASSERT_NOALLOC( (reinterpret_cast< uintptr_t >( addr ) & ( (align) - 1 )) == 0 )

#ifdef _DEBUG
#define ENABLE_ATOMIC32_ALIGNMENT_CHECK 1
#else
#define ENABLE_ATOMIC32_ALIGNMENT_CHECK 0
#endif

BW_BEGIN_NAMESPACE
typedef void (* SimpleThreadFunc)(void*);
BW_END_NAMESPACE

#if defined(WIN32)
#define BW_ALIGN( x ) __declspec(align( x ))
// As per MSDN
typedef LONG volatile bw_atomic32_t;

#if ENABLE_ATOMIC32_ALIGNMENT_CHECK

inline LONG InterlockedCompareExchangeChecked( LONG volatile * dest, LONG newval,
	LONG oldval )
{
	MF_ASSERT_ALIGNED( dest, 4 );
	return InterlockedCompareExchange( dest, newval, oldval );
}
inline LONG InterlockedIncrementChecked( LONG volatile * dest )
{
	MF_ASSERT_ALIGNED( dest, 4 );
	return InterlockedIncrement( dest );
}
inline LONG InterlockedDecrementChecked( LONG volatile * dest )
{
	MF_ASSERT_ALIGNED( dest, 4 );
	return InterlockedDecrement( dest );

}
inline LONG InterlockedExchangeAddChecked( LONG volatile * dest, LONG value )
{
	MF_ASSERT_ALIGNED( dest, 4 );
	return InterlockedExchangeAdd( dest, value );
}

#define BW_ATOMIC32_COMPARE_AND_SWAP( dest, newval, oldval ) \
	InterlockedCompareExchangeChecked( (dest), (newval), (oldval) )
#define BW_ATOMIC32_INC_AND_FETCH( dest ) \
	InterlockedIncrementChecked( (dest) )
#define BW_ATOMIC32_DEC_AND_FETCH( dest ) \
	InterlockedDecrementChecked( (dest) )
#define BW_ATOMIC32_FETCH_AND_ADD( dest, value ) \
	InterlockedExchangeAddChecked( (dest), (value) )
#define BW_ATOMIC32_FETCH_AND_SUB( dest, value ) \
	InterlockedExchangeAddChecked( (dest), -(value) )

#else // ENABLE_ATOMIC32_ALIGNMENT_CHECK

#define BW_ATOMIC32_COMPARE_AND_SWAP( dest, newval, oldval ) \
	InterlockedCompareExchange( (dest), (newval), (oldval) )
#define BW_ATOMIC32_INC_AND_FETCH( dest ) \
	InterlockedIncrement( (dest) )
#define BW_ATOMIC32_DEC_AND_FETCH( dest ) \
	InterlockedDecrement( (dest) )
#define BW_ATOMIC32_FETCH_AND_ADD( dest, value ) \
	InterlockedExchangeAdd( (dest), (value) )
#define BW_ATOMIC32_FETCH_AND_SUB( dest, value ) \
	InterlockedExchangeAdd( (dest), -(value) )
#endif // ENABLE_ATOMIC32_ALIGNMENT_CHECK

#elif defined(__GNUC__)
// As per GCC docs, only int, long and long long are specified in the ABI
typedef int bw_atomic32_t;
#define BW_ALIGN(x) __attribute__((aligned (( x ))))

#ifdef ENABLE_ATOMIC32_ALIGNMENT_CHECK
#define MF_DEBUG_ASSERT_ALIGNED( addr, align ) MF_ASSERT_ALIGNED( (addr), (align) )
#else
#define MF_DEBUG_ASSERT_ALIGNED( addr, align )
#endif

static inline bw_atomic32_t __sync_val_compare_and_swap_checked(bw_atomic32_t * dest, bw_atomic32_t oldval, bw_atomic32_t newval)
{
	MF_DEBUG_ASSERT_ALIGNED( dest, 4 );
	return __sync_val_compare_and_swap( dest, oldval, newval );
}

static inline bw_atomic32_t __sync_add_and_fetch_checked(bw_atomic32_t * dest, bw_atomic32_t value)
{
	MF_DEBUG_ASSERT_ALIGNED( dest, 4 );
	return __sync_add_and_fetch( dest, value );
}

static inline bw_atomic32_t __sync_sub_and_fetch_checked(bw_atomic32_t * dest, bw_atomic32_t value)
{
	MF_DEBUG_ASSERT_ALIGNED( dest, 4 );
	return __sync_sub_and_fetch( dest, value );
}

static inline bw_atomic32_t __sync_fetch_and_add_checked(bw_atomic32_t * dest, bw_atomic32_t value)
{
	MF_DEBUG_ASSERT_ALIGNED( dest, 4 );
	return __sync_fetch_and_add( dest, value );
}

static inline bw_atomic32_t __sync_fetch_and_sub_checked(bw_atomic32_t * dest, bw_atomic32_t value)
{
	MF_DEBUG_ASSERT_ALIGNED( dest, 4 );
	return __sync_fetch_and_sub( dest, value );
}

#define BW_ATOMIC32_COMPARE_AND_SWAP(dest, newval, oldval) __sync_val_compare_and_swap_checked((dest), (oldval), (newval))
#define BW_ATOMIC32_INC_AND_FETCH(dest) __sync_add_and_fetch_checked((dest), 1)
#define BW_ATOMIC32_DEC_AND_FETCH(dest) __sync_sub_and_fetch_checked((dest), 1)
#define BW_ATOMIC32_FETCH_AND_ADD(dest, value) __sync_fetch_and_add_checked((dest), (value)) 
#define BW_ATOMIC32_FETCH_AND_SUB(dest, value) __sync_fetch_and_sub_checked((dest), (value)) 

#elif defined( PLAYSTATION3 )
// Wild guess. It should match the above two.
typedef int32_t bw_atomic32_t;
#define BW_ATOMIC32_INC_AND_FETCH(dest) cellAtomicIncr32((dest))
#define BW_ATOMIC32_DEC_AND_FETCH(dest) cellAtomicDecr32((dest))
#endif

//-----------------------------------------------------------------------------

BW_BEGIN_NAMESPACE

/**
 * Helper structure for SimpleThread on both windows and linux.
 */
class SimpleThread;

typedef int bwThreadID;

struct ThreadTrampolineInfo
{
	ThreadTrampolineInfo( SimpleThreadFunc func, void * arg,
		SimpleThread * thread )
	{
		this->func = func;
		this->arg = arg;
		this->thread_ = thread;
	}

	SimpleThreadFunc func;
	void * arg;


	SimpleThread* thread_;
};


/**
 *  Dummy mutex class that doesn't actually do anything.
 */
class DummyMutex
{
public:
	void grab() {}
	bool grabTry() { return true; }
	void give() {}
};

#if defined( _WIN32 )

BW_END_NAMESPACE

#include <process.h>

BW_BEGIN_NAMESPACE

inline unsigned long OurThreadID()
{
	return (unsigned long)(::GetCurrentThreadId());
}

/**
 * Determines if we are running on a single CPU core machine.
 */
bool isSingleLogicalProcessor();

struct FrozenThreadInfo
{
	DWORD	id;
	HANDLE	handle;
	bool	isCurrent;
};


/**
 * This function is for crash handlers. Mostly to freeze everything ASAP and
 * collect frozen threads for callstack routines.
 *
 * @var out threads - pre-allocated array of information to return.
 *					Could be NULL if only freeze is required.
 *
 * @var in maxSize - size of pre-allocated array. Affects only amount of data
 *					recorded - all threads will be frozen after the call, even if
 *                  there were more threads then maxSize.
 *
 * @return actual amount of threads recorded into array (zero if threads == NULL)
 */
size_t freezeAllThreads( FrozenThreadInfo * threads, size_t maxSize);
void cleanupFrozenThreadsInfo( FrozenThreadInfo * threads, size_t size, 
	bool resumeThreads );


/**
 *	This class is a simple mutex.
 *	Note that critical sections are reentrant for the same thread, but this
 *	mutex is not reentrant and will raise an assert if reentered by the same
 *	thread.
 */
class SimpleMutex
{
public:
	SimpleMutex() : gone_( false ) { InitializeCriticalSection( &mutex_ ); }
	~SimpleMutex()	{ DeleteCriticalSection( &mutex_ ); }

	void grab()		{ EnterCriticalSection( &mutex_ ); MF_ASSERT_NOALLOC( !gone_ ); gone_ = true; }
	void give()		{ gone_ = false; LeaveCriticalSection( &mutex_ ); }
	bool grabTry()
	{
		if (TryEnterCriticalSection( &mutex_ ))
		{
			// Reentered by same thread
			// Did you forget to give()?
			MF_ASSERT_NOALLOC( !gone_ );
			gone_ = true;
			return true;
		}
		return false;
	};

private:
	CRITICAL_SECTION	mutex_;
	bool				gone_;
};


/**
 *	This class is a recursive mutex.
 */
class RecursiveMutex
{
public:
	RecursiveMutex() : count_(0)	{ InitializeCriticalSection( &mutex_ ); }
	~RecursiveMutex()	{ DeleteCriticalSection( &mutex_ ); }

	void grab()		{ EnterCriticalSection( &mutex_ ); ++count_; }
	void give()		{ --count_; LeaveCriticalSection( &mutex_ ); }
	bool grabTry()
	{
		if (TryEnterCriticalSection( &mutex_ ))
		{
			++count_;
			return true;
		}
		return false;
	}

	int lockCount() const { return count_; }

private:
	CRITICAL_SECTION	mutex_;
	int count_;
};


/**
 *	This class is a simple event
 */
class SimpleEvent
{
public:
	SimpleEvent( bool manReset = true,
		bool initVal = false, LPCTSTR name = NULL )
	{
		event_ = CreateEvent( 0, manReset, initVal, name );
		MF_ASSERT( event_ != NULL );
	}
	~SimpleEvent()
	{
		CloseHandle( event_ );
	}

	void set()
	{
		BOOL res = SetEvent( event_ );
		MF_ASSERT( res );
	}

	void reset()
	{
		BOOL res = ResetEvent( event_ );
		MF_ASSERT( res );
	}

	bool wait( DWORD dwMilliseconds = INFINITE ) const
	{
		return WaitForSingleObject( event_, dwMilliseconds ) == WAIT_OBJECT_0;
	}

private:
	HANDLE event_;
};

/**
 *	This class is an interlocked counter & event
 */
class SimpleEventCounter : public SimpleEvent
{
public:
	SimpleEventCounter( long initialValue, bool manReset = true,
		bool initVal = false, LPCTSTR name = NULL ) :
			SimpleEvent( manReset, initVal, name ),
			counter_( initialValue ) 
	{
	}

	void increment()
	{
		if (InterlockedIncrement( &counter_ ) == 1)
		{
			this->reset(); // first time counter is non zero, signal the event to wait if asked
		}
	}

	void decrement()
	{
		if (InterlockedDecrement( &counter_ ) == 0)
		{
			this->set(); // counter is 0, signal the event not to wait.
		}
	}

	void zeroCounter()
	{
		counter_ = 0;
		this->set(); // counter is 0, signal the event not to wait.
	}

	int getCounter()
	{
		return counter_;
	}

	bool wait() 
	{
		return SimpleEvent::wait();
	}

private:
	volatile long counter_;
};

/**
 *	This class is a simple semaphore
 */
class SimpleSemaphore
{
public:
	SimpleSemaphore() : sema_( CreateSemaphore( NULL, 0, 32767, NULL ) ) { }
	~SimpleSemaphore()	{ CloseHandle( sema_ ); }

	void push( int count = 1 ) { ReleaseSemaphore( sema_, count, NULL ); }
	void pull()			{ WaitForSingleObject( sema_, INFINITE ); }
	bool pullTry()		{ return WaitForSingleObject( sema_, 0 ) == WAIT_OBJECT_0; }
	bool pullTry( unsigned milliseconds )	{ return WaitForSingleObject( sema_, milliseconds ) == WAIT_OBJECT_0; }

private:
	HANDLE sema_;
};


/**
 *	This class implements a read-write lock
 */
class ReadWriteLock
{
	SimpleMutex lock_;
	HANDLE readEvent_;
	HANDLE writeEvent_;
	volatile LONG readCount_;

public:
	ReadWriteLock()
	{
		readEvent_ = CreateEvent( NULL, TRUE, FALSE, NULL );
		writeEvent_ = CreateEvent( NULL, FALSE, TRUE, NULL );
		readCount_ = 0;
	}

	~ReadWriteLock()
	{
		CloseHandle( readEvent_ );
		CloseHandle( writeEvent_ );
	}

	void beginRead()
	{
		if (InterlockedIncrement( &readCount_ ) == 1)
		{
			WaitForSingleObject( writeEvent_, INFINITE );
			SetEvent( readEvent_ );
		}

		WaitForSingleObject( readEvent_, INFINITE );
	}

	void endRead()
	{
		if (InterlockedDecrement( &readCount_ ) == 0)
		{
			ResetEvent( readEvent_ );
			SetEvent( writeEvent_ );
		}
	}

	void beginWrite()
	{
		lock_.grab();
		WaitForSingleObject( writeEvent_, INFINITE );
	}

	void endWrite()
	{
		SetEvent( writeEvent_ );
		lock_.give();
	}

	class ReadGuard
	{
		ReadWriteLock& rwl_;

	public:
		ReadGuard( ReadWriteLock& rwl )
			: rwl_( rwl )
		{
			rwl_.beginRead();
		}

		~ReadGuard()
		{
			rwl_.endRead();
		}
	};

	class WriteGuard
	{
		ReadWriteLock& rwl_;

	public:
		WriteGuard( ReadWriteLock& rwl )
			: rwl_( rwl )
		{
			rwl_.beginWrite();
		}

		~WriteGuard()
		{
			rwl_.endWrite();
		}
	};
};

/**
 *  This class is a simple thread
 */

class SimpleThread
{
public:
	CSTDMF_DLL SimpleThread();

	CSTDMF_DLL SimpleThread(SimpleThreadFunc threadfunc, void * arg );

	CSTDMF_DLL SimpleThread(SimpleThreadFunc threadfunc, void * arg,	const BW::string& threadName );

	CSTDMF_DLL ~SimpleThread();

	CSTDMF_DLL void init(SimpleThreadFunc threadfunc, void * arg );

	CSTDMF_DLL void init(SimpleThreadFunc threadfunc, void * arg, const BW::string& threadName );

	struct AutoHandle	// Ugly!!
	{
		AutoHandle( HANDLE h ) { HANDLE p = GetCurrentProcess();
			DuplicateHandle( p, h, p, &real_, 0, TRUE, 0 ); }
		~AutoHandle() { CloseHandle( real_ ); }
		operator HANDLE() const { return real_; }
		HANDLE real_;
	};

	HANDLE handle() const	{ return thread_; }	// exposed for more complex ops
	static AutoHandle current()	{ return AutoHandle( GetCurrentThread() ); }

	const char* GetThreadName() {return threadName_.c_str();}

private:
	HANDLE thread_;
	BW::string threadName_;
	ThreadTrampolineInfo* trampolineInfo_;
	/*
	 * this trampoline function is present so that we can hide the fact that windows
	 * and linux expect different function signatures for thread functions
	 */

	static unsigned __stdcall trampoline( void * arg );


#if !defined( CONSUMER_RELEASE ) && !defined( __clang__ )
	// SetThreadName taken from
	// http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
	static const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	} THREADNAME_INFO;
#pragma pack(pop)

	static void SetThreadName( DWORD dwThreadID, const char* threadName )
	{
		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = threadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;

		__try
		{
			RaiseException(
				MS_VC_EXCEPTION,
				0,
				sizeof( info ) / sizeof( ULONG_PTR ),
				( ULONG_PTR* )&info );
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
		}
	}
#endif
};


#if BWCLIENT_AS_PYTHON_MODULE

int tlsGetOffSet(int size);
char * tlsGetData();
void tlsIncRef();
void tlsDecRef();

const char   c_initFalse   = 0;
const char   c_initTrue    = 1;

/**
 *	You are not allowed to use __declspec( thread ) when building
 *	a DLL. The TLS API is the way to go if you require thread local
 *	storage. The ThreadLocal template class encapsulates the
 *	underlying TLS implementation and can be used in a way very
 *	similar to __declspec( thread ).
 */
template<typename T>
class ThreadLocal
{
public:
	ThreadLocal();
	ThreadLocal(const T & value);

	~ThreadLocal();

	T & get();
	const T & get() const;

	void set(const T & value);

	operator T & ()
	{
		return get();
	}

	T & operator -> ()
	{
		return get();
	}

	T* operator&()
	{
		return &(get());
	}

	const T & operator = ( const T & other )
	{
		this->set(other);
		return this->get();
	}

	bool operator == ( const T & other ) const
	{
		return this->get() == other;
	}

	bool operator != ( const T & other ) const
	{
		return this->get() != other;
	}

	const T & operator ++ ()
	{
		return ++this->get();
	}

	T operator ++ (int)
	{
		return this->get()++;
	}

	const T & operator--()
	{
		return --this->get();
	}

	T operator--(int)
	{
		return this->get()--;
	}

	const T & operator+=( const T & inc )
	{
		return this->get() += inc;
	}

	const T & operator-=( const T & dec )
	{
		return this->get() -= dec;
	}

private:
	int dataOffset_;
	T   initValue_;
};

template<typename T>
ThreadLocal<T>::ThreadLocal()
{
	this->dataOffset_ = tlsGetOffSet(sizeof(T) + 1);
	tlsIncRef();
}

template<typename T>
ThreadLocal<T>::ThreadLocal(const T & value)
{
	char * tlsData    = tlsGetData();
	this->dataOffset_ = tlsGetOffSet(sizeof(T) + 1);
	this->initValue_  = value;
	this->get();
	tlsIncRef();
}

template<typename T>
ThreadLocal<T>::~ThreadLocal()
{
	tlsDecRef();
}

template<typename T>
T & ThreadLocal<T>::get()
{
	char * tlsChar = tlsGetData();
	char * initialised = tlsChar + this->dataOffset_;
	T & value = *(T*)(tlsChar + this->dataOffset_ + 1);
	if (*initialised == c_initFalse)
	{
		value = this->initValue_;
		*initialised = c_initTrue;
	}
	return value;
}

template<typename T>
const T & ThreadLocal<T>::get() const
{
	return const_cast<ThreadLocal<T>*>(this)->get();
}

template<typename T>
void ThreadLocal<T>::set(const T & value)
{
	T & data = this->get();
	data = value;
}

// Reflect == to use the version in the ThreadLocal class template
template<typename T>
int operator== (const T & left, const ThreadLocal<T> & right)
{
	return right == left;
}

template<typename T>
std::ostream& operator<<(std::ostream & o, const ThreadLocal<T> & t)
{
	o << t.get();
	return o;
}

// Using "ThreadLocal<type>" as opposed to "__declspec( thread ) type" incurs
// an approximate 20% performance penalty. But because thread local variables
// are rarely used in time-critical code this shouldn't be a problem. Anyway,
// "ThreadLocal<type>" is only used when building <bwclient>.dll so this will
// never be a problem in a production release.
#define THREADLOCAL(type) ThreadLocal<type>

#else // BWCLIENT_AS_PYTHON_MODULE

/// thread local declaration - ie "static THREADLOCAL(type) blah"
#if !defined( MF_SINGLE_THREADED )
	#define THREADLOCAL(type) __declspec( thread ) type
#else
	#define THREADLOCAL(type) type
#endif

#endif // BWCLIENT_AS_PYTHON_MODULE

#else // !defined( _WIN32 )

#if !defined( MF_SINGLE_THREADED )
BW_END_NAMESPACE

#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

BW_BEGIN_NAMESPACE

/**
 *	This class is a simple mutex
 */
class SimpleMutex
{
public:
	SimpleMutex()	{ pthread_mutex_init( &mutex_, NULL); }
	~SimpleMutex()	{ pthread_mutex_destroy( &mutex_ ); }

	void grab()		{ pthread_mutex_lock( &mutex_ ); }
	bool grabTry()	{ return (pthread_mutex_trylock( &mutex_ ) == 0); }
	void give()		{ pthread_mutex_unlock( &mutex_ ); }

private:
	pthread_mutex_t mutex_;
};


/**
 *	This class is a recursive mutex
 */
class RecursiveMutex
{
public:
	RecursiveMutex() : count_( 0 )
	{
		pthread_mutexattr_t mta;

		pthread_mutexattr_init( &mta );
		pthread_mutexattr_settype( &mta, PTHREAD_MUTEX_RECURSIVE );
		pthread_mutex_init( &mutex_, &mta );
	}
	~RecursiveMutex()	{ pthread_mutex_destroy( &mutex_ ); }

	void grab()		{ pthread_mutex_lock( &mutex_ ); ++count_; }
	bool grabTry()
	{
		if (pthread_mutex_trylock( &mutex_ ) == 0)
		{
			++count_;
			return true;
		}
		return false;
	}
	void give()		{ --count_; pthread_mutex_unlock( &mutex_ ); }

	int lockCount() const { return count_; }

private:
	pthread_mutex_t mutex_;
	int count_;
};


/**
 *	This class is a simple semaphore, implemented with a condition variable
 */
class SimpleSemaphore
{
public:
	SimpleSemaphore()
	{
		sem_init( &sem_, /* pshared */ 0, /* value */ 0);		
	}
	~SimpleSemaphore()
	{
		sem_destroy( &sem_ );
	}

	void pull()
	{
		while( sem_wait( &sem_ ) == -1 && errno == EINTR)
			;
	}
	bool pullTry()
	{
		return sem_trywait( &sem_ ) == 0;
	}

	// Unimplemented
	//bool pullTry( unsigned long ms )
	//{
	//	return true;
	//}

	void push( int count = 1 )
	{
		for (int i = 0; i < count; ++i)
		{
			sem_post( &sem_ );
		}
	}

private:
	sem_t sem_;
};


/**
 *	This class implements a read-write lock
 */
class ReadWriteLock
{
	pthread_rwlock_t lock_;

public:
	ReadWriteLock()
	{
		MF_VERIFY( pthread_rwlock_init( &lock_, NULL ) == 0 );
	}

	~ReadWriteLock()
	{
		MF_VERIFY( pthread_rwlock_destroy( &lock_ ) == 0 );
	}

	void beginRead()
	{
		MF_VERIFY( pthread_rwlock_rdlock( &lock_ ) == 0 );
	}

	void endRead()
	{
		MF_VERIFY( pthread_rwlock_unlock( &lock_ ) == 0 );
	}

	void beginWrite()
	{
		MF_VERIFY( pthread_rwlock_wrlock( &lock_ ) == 0 );
	}

	void endWrite()
	{
		MF_VERIFY( pthread_rwlock_unlock( &lock_ ) == 0 );
	}

	class ReadGuard
	{
		ReadWriteLock& rwl_;

	public:
		ReadGuard( ReadWriteLock& rwl )
			: rwl_( rwl )
		{
			rwl_.beginRead();
		}

		~ReadGuard()
		{
			rwl_.endRead();
		}
	};

	class WriteGuard
	{
		ReadWriteLock& rwl_;

	public:
		WriteGuard( ReadWriteLock& rwl )
			: rwl_( rwl )
		{
			rwl_.beginWrite();
		}

		~WriteGuard()
		{
			rwl_.endWrite();
		}
	};
};

inline unsigned long OurThreadID()
{
	return (unsigned long)(pthread_self());
}

/**
 *  This class is a simple thread
 */
class SimpleThread
{
public:
	SimpleThread();

	SimpleThread(
		SimpleThreadFunc threadfunc,
		void * arg );

	SimpleThread(
		SimpleThreadFunc threadfunc,
		void * arg,
		const BW::string& threadName );


	void init( SimpleThreadFunc threadfunc,
			   void * arg );
	void init( SimpleThreadFunc threadfunc,
			   void * arg,
			   const BW::string& threadName );

	~SimpleThread();

	pthread_t handle() const	{ return thread_; }
	static pthread_t current()	{ return pthread_self(); }

	const char* GetThreadName() {return threadName_.c_str();}

private:
	pthread_t thread_;
	ThreadTrampolineInfo* trampolineInfo_;
	BW::string threadName_;
	/*
	 * this trampoline function is present so that we can hide the fact that windows
	 * and linux expect different function signatures for thread functions
	 */

	static void * trampoline( void * arg );
};

#else // MF_SINGLE_THREADED

/**
 *	No-op version of SimpleMutex.
 */
class SimpleMutex
{
public:
	SimpleMutex() {}
	~SimpleMutex() {}

	void grab() {}
	void give() {}
	bool grabTry() { return true; }
};

/**
 *	No-op version of RecursiveMutex
 */
class RecursiveMutex
{
public:
	void grab() {}
	bool grabTry() { return true; }
	void give() {}
	int lockCount() const { return 0; }
};

/**
 *	No-op version of SimpleSemaphore.
 */
class SimpleSemaphore
{
public:
	SimpleSemaphore() {}
	~SimpleSemaphore() {}

	void push( int count = 1 ) {}
	void pull() {}
	bool pullTry() { return true; }
};

inline unsigned long OurThreadID()
{
	return 0;
}


/**
 *	No-op version of ReadWriteLock
 */
class ReadWriteLock
{
public:
	void beginRead();
	void endRead();
	void beginWrite();
	void endWrite();
	class ReadGuard
	{
	public:
		ReadGuard(ReadWriteLock& rwl) {}
	};
	class WriteGuard
	{
	public:
		WriteGuard(ReadWriteLock& rwl) {}
	};
};


/**
 *	No-op version of SimpleThread.
 */
class SimpleThread
{
public:
	SimpleThread() {}
	SimpleThread( SimpleThreadFunc threadfunc, void * arg );

	SimpleThread( SimpleThreadFunc threadfunc, void * arg,
			const BW::string& threadName );

	void init( SimpleThreadFunc threadfunc, void * arg )
	{
	}

	void init( SimpleThreadFunc threadfunc,
			   void * arg,
			   const BW::string& threadName )
	{
	}

	int handle() const { return 0; }
	static int current() { return 0; }

};

#endif // MF_SINGLE_THREADED

/// thread local declaration - ie "static THREADLOCAL(type) blah"
#if !defined( __APPLE__ )
# define THREADLOCAL(type) __thread type
#else
# define THREADLOCAL(type) type
#endif

#endif // _WIN32

/**
 *	This class grabs and holds a mutex for as long as it is around
 */
class SimpleMutexHolder
{
public:
	SimpleMutexHolder( SimpleMutex & sm ) : sm_( sm )	{ sm_.grab(); }
	~SimpleMutexHolder()								{ sm_.give(); }
private:
	SimpleMutex & sm_;
};

/**
 *	This class grabs and holds a mutex for as long as it is around
 */
class RecursiveMutexHolder
{
public:
	RecursiveMutexHolder( RecursiveMutex & sm ) :
		sm_( sm )
	{
		sm_.grab();
		index_ = sm_.lockCount();
	}

	~RecursiveMutexHolder() { sm_.give(); }

	int recursiveIndex() const
	{
		return index_;
	}

	bool isOwner() const
	{
		return index_ == 1;
	}
private:
	RecursiveMutex & sm_;
	int index_;
};

CSTDMF_DLL extern void (*pMainThreadIdleStartFunc)();
CSTDMF_DLL extern void (*pMainThreadIdleEndFunc)();

namespace BWConcurrency
{
inline void startMainThreadIdling()
{
	(*pMainThreadIdleStartFunc)();
}

inline void endMainThreadIdling()
{
	(*pMainThreadIdleEndFunc)();
}

inline
void setMainThreadIdleFunctions( void (*pStartFunc)(), void (*pEndFunc)() )
{
	pMainThreadIdleStartFunc = pStartFunc;
	pMainThreadIdleEndFunc = pEndFunc;
}

} // namespace BWConcurrency

BW_END_NAMESPACE

#endif // CONCURRENCY_HPP
