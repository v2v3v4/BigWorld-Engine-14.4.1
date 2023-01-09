#include "pch.hpp"

#include "concurrency.hpp"

#include "allocator.hpp"
#include "bw_util.hpp"
#include "debug_exception_filter.hpp"
#include "profiler.hpp"

#if defined(_WIN32)
#include <TlHelp32.h>
#pragma warning(push)
#pragma warning(disable: 4091)
#include "dbghelp.h"
#pragma warning(pop)
#endif

DECLARE_DEBUG_COMPONENT2( "CStdMF", 0 )

int mf_getpid()
{
#if defined(__unix__)
	return getpid();
#elif defined( _WIN32 )
	return (int) GetCurrentProcessId();
#else
	return -1;
#endif
}

BW_BEGIN_NAMESPACE

static void NoOp() { }
void (*pMainThreadIdleStartFunc)() = &NoOp;
void (*pMainThreadIdleEndFunc)() = &NoOp;

namespace
{
void assertHolder()
{
	// This is here because there's no really appropriate methods to
	// put it in.
	BW_STATIC_ASSERT( sizeof(bw_atomic32_t) == 4, bw_atomic32_t_bad_size );
}
}


#if defined(_WIN32)
bool isSingleLogicalProcessor()
{
	DWORD_PTR processAffinity;
	DWORD_PTR systemAffinity;
	HRESULT res = GetProcessAffinityMask(
		GetCurrentProcess(), &processAffinity, &systemAffinity );
	if (!SUCCEEDED( res ))
	{
		ERROR_MSG( "Could not get process affinity mask\n" );
		return 1;
	}

	int count = 0;
	for (int i = 0; i < sizeof(DWORD_PTR)*8; i++)
	{
		count += ( processAffinity & ( DWORD_PTR(1) << i ) ) ? 1 : 0;
	}

	return count == 1;
}
#endif // defined (_WIN32)

#if !defined ( MF_SINGLE_THREADED )
//------------------------------------------------------------------------------
/**
 *	Constructor.
 */
SimpleThread::SimpleThread() :
#if defined( _WIN32 )
	thread_( NULL ),
#else
	thread_( 0 ),
#endif
	trampolineInfo_( NULL )
{}


/**
 *	Constructor.
 */
SimpleThread::SimpleThread( SimpleThreadFunc threadfunc, void * arg ) :
#if defined( _WIN32 )
	thread_( NULL ),
#else
	thread_( 0 ),
#endif
	trampolineInfo_( NULL )
{
	this->init( threadfunc, arg, "Unnamed thread" );
}


/**
 *	Constructor.
 */
SimpleThread::SimpleThread( SimpleThreadFunc threadfunc, void * arg,
		const BW::string & threadName ) :
#if defined( _WIN32 )
	thread_( NULL ),
#else
	thread_( 0 ),
#endif
	trampolineInfo_( NULL )
{
	this->init( threadfunc, arg, threadName );
}


/**
 *	Destructor.
 */
SimpleThread::~SimpleThread()
{
#if defined( _WIN32 )
	WaitForSingleObject( thread_, INFINITE );
	CloseHandle( thread_ );
#else
	pthread_join( thread_, NULL );
#endif
	bw_safe_delete( trampolineInfo_ );
}


/**
 *
 */
void SimpleThread::init( SimpleThreadFunc threadfunc, void * arg )
{
	this->init( threadfunc, arg, "Unknown simple thread" );
}


/**
 *
 */
void SimpleThread::init( SimpleThreadFunc threadfunc, void * arg,
	const BW::string & threadName )
{
	MF_ASSERT( trampolineInfo_ == NULL )
	bool threadCreated = false;
	
	trampolineInfo_ = new ThreadTrampolineInfo( threadfunc, arg, this );
	
#if defined( _WIN32 )
	unsigned threadId;
	thread_ = HANDLE( _beginthreadex( 0, 0, trampoline, trampolineInfo_,
					0, &threadId ) );
	threadCreated = thread_ != 0;
#else
	threadCreated  = pthread_create( &thread_, NULL, trampoline,
					trampolineInfo_ ) == 0;
#endif

	if (threadCreated)
	{
#if !defined( CONSUMER_RELEASE ) && defined( _WIN32 ) && !defined( __clang__ )
		SetThreadName( threadId, threadName.c_str() );
#endif
		threadName_ = threadName;
	}
	else
	{
		ERROR_MSG( "Could not create new SimpleThread\n" );
	}
}


/**
 *
 */
#if defined( _WIN32 )
unsigned __stdcall SimpleThread::trampoline( void * arg )
#else
void * SimpleThread::trampoline( void * arg )
#endif
{
	ThreadTrampolineInfo * info = static_cast< ThreadTrampolineInfo * >( arg );

#if ENABLE_PROFILER
	// Threads add themselves to the profiling system by default.
	g_profiler.addThisThread( info->thread_->GetThreadName() );
#endif

#if defined( _WIN32 )
	CallWithExceptionFilter( info->func, info->arg );
#else
	info->func( info->arg );
#endif

#if ENABLE_PROFILER
	g_profiler.removeThread( OurThreadID() );
#endif

#if defined( _WIN32 )
	BW::Allocator::onThreadFinish();
	return 0;
#else
	return NULL;
#endif
}

#endif // !MF_SINGLE_THREADED

#if defined( BWCLIENT_AS_PYTHON_MODULE )

const  int c_ltsDataSize        = 1024;
const  int c_ltsOffsetOffset    = c_ltsDataSize;
const  int c_ltsTotalBufferSize = c_ltsOffsetOffset + sizeof( int );
static int s_refCounter = 0;

typedef BW::vector< void * > VoidPtrVector;
static VoidPtrVector s_tlsData;


/**
 *
 */
int tlsGetIndex()
{
	static int tlsIndex = TlsAlloc();
	return tlsIndex;
}


/**
 *
 */
int tlsGetOffSet( int step )
{
	char * data = tlsGetData();
	int * dataOffSet = (int *)&data[ c_ltsOffsetOffset ];
	MF_ASSERT( (*dataOffSet + step) <= c_ltsOffsetOffset );

	int oldOffSet = *dataOffSet;

	*dataOffSet += step;
	return oldOffSet;
}


/**
 *
 */
char * tlsGetData()
{
	int index = tlsGetIndex();
	char * tlsData = (char *) TlsGetValue( index );

	if (tlsData == NULL)
	{
		tlsData = new char[ c_ltsTotalBufferSize ];
		s_tlsData.push_back( tlsData );

		memset( tlsData, c_initFalse, c_ltsDataSize );

		// reset offet (c_initFalse may not be 0)
		int * offset = (int*)&tlsData[ c_ltsOffsetOffset ];
		*offset = 0;

		TlsSetValue( tlsGetIndex(), tlsData ); 
	}

	return tlsData;
}


/**
 *
 */
void tlsIncRef()
{
	++s_refCounter;
}


/**
 *
 */
void tlsDecRef()
{
	--s_refCounter;

	if (s_refCounter == 0)
	{
		TlsFree( tlsGetIndex() );

		VoidPtrVector::iterator it  = s_tlsData.begin();
		VoidPtrVector::iterator end = s_tlsData.end();

		while (it != end)
		{
			bw_safe_delete_array( *it );
			it++;
		}

		s_tlsData.clear();
	}
}

#endif // BWCLIENT_AS_PYTHON_MODULE


#if defined( _WIN32 )

size_t freezeAllThreads( FrozenThreadInfo * threads, size_t maxSize )
{
	static const int SUSPEND_ERROR = static_cast<DWORD>(-1);

	const DWORD processID = ::GetCurrentProcessId();
	const DWORD currentThreadID = ::GetCurrentThreadId();

	const HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 
		processID );

	if (snapshot == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	THREADENTRY32 threadInfo;
	threadInfo.dwSize = sizeof(threadInfo);
	char errorMsg[4096];

	if (!Thread32First( snapshot, &threadInfo ))
	{
		BWUtil::getLastErrorMsg( errorMsg, ARRAY_SIZE( errorMsg ) );
		ERROR_MSG( "Failed to retrieve first thread: %s\n", errorMsg );

		CloseHandle( snapshot );
		return 0;
	}

	size_t size = 0;
	if (threads && (maxSize > 0))
	{
		threads[0].id = currentThreadID;
		threads[0].handle = GetCurrentThread();
		threads[0].isCurrent = true;
		++size;
	}

	do 
	{
		if ((threadInfo.th32OwnerProcessID != processID) ||
			(threadInfo.th32ThreadID == currentThreadID))
		{
			continue;
		}

		DWORD permissions = THREAD_GET_CONTEXT | READ_CONTROL |
			THREAD_QUERY_INFORMATION |
			THREAD_SUSPEND_RESUME;

		HANDLE threadHandle = OpenThread( permissions, FALSE,
			threadInfo.th32ThreadID );

		if (threadHandle == INVALID_HANDLE_VALUE)
		{
			BWUtil::getLastErrorMsg( errorMsg, ARRAY_SIZE( errorMsg ) );
			WARNING_MSG( "Thread open failed: %s\n", errorMsg );
			continue;
		}

		if (SuspendThread( threadHandle ) == SUSPEND_ERROR)
		{
			BWUtil::getLastErrorMsg( errorMsg, ARRAY_SIZE( errorMsg ) );
			WARNING_MSG( "Failed to freeze thread #%lu: %s\n",
				threadInfo.th32ThreadID, errorMsg );
		}

		if (threads && (size < maxSize))
		{
			threads[size].id = threadInfo.th32ThreadID;
			threads[size].handle = threadHandle;
			threads[size].isCurrent = false;
			++size;
		}
	} while (Thread32Next( snapshot, &threadInfo ));

	CloseHandle( snapshot );
	return size;
}


void cleanupFrozenThreadsInfo( FrozenThreadInfo * threads, size_t size,
	bool resumeThreads )
{
	static const int RESUME_ERROR = static_cast< DWORD >( -1 );

	for (size_t i = 0; i < size; ++i)
	{
		if (threads[i].isCurrent)
		{
			continue;
		}

		if (resumeThreads && (ResumeThread( threads[i].handle ) == RESUME_ERROR))
		{
			char errorMsg[4096];
			BWUtil::getLastErrorMsg( errorMsg, ARRAY_SIZE( errorMsg ) );
			WARNING_MSG( "Failed to resume thread #%lu: %s\n",
				threads[i].id, errorMsg );
		}

		CloseHandle( threads[i].handle );
	}
}

#endif // #if defined( _WIN32 )

BW_END_NAMESPACE

// concurrency.cpp
