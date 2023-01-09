#include "pch.hpp"

#include "bgtask_manager.hpp"
#include "profiler.hpp"
#include "guard.hpp"

#include <sstream>

DECLARE_DEBUG_COMPONENT2( "CStdMF", 0 )

BW_BEGIN_NAMESPACE

/*static*/ volatile unsigned int TaskManager::s_threadIndex_ = 0;
/*static*/ THREADLOCAL( volatile bool* ) TaskManager::s_pTaskStopped_ = NULL;
/*static*/ THREADLOCAL( TaskManager::ThreadBlockCallback** )
								TaskManager::s_pThreadBlockCallback_ = NULL;

int BgTaskManager_token = 1;

BW_INIT_SINGLETON_STORAGE(BgTaskManager);
BW_INIT_SINGLETON_STORAGE(FileIOTaskManager);

// -----------------------------------------------------------------------------
// Section: BackgroundTask
// -----------------------------------------------------------------------------

//#define FORCE_MAIN_THREAD

/**
 *	Constructor.
 */
CStyleBackgroundTask::CStyleBackgroundTask( const char* name,
		FUNC_PTR bgFunc, void * bgArg, FUNC_PTR fgFunc, void * fgArg ) :
	BackgroundTask( name ),
	bgFunc_( bgFunc ),
	bgArg_( bgArg ),
	fgFunc_( fgFunc ),
	fgArg_( fgArg )
{
}


/**
 *	Constructor.
 */
CStyleBackgroundTask::CStyleBackgroundTask( FUNC_PTR bgFunc, void * bgArg,
		FUNC_PTR fgFunc, void * fgArg ) :
	BackgroundTask( "CStyleBackgroundTask" ),
	bgFunc_( bgFunc ),
	bgArg_( bgArg ),
	fgFunc_( fgFunc ),
	fgArg_( fgArg )
{
}


/**
 *	This method is called in a background thread to perform this task. The task
 *	manager is passed in. At the end of the background task, this object is
 *	added back as a main thread task, if necessary.
 */
void CStyleBackgroundTask::doBackgroundTask( TaskManager & mgr )
{
	if (bgFunc_)
	{
		(*bgFunc_)( bgArg_ );
	}

	if (fgFunc_)
	{
		mgr.addMainThreadTask( this );
	}
}


/**
 *	This method override BackgroundTask and is called in the main thread
 *	once the background task has finished.
 */
void CStyleBackgroundTask::doMainThreadTask( TaskManager & mgr )
{
	if (fgFunc_)
	{
		(*fgFunc_)( fgArg_ );
	}
}


// -----------------------------------------------------------------------------
// Section: BackgroundTaskThread
// -----------------------------------------------------------------------------

/**
 * BackgroundTaskThread
 */
BackgroundTaskThread::BackgroundTaskThread(
		TaskManager & mgr,
		BackgroundThreadDataPtr pData ) :
	SimpleThread(),
	mgr_( mgr ),
	pData_( pData )
{
	this->SimpleThread::init(
		BackgroundTaskThread::s_start, this, "Unknown BG Task" );
}

/**
 * BackgroundTaskThread
 */
BackgroundTaskThread::BackgroundTaskThread(
		TaskManager & mgr,
		BackgroundThreadDataPtr pData,
		const BW::string& threadName ) :
	SimpleThread(),
	mgr_( mgr ),
	pData_( pData )
{
	this->SimpleThread::init(
		BackgroundTaskThread::s_start, this, threadName );
}


/**
 *	This static method is the entry-point for the background thread.
 */
void BackgroundTaskThread::s_start( void * arg )
{
	BackgroundTaskThread * pTaskThread = (BackgroundTaskThread*)arg;
	pTaskThread->run();
}


/**
 *	This method is the main function of the background thread.
 *	Called at the end of BackgroundTaskThread::s_start.
 */
void BackgroundTaskThread::run()
{
	bool shouldRun = pData_ ? pData_->onStart( *this ) : true;

	while (shouldRun)
	{
		// This waits on a semaphore for a task to be ready to pull
		// Push a "stop" task to the manager to cancel this thread
		BackgroundTaskPtr pTask = mgr_.pullBackgroundTask();

		if (pTask)
		{
#if ENABLE_PROFILER
			ScopedProfiler scopedProfile( pTask->name() );
#endif
			#ifdef FORCE_MAIN_THREAD
			MF_ASSERT( 0 );
			#endif

			mgr_.setStaticThreadData( pTask );
			// Give the BgTaskManager a chance to block our thread before
			// we start a task
			if (TaskManager::s_pThreadBlockCallback_ &&
				*TaskManager::s_pThreadBlockCallback_)
			{
				(*TaskManager::s_pThreadBlockCallback_)();
			}

			uint64 timeStarted = timestamp();

			pTask->doBackgroundTask( mgr_, this );

			mgr_.setStaticThreadData( NULL );

			mgr_.onBackgroundTaskFinished( pTask, timeStarted );
		}
		// A NULL task indicates that the thread should terminate
		else
		{
			shouldRun = false;
		}
	}

	if (pData_)
	{
		pData_->onEnd( *this );
	}

	// Inform the main thread that this thread has terminated.
	mgr_.addMainThreadTask( new ThreadFinisher( this ) );
}

/**
 *	This method tells the caller the number of background tasks that are
 *	either being processed or queued.
 *	Does not include foreground tasks.
 */
unsigned int TaskManager::numBgTasksLeft() const
{
	SimpleMutexHolder smh( activeBgTasksMutex_ );
	size_t numBgTasksLeft = bgTaskList_.size() + activeBgTasks_.size();
	MF_ASSERT( numBgTasksLeft <= UINT_MAX );
	return ( unsigned int )numBgTasksLeft;
}

/**
 *	Check if we have tasks to process.
 *	You should also check that you have background threads available to process
 *	these tasks.
 */
bool TaskManager::hasTasks() const
{
	SimpleMutexHolder smh1( activeBgTasksMutex_ );
	SimpleMutexHolder smh2( fgTaskListMutex_ );

	return !bgTaskList_.isEmpty() ||
		!activeBgTasks_.empty() ||
		!fgTaskList_.empty();
}


/**
 *	This method sets the maximum size of the background task list. A warning
 *	message will be emitted when the size reaches the warning size, and warnings
 *	will also be issued when the size increases by a multiple of
 *	'warningStepSize'
 */
void TaskManager::setTaskListWarningSize( uint warningSize,
		uint warningStepSize )
{
	bgTaskList_.warnSize( warningSize );
	bgTaskList_.warnStepSize( warningStepSize );
}


// -----------------------------------------------------------------------------
// Section: ThreadFinisher
// -----------------------------------------------------------------------------

/**
 *	This method informs the thread manager that a thread has finished.
 */
void ThreadFinisher::doMainThreadTask( TaskManager & mgr )
{
	mgr.onThreadFinished( pThread_ );
}


// -----------------------------------------------------------------------------
// Section: BackgroundTaskList
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */

TaskManager::BackgroundTaskList::BackgroundTaskList() :
		warnSize_( 1000 ),
		warnStepSize_( 100 ),
		currentWarnSize_( warnSize_ )
{
}


/**
 *	This method adds a task to the list of tasks in a thread-safe way.
 *
 *	@param pTask	The task to add
 *	@param priority	The priority to add this task at. A higher value means that
 *		the task is done earlier.
 */
void TaskManager::BackgroundTaskList::push( BackgroundTaskPtr pTask,
												int priority )
{
	{
		SimpleMutexHolder holder( mutex_ );

		// Insert with highest priority value tasks first
		List::reverse_iterator iter = list_.rbegin();

		while ((iter != list_.rend()) && (priority > iter->first))
		{
			++iter;
		}

		list_.insert( iter.base(), std::make_pair( priority, pTask ) );

		if (list_.size() >= currentWarnSize_)
		{
			WARNING_MSG( "TaskManager::BackgroundTaskList::push: "
						"Task list has %d pending tasks. "
						"The warning size is %d.\n",
					(int)list_.size(), (int)warnSize_);
			currentWarnSize_ += warnStepSize_;
		}

	}

	// Add item to semaphore count
	// Do last because background tasks wait on the semaphore, not the mutex
	semaphore_.push();
}


/**
 *	This method adds a list of tasks to the list of tasks in a thread-safe way.  
 * 
 *	@param tasks	The list of tasks to add 
 */ 
void TaskManager::BackgroundTaskList::push( const List & tasks )
{
	{
		SimpleMutexHolder holder( mutex_ );
		list_.insert( list_.end(), tasks.begin(), tasks.end() );
	}

	semaphore_.push( static_cast<int>(tasks.size()) );
}


/**
 *	This method adds a stop task to the list of tasks in a thread-safe way.
 *	Pushing a NULL task marks that a thread should stop. It will add it to the
 *	task list with DEFAULT priority.
 */
void TaskManager::BackgroundTaskList::pushStopTask()
{
	this->push( NULL, DEFAULT );
}


/**
 *	This method pulls a task from the list of tasks in a thread-safe way. It
 *	waits on a semaphore if no tasks exist.
 */
BackgroundTaskPtr TaskManager::BackgroundTaskList::pull()
{
	// Makes thread wait until there is a task in the queue for it
	// But we can't wait inside the mutex because then we can't add new tasks
	// or clear old tasks

	semaphore_.pull();

	// Get task off queue
	mutex_.grab();

	MF_ASSERT( !list_.empty() );
	BackgroundTaskPtr pTask = list_.front().second;
	list_.pop_front();

	if (list_.size() < warnSize_ && currentWarnSize_ != warnSize_)
	{
		NOTICE_MSG( "TaskManager::BackgroundTaskList::pull: "
				"Task list has returned to below the warning size.\n");
		currentWarnSize_ = warnSize_;
	}

	mutex_.give();

	return pTask;
}

/**
 *	This method tells the caller if the list is empty.
 *	Includes both real tasks and "stop" tasks (same as number on semaphore).
 */
bool TaskManager::BackgroundTaskList::isEmpty() const
{
	SimpleMutexHolder holder( mutex_ );
	return list_.empty();
}

/**
 *	This method clears the task list.
 *	Clears the real tasks only, not the "stop" tasks.
 *	Note that background tasks could also be being grabbed by background threads
 *	while this is clearing.
 */
void TaskManager::BackgroundTaskList::clearRealTasks( TaskManager & mgr )
{
	size_t numStop = 0;

	// Editing list
	mutex_.grab();

	// Clear list
	// There could be threads waiting on semaphore_.pull() and they might get
	// it before this while loop is done
	// Each time we decrement the semaphore, take one item off the back of the
	// list
	// Keep stop (NULL) tasks or their threads will never be stopped
	while (!list_.empty())
	{
		if (!semaphore_.pullTry())
		{
			// We can't assume the list is empty, so break here.
			break;
		}

		// Pop from the back because other threads might be in the process of
		// doing a pull and we want to preserve ordering
		BackgroundTaskPtr pTask = list_.back().second;

		if (pTask)
		{
			pTask->cancel( mgr );
		}
		else
		{
			++numStop;
		}

		list_.pop_back();
	}

	// Done changing list
	mutex_.give();

	// We can't actually assume the list is empty now, another thread might
	// have pulled a task, but not taken it off the list yet

	// Push the "stop" tasks back on the semaphore
	// Stop tasks all have the same priority so we just needed to add them up
	for ( size_t i = 0; i < numStop; ++i )
	{
		this->pushStopTask();
	}
}

/**
 *	This method returns the size of the task list.
 *	Includes both real tasks and "stop" tasks.
 */
unsigned int TaskManager::BackgroundTaskList::size() const
{
	SimpleMutexHolder holder( mutex_ );

	size_t size = list_.size();
	MF_ASSERT( size <= UINT_MAX );
	return ( unsigned int )size;
}


// -----------------------------------------------------------------------------
// Section: BgTaskManager
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
TaskManager::TaskManager( bool exclusive ) :
	fgTaskListSize_( 0 ),
	numRunningThreads_( 0 ),
	numUnstoppedThreads_( 0 ),
	pThreadBlockCallback_( NULL ),
	pThreadBlockCallbackReadWriteLock_(),
	exclusive_( exclusive )
{
}


/**
 *	Destructor.
 */
TaskManager::~TaskManager()
{
	this->stopAll();
}


/**
 * This method enables watchers for task meterics
 */
bool TaskManager::initWatchers( const char * path, float warnThreshold )
{
#if ENABLE_WATCHERS
	BW::ostringstream bgPath;
	bgPath << "tasks/" << path << "/background";

	BW::ostringstream fgPath;
	fgPath << "tasks/" << path << "/mainThread";

	if (!taskWatchers_.init(
				activeBgTasksMutex_,
				bgPath.str().c_str(),
				fgPath.str().c_str(),
				warnThreshold ))
	{
		ERROR_MSG( "TaskManager::initWatchers: Failed to init TaskWatchers with "
				"path tasks/%s\n", path );
		return false;
	}

	bgPath << "QueueSize";
	fgPath << "QueueSize";

	MF_WATCH( bgPath.str().c_str(), *this, &TaskManager::bgTaskListSize );
	MF_WATCH( fgPath.str().c_str(), *this, &TaskManager::fgTaskListSize );

	return true;
#else
	return false;
#endif
}


/**
 *	This method starts the background threads.
 *
 *	@param threadNamePrefix name for thread.
 *	@param numThreads The number of threads to add to the thread pool.
 *	@param pData Per thread data.
 *	
 *	@note don't call stop while starting.
 */
void TaskManager::startThreads(
	const BW::string& threadNamePrefix,
	int numThreads,
	BackgroundThreadDataPtr pData )
{
	BW_GUARD;

	numUnstoppedThreads_ += numThreads;
	numRunningThreads_ += numThreads;

	for (int i = 0; i < numThreads; ++i)
	{
		// Name thread
		BW::ostringstream oss;
		oss << threadNamePrefix << " (" << s_threadIndex_ << ")";
		++s_threadIndex_;

		//DEBUG_MSG( "Starting thread %s\n", oss.str().c_str() );

		// This object deletes itself
		BackgroundTaskThread * pThread =
			new BackgroundTaskThread( *this, pData, oss.str() );

#ifdef _WIN32
		threadHandles_.push_back( pThread->handle() );
#else
		// Stop compiler warning of unused variable
		(void)pThread;
#endif
	}
}

/**
 *	This method starts the background threads.
 *
 *	@param numThreads The number of threads to add to the thread pool.
 *	@param pData Per thread data.
 *	
 *	@note don't call stop while starting.
 */
void TaskManager::startThreads(
	int numThreads,
	BackgroundThreadDataPtr pData )
{
	this->startThreads( "Unknown BG Task", numThreads, pData );
}

/**
 *	This method expand or shrink the thread pool to the
 *	target number.
 *	@note don't call while stopping or starting.
 */
void TaskManager::setNumberOfThreads( int numThreads )
{
	// Too many threads, stop some
	if (numUnstoppedThreads_ > numThreads)
	{
		this->stopThreads( numUnstoppedThreads_ - numThreads );
	}
	// Not enough threads, start some
	else if (numUnstoppedThreads_ < numThreads)
	{
		BW::ostringstream oss;
		oss << "TaskManager::setNumberOfThreads: " << numThreads;
		this->startThreads( oss.str(), numThreads - numUnstoppedThreads_ );
	}
	// Otherwise if just the right number, do nothing
}


/**
 *	This method stops a certain numbers of threads.
 *	Threads will only be stopped once the current lot of tasks have finished
 *	though.
 *	@note don't call while starting threads.
 */
void TaskManager::stopThreads( int numThreads )
{
	numUnstoppedThreads_ -= numThreads;

	for (int i = 0; i < numThreads; ++i)
	{
		// Special task indicates that a thread should shut down.
		bgTaskList_.pushStopTask();
	}
}


/**
 *	This method stops all threads. This method should only be called by the main
 *	thread.
 *
 *	@param discardPendingTasks If true, tasks that have yet to be added to a
 *		background thread will be deleted.
 *  @param waitForThreads	If true, this method blocks until all
 *                          the owned threads have terminated.
 *	
 *	@note don't call while starting threads.
 */
void TaskManager::stopAll( bool discardPendingTasks, bool waitForThreads )
{


	// Discard any pending tasks
	// Do this before stopping threads, because "stop" tasks are pulled last
	if (discardPendingTasks)
	{
		// Do not clear "stop" tasks that haven't been pulled yet!
		// or they will never stop and we wait forever if waitForThreads is true
		bgTaskList_.clearRealTasks( *this );
	}

	// Stop threads
	this->stopThreads( numUnstoppedThreads_ );
	MF_ASSERT( numUnstoppedThreads_ == 0 );

	if (waitForThreads && numRunningThreads_ > 0)
	{
		TRACE_MSG( "TaskManager::stopAll: Waiting for threads: %d\n",
			numRunningThreads_ );

		while (true)
		{
			this->tick();

			if (numRunningThreads_)
			{
				if (discardPendingTasks)
				{
					SimpleMutexHolder smh( activeBgTasksMutex_ );

					while (!activeBgTasks_.empty())
					{
						activeBgTasks_.back()->stop();
						activeBgTasks_.pop_back();
					}
				}
#ifdef _WIN32
				Sleep( 100 );
#else
				usleep( 100000 );
#endif
			}
			else
			{
				break;
			}
		}
	}

	// Clear again, in case addBackgroundTask was called while we were waiting
	// Now there are no more background threads running, so addBackgroundTask
	// shouldn't be called randomly
	if (discardPendingTasks)
	{
		bgTaskList_.clearRealTasks( *this );
	}

	// do a tick to stop all foreground tasks
	// Note that more foreground tasks could be added while it's stopping
	// the current list
	this->tick();
}


/**
 *	This method adds a task that should be processed by a background thread
 *	into a thread safe work queue.
 *
 *	BackgroundTaskPtr is a SmartPointer, so you can use
 *	addBackgroundTask( new MyTask() );
 *	and it will be safely deleted.
 *	If your task adds itself to the foreground list for cleanup, then it'll be
 *	dropped at the end of a BgTaskManager::tick().
 *	
 *	@param pTask	The task to add.
 *	@param priority	The priority to add this task at. A higher value means that
 *		the task is done earlier.
 *	
 *	@note any thread can access this.
 */
void TaskManager::addBackgroundTask( BackgroundTaskPtr pTask, int priority )
{
#ifdef FORCE_MAIN_THREAD
	pTask->doBackgroundTask( *this, NULL );
#else
	pTask->timeEnqueuedToBackground_ = timestamp();
	bgTaskList_.push( pTask, priority );
#endif
}


/**
 *	This method adds a task that should be processed by a background thread.
 *	
 *	@note any thread can access this.
 */
void TaskManager::addMainThreadTask( BackgroundTaskPtr pTask )
{
	fgTaskListMutex_.grab();
	pTask->timeEnqueuedToMainThread_ = timestamp();
	fgTaskList_.push_back( pTask );
	fgTaskListMutex_.give();
}


/**
 *	This method adds a task that should be processed by a background thread
 *	into a non thread safe intermediate work queue. Delaying into the
 *	intermediate queue avoids locking the background queue.
 *
 *	@param pTask	The task to add.
 */
void TaskManager::addDelayedBackgroundTask( BackgroundTaskPtr pTask )
{
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

#ifdef FORCE_MAIN_THREAD
	pTask->doBackgroundTask( *this, NULL );
#else
	pTask->timeEnqueuedToBackground_ = timestamp();
	delayedBgTaskList_.push_back( std::make_pair( 0/*priority*/, pTask ) );
#endif
}


/**
 *	This method should be called periodically in the main thread. It processes
 *	all foreground tasks.
 *	
 *	@note should only be called by the owning thread.
 */
void TaskManager::tick()
{
	fgTaskListMutex_.grab();
	fgTaskListSize_ = (unsigned int) fgTaskList_.size();
	fgTaskList_.swap( newTasks_ );
	fgTaskListMutex_.give();

	ForegroundTaskList::iterator iter = newTasks_.begin();

	while (iter != newTasks_.end())
	{
		BackgroundTaskPtr pTask = *iter;
		++iter;

#if ENABLE_WATCHERS
		uint64 timeStarted = timestamp();
#endif

		pTask->doMainThreadTask( *this );

#if ENABLE_WATCHERS
		uint64 now = timestamp();
		double duration = stampsToSeconds(
				now - pTask->timeEnqueuedToMainThread_ );
		double timeInThread = stampsToSeconds( now - timeStarted );

		taskWatchers_.update( pTask->name_, duration, timeInThread,
			false /*isBackgroundThread*/ );
#endif
	}

	newTasks_.clear();
	// Smartpointers to tasks that have finished get deleted here

	if (!delayedBgTaskList_.empty())
	{
		bgTaskList_.push( delayedBgTaskList_ );
		delayedBgTaskList_.clear();
	}
}


/**
 *	This method is called to inform the manager that a thread has finished.
 *	
 *	@note should be called by owning thread only.
 */
void TaskManager::onThreadFinished( BackgroundTaskThread * pThread )
{
	--numRunningThreads_;

#ifdef _WIN32
	BW::vector<HANDLE>::iterator it = std::find(
		threadHandles_.begin(),
		threadHandles_.end(),
		pThread->handle() );

	if (it != threadHandles_.end())
	{
		threadHandles_.erase( it );
	}
#endif

	bw_safe_delete(pThread);
	TRACE_MSG( "TaskManager::onThreadFinished: "
		"Thread finished. %d remaining\n", numRunningThreads_ );
}


/**
 *	This method pulls a task from the list of background tasks in a thread-safe
 *	way. It waits on a semaphore if no tasks exist.
 *	
 *	@note should be called by background threads only.
 */
BackgroundTaskPtr TaskManager::pullBackgroundTask()
{
	BackgroundTaskPtr task = bgTaskList_.pull();

	if (task)
	{
		activeBgTasksMutex_.grab();

		activeBgTasks_.push_back( task );

		activeBgTasksMutex_.give();
	}

	return task;
}


/**
 *	This method tells the BgTaskManager that a task has finished
 *	in the background.
 *	
 *	@note should be called by background threads only.
 */
void TaskManager::onBackgroundTaskFinished( BackgroundTaskPtr pTask,
		uint64 timeStarted )
{
	activeBgTasksMutex_.grab();

	BW::vector<BackgroundTaskPtr>::iterator iter =
		std::find( activeBgTasks_.begin(), activeBgTasks_.end(), pTask );

	if (iter != activeBgTasks_.end())
	{
		activeBgTasks_.erase( iter );
	}

#if ENABLE_WATCHERS
	uint64 now = timestamp();
	double duration = stampsToSeconds( now - pTask->timeEnqueuedToBackground_ );
	double timeInThread = stampsToSeconds( now - timeStarted );

	taskWatchers_.update( pTask->name_, duration, timeInThread,
		true /*isBackgroundThread*/ );
#endif

	activeBgTasksMutex_.give();
}


/**
 *	This method tells gives the calling thread a chance to abort early
 *	if it's running a stopped task.
 *	
 *	@note may be called from any threads without locking, but should be
 *	@note really really fast in the fast-path (not stopped, no callback)
 */
/*static*/ bool TaskManager::shouldAbortTask()
{
	if (s_pTaskStopped_ && *s_pTaskStopped_)
		return true;
	if (s_pThreadBlockCallback_ && *s_pThreadBlockCallback_) 
	{
		(*s_pThreadBlockCallback_)();
		if ( s_pTaskStopped_ && *s_pTaskStopped_ )
			return true;
	}
	return false;
}


/**
 *	This method records a pointer to the volatile bool * indicating
 *	if this task should abort, and the pointer to a callback pointer
 *  to let a task or thread be suspended.
 *
 *	@note will be called from background threads without locking,
 *	@note but only updates thread-local static variables.
 */
void TaskManager::setStaticThreadData( const BackgroundTaskPtr pTask )
{
	if (pTask != NULL)
	{
		s_pTaskStopped_ = &(pTask->stop_);
		ReadWriteLock::ReadGuard readGuard( pThreadBlockCallbackReadWriteLock_ );
		s_pThreadBlockCallback_ = &this->pThreadBlockCallback_;
	}
	else
	{
		s_pTaskStopped_ = NULL;
		s_pThreadBlockCallback_ = NULL;
	}
}


/**
 *	This method records a pointer to the function for threads owned
 *	by this manager to call when they are in an opportune place to
 *	block.
 */
void TaskManager::setThreadBlockCallback( ThreadBlockCallback* pCallback )
{
	ReadWriteLock::WriteGuard writeGuard( pThreadBlockCallbackReadWriteLock_ );
	pThreadBlockCallback_ = pCallback;
}

BW_END_NAMESPACE

// bgtask_manager.cpp
