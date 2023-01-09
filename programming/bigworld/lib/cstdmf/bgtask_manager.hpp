#ifndef BGTASK_MANAGER_HPP
#define BGTASK_MANAGER_HPP

#include "cstdmf/concurrency.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/init_singleton.hpp"
#include "cstdmf/timestamp.hpp"
#include "cstdmf/task_watcher.hpp"

#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

class TaskManager;
class BackgroundTaskThread;

/**
 *	This interface is used to implement tasks that will be run by the 
 *	TaskManager.
 */
class BackgroundTask : public SafeReferenceCount
{
public:
	BackgroundTask( const char * name ) :
		stop_( false ),
		name_( name ),
		timeEnqueuedToBackground_( 0 ),
		timeEnqueuedToMainThread_( 0 )
	{}
	/**
	 *	This method is called to perform a task in the background thread. Derived
	 *	classes will often add themselves back to the manager at the end of this
	 *	method by calling <code>mgr.addMainThreadTask( this )</code>. This allows
	 *	this object to complete in the main thread.
	 */
	virtual void doBackgroundTask( TaskManager & mgr,
		   BackgroundTaskThread * pThread )
	{
		this->doBackgroundTask( mgr );
	}

	virtual void doMainThreadTask( TaskManager & mgr ) {};

	/**
	 *	This method is called on a task that is already running and you want it
	 *	to stop.
	 */
	virtual void stop() { stop_ = true; }

	/**
	 *	This method is called on a task that has not been run yet, but we want
	 *	to discard it.
	 *	@note this should only be called by TaskManager when clearing the
	 *	pending task list.
	 */
	virtual void cancel( TaskManager & mgr ) {}

    const char* name() { return name_; }

protected:
	virtual ~BackgroundTask() {};

	virtual void doBackgroundTask( TaskManager & mgr ) = 0;

private:
	friend class TaskManager;
	volatile bool stop_;
	friend class BackgroundTaskThread;
    const char* name_;
	uint64 timeEnqueuedToBackground_;
	uint64 timeEnqueuedToMainThread_;
};

typedef SmartPointer< BackgroundTask > BackgroundTaskPtr;
typedef void StartThreadCallback();

/**
 *	This class encapsulate a task that can be submitted to the background task
 *	manager for processing. The task function and callback function must be
 *	static methods.
 *
 *	This class is for backwards compatibility and should probably not be used
 *	in new code.
 */
class CStyleBackgroundTask : public BackgroundTask
{
public:
	typedef void (*FUNC_PTR)( void * );

	CSTDMF_DLL CStyleBackgroundTask( const char* name, FUNC_PTR bgFunc, void * bgArg,
		FUNC_PTR fgFunc = NULL, void * fgArg = NULL );

	CSTDMF_DLL CStyleBackgroundTask( FUNC_PTR bgFunc, void * bgArg,
		FUNC_PTR fgFunc = NULL, void * fgArg = NULL );

	CSTDMF_DLL void doBackgroundTask( TaskManager & mgr );
	CSTDMF_DLL void doMainThreadTask( TaskManager & mgr );

private:
	FUNC_PTR bgFunc_;
	void * bgArg_;

	FUNC_PTR fgFunc_;
	void * fgArg_;
};


/**
 *
 */
class BackgroundThreadData : public SafeReferenceCount
{
public:
	virtual bool onStart( BackgroundTaskThread & thread ) = 0;
	virtual void onEnd( BackgroundTaskThread & thread ) {}
};


typedef SmartPointer< BackgroundThreadData > BackgroundThreadDataPtr;


/**
 *	This class encapsulates a working thread execute given
 *	tasks.
 */
class BackgroundTaskThread : public SimpleThread
{
public:
	BackgroundTaskThread(
		TaskManager & mgr,
		BackgroundThreadDataPtr pData );

	BackgroundTaskThread(
		TaskManager & mgr,
		BackgroundThreadDataPtr pData,
		const BW::string& threadName );

	BackgroundThreadDataPtr pData() const			{ return pData_; }
	void pData( BackgroundThreadDataPtr pData ) 	{ pData_ = pData; }
	TaskManager& mgr() { return mgr_; }

private:
	static void s_start( void * arg );
	void run();

	TaskManager & mgr_;
	BackgroundThreadDataPtr pData_;
};


/**
 *	This class is a simple class is used to run a C function at the start of a
 *	background thread.
 */
class StartThreadCallbackWrapper : public BackgroundThreadData
{
public:
	StartThreadCallbackWrapper( StartThreadCallback * pCallback ) :
		pCallback_( pCallback )
	{
	}

	virtual bool onStart( BackgroundTaskThread & thread )
	{
		if (pCallback_)
		{
			(*pCallback_)();
		}

		thread.pData( NULL );
		return true;
	}

private:
	StartThreadCallback * pCallback_;
};


/**
 *	This class is used to help in the completion on a thread. It informs the
 *	manager that the thread has gone.
 */
class ThreadFinisher : public BackgroundTask
{
public:
	ThreadFinisher( BackgroundTaskThread * pThread ) :
		BackgroundTask(  "ThreadFinisher" ),
		pThread_( pThread )
	{
	}

	virtual void doBackgroundTask( TaskManager & mgr ) {};
	virtual void doMainThreadTask( TaskManager & mgr );

private:
	BackgroundTaskThread * pThread_;
};


/**
 *	This class defines a background task manager that manages a pool
 *	of working threads. BackgroundTask objects are added to be processed by a
 *	background thread and then, possibly by the main thread again.
 *	
 *	Tasks vs threads:
 *	There can be any number of tasks added to the background task manager.
 *	Tasks wait until a thread pulls and runs them.
 *	
 *	Adding tasks:
 *	The pool of tasks is called "bgTaskList_".
 *	Tasks (of type BackgroundTask) are created on the main thread and add
 *	themselves to be processed in the background by the thread pool by calling
 *	BackgroundTaskThread::addBackgroundTask when they are ready to run.
 *	This must only be called in the main thread.
 *	
 *	Removing tasks:
 *	stop() will automatically be called on tasks in BgTaskManager::stopAll()
 *	by default. Tasks which override stop() must also call BackgroundTask::stop().
 *
 *	When a background task finishes, it can add itself to the main thread to
 *	do cleanup tasks.
 *	
 *	Running tasks:
 *	Tasks start to run when there is a thread free for them.
 *	When a free background thread starts, BackgroundTaskThread::run calls
 *	BgTaskManager::pullBackgroundTask (must be called on a background thread)
 *	to get the next highest priority task in the bgTaskList_.
 *	
 *	Adding threads:
 *	Threads are started by calling BackgroundTaskThread::startThreads. You can
 *	specify a name for the threads for debugging purposes.
 *	
 *	BackgroundTaskThread::setNumberOfThreads can be used to keep a consistent
 *	thread count. For example to keep the number of threads the same as the
 *	number of processor cores, as tasks finish their threads also end.
 *	See also ProcessorAffinity.
 *	
 *	Removing threads:
 *	Call BgTaskManager::stopThreads or BgTaskManager::stopAll.
 *	stopThreads will call bgTaskList_.push( NULL ), when
 *	BackgroundTaskThread::run() pulls a NULL task, it ends the thread.
 *	
 *	Managing starting/stopping threads:
 *	In startThreads, the total number of threads running is incremented.
 *	When threads finish, they call the ThreadFinisher which will call
 *	onThreadFinished which decrements the total number of threads running.
 *	stopAll will wait for numRunningThreads_ to go down, so threads must not
 *	start while stopAll is running.
 *
 *	Main vs background threads
 *	Methods in BgTaskManager can be accessed by any thread so there needs to be
 *	a distinction between what is for the main thread or background threads.
 *	
 *	Main thread functions are ones which involve setting up the BgTaskManager
 *	(init, fini) and those that are for starting/stopping/updating
 *	threads (tick, startThreads, setNumberOfThreads, stopThreads, stopAll,
 *	onThreadFinished)
 *	There could be problems if eg startThreads is called at the same time as
 *	stopThreads, so restricting these to the main thread also resolves this.
 *	
 *	Functions that can be accessed from any thread are const member functions
 *	(such as numBgTasksLeft) and functions for adding tasks (addBackgroundTask,
 *	addMainThreadTask).
 *	
 *	Functions that should be only accessed by background threads are for
 *	background thread callbacks (pullBackgroundTask, onBackgroundTaskFinished).
 *	
 */
class TaskManager
{
public:
	friend class BackgroundTaskThread;

	enum 
	{ 
		MIN = 0,
		LOW = 32,
		MEDIUM = 64,
		DEFAULT = MEDIUM,
		HIGH = 96,
		MAX = 128
	};

	CSTDMF_DLL TaskManager( bool exclusive = true );
	CSTDMF_DLL virtual ~TaskManager();

	CSTDMF_DLL void tick();

	CSTDMF_DLL virtual bool initWatchers( const char * path, float warnThreshold = 0 );

	CSTDMF_DLL void startThreads( int numThreads,
		BackgroundThreadDataPtr pData = NULL );

	CSTDMF_DLL void startThreads( const BW::string& threadNamePrefix,
		int numThreads,
		BackgroundThreadDataPtr pData = NULL );

	void startThreads( int numThreads,
		StartThreadCallback * startThreadCallback )
	{
		this->startThreads(
			"Unknown BG Task",
			numThreads,
			new StartThreadCallbackWrapper( startThreadCallback ) );
	}

	void startThreads( const BW::string& threadNamePrefix,
		int numThreads,
		StartThreadCallback * startThreadCallback )
	{
		this->startThreads(
				threadNamePrefix,
				numThreads,
				new StartThreadCallbackWrapper( startThreadCallback ) );
	}

	CSTDMF_DLL void setNumberOfThreads( int numThreads );

	CSTDMF_DLL void stopThreads( int numThreads );

	CSTDMF_DLL virtual void stopAll( bool discardPendingTasks = true,
		bool waitForThreads = true );

	CSTDMF_DLL void addBackgroundTask( BackgroundTaskPtr pTask, int priority = DEFAULT );
	CSTDMF_DLL void addMainThreadTask( BackgroundTaskPtr pTask );

	CSTDMF_DLL void addDelayedBackgroundTask( BackgroundTaskPtr pTask );

	CSTDMF_DLL unsigned int numBgTasksLeft() const;
	CSTDMF_DLL bool hasTasks() const;

	void setTaskListWarningSize( uint warningSize, uint warningStepSize );

	/**
	 *	This method returns the total number of running threads. That is threads
	 *	that have not told the main thread that they have stopped. This is always
	 *	no less than numUnstoppedThreads.
	 */
	int numRunningThreads() const { return numRunningThreads_; }

	/**
	 *	This method returns the number of threads that are running that are not
	 *	pending for deletion.
	 */
	int numUnstoppedThreads() const { return numUnstoppedThreads_; }

	/**
	 *	This method returns if the BgTaskManager should lock its threads on only
	 *	one core.
	 */
	bool exclusive() const { return exclusive_; }

	// Used by background tasks.
	void onThreadFinished( BackgroundTaskThread * pThread ); // In main thread
	BackgroundTaskPtr pullBackgroundTask(); // In background thread
	void onBackgroundTaskFinished( BackgroundTaskPtr task,
			uint64 timeStarted ); // In background thread

	#ifdef _WIN32
	const BW::vector<HANDLE> & getThreadHandles() { return threadHandles_; }
	#endif

	/**
	 *	The (trivial) callback type used to check if a BackgroundTaskThread
	 *	should be blocked.
	 *
	 *	@note may be called from any thread without locking
	 */
	typedef void ThreadBlockCallback();

	/**
	 *	This method is called to see if we're currently running inside a cancelled
	 *	task.
	 *	Any long-running code may call it. It should be very quick, but may block
	 *  or suspend the thread if the BgTaskManager for the current task so wishes.
	 *
	 *	@note can be called from anywhere without locking, should be really fast
	 *	@note unless the thread is blocked.
	 */
	CSTDMF_DLL static bool shouldAbortTask();

	/**
	 *	This method is used to set up the static pointers used for task abort or
	 *	thread blocking
	 */
	// Overriders must call up to this function
	CSTDMF_DLL virtual void setStaticThreadData( const BackgroundTaskPtr pTask );

	/**
	 *	This method sets the callback to query for blocking threads owned by
	 *	this BgTaskManager.
	 */
	CSTDMF_DLL void setThreadBlockCallback( ThreadBlockCallback* pCallback );

	unsigned int bgTaskListSize() const { return bgTaskList_.size(); }
	unsigned int fgTaskListSize() const { return fgTaskListSize_; }

private:
	static volatile unsigned int s_threadIndex_;

	/**
	 *	A priority list of tasks waiting for a background thread to process
	 *	them.
	 *	It uses a semaphore to prevent more pulls than pushes.
	 */
	class BackgroundTaskList
	{
	public:
		typedef BW::list< std::pair< int, BackgroundTaskPtr > > List;

		BackgroundTaskList();
		void push( BackgroundTaskPtr pTask, int priority = DEFAULT );
		void push( const List & tasks );
		void pushStopTask();
		BackgroundTaskPtr pull();
		bool isEmpty() const;
		void clearRealTasks( TaskManager & mgr );
		unsigned int size() const;
		void warnSize( uint val ) { warnSize_ = val; }
		void warnStepSize( uint val ) { warnStepSize_ = val; }

	private:
		List list_;
		uint warnSize_;
		uint warnStepSize_;
		uint currentWarnSize_;
		mutable SimpleMutex mutex_;
		SimpleSemaphore semaphore_;
	};

	BackgroundTaskList			bgTaskList_;
	BackgroundTaskList::List	delayedBgTaskList_;

	typedef BW::list< BackgroundTaskPtr > ForegroundTaskList;
	ForegroundTaskList fgTaskList_;
	ForegroundTaskList newTasks_;
	unsigned int fgTaskListSize_;
	mutable SimpleMutex fgTaskListMutex_;

	int numRunningThreads_;
	int numUnstoppedThreads_;

	#ifdef _WIN32
	BW::vector<HANDLE> threadHandles_;
	#endif

	ThreadBlockCallback* pThreadBlockCallback_;
	mutable ReadWriteLock pThreadBlockCallbackReadWriteLock_;
	static THREADLOCAL( volatile bool* ) s_pTaskStopped_;
	static THREADLOCAL( ThreadBlockCallback** ) s_pThreadBlockCallback_;

	mutable SimpleMutex activeBgTasksMutex_;
	BW::vector<BackgroundTaskPtr> activeBgTasks_;

	bool exclusive_;

#if ENABLE_WATCHERS
	TaskWatchers taskWatchers_;
#endif // ENABLE_WATCHERS
};
//------------------------------------------------------------------------------
class BgTaskManager : public InitSingleton<BgTaskManager>, public TaskManager
{
public:
	friend class BackgroundTaskThread;

	CSTDMF_DLL BgTaskManager( bool exclusive = true ) : TaskManager( exclusive ) {};
	CSTDMF_DLL ~BgTaskManager() {};

	CSTDMF_DLL static bool init()
	{
		return InitSingleton<BgTaskManager>::init();
	}

	CSTDMF_DLL static bool fini()
	{
		return InitSingleton<BgTaskManager>::fini();
	}

	CSTDMF_DLL static bool finalised()
	{
		return InitSingleton<BgTaskManager>::finalised();
	}

	CSTDMF_DLL static BgTaskManager & instance()
	{
		return Singleton<BgTaskManager>::instance();
	}

	CSTDMF_DLL static BgTaskManager * pInstance()
	{
		return Singleton<BgTaskManager>::pInstance();
	}

};

//------------------------------------------------------------------------------
class FileIOTaskManager : public InitSingleton<FileIOTaskManager>, public TaskManager
{
public:
	friend class BackgroundTaskThread;

	CSTDMF_DLL FileIOTaskManager( bool exclusive = true ) : TaskManager( exclusive ) {};
	CSTDMF_DLL ~FileIOTaskManager() {};
	CSTDMF_DLL static bool init()
	{
		return InitSingleton<FileIOTaskManager>::init();
	}
	CSTDMF_DLL static bool fini()
	{
		return InitSingleton<FileIOTaskManager>::fini();
	}

	CSTDMF_DLL static bool finalised()
	{
		return InitSingleton<FileIOTaskManager>::finalised();
	}

	CSTDMF_DLL static FileIOTaskManager & instance()
	{
		return Singleton<FileIOTaskManager>::instance();
	}

	CSTDMF_DLL static FileIOTaskManager * pInstance()
	{
		return Singleton<FileIOTaskManager>::pInstance();
	}

};

BW_END_NAMESPACE

#endif // BGTASK_MANAGER_HPP
