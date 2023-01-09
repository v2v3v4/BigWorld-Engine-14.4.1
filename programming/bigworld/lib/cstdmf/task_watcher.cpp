#include "pch.hpp"

#include "debug.hpp"
#include "task_watcher.hpp"

DECLARE_DEBUG_COMPONENT2( "CStdMF", 0 )

BW_BEGIN_NAMESPACE

#if ENABLE_WATCHERS


/**
 *	Constructor.
 */
TaskWatcher::TaskWatcher():
	numTasks_( 0 ),
	totalTime_( 0 ),
	totalTimeInQueue_( 0 ),
	totalTimeInThread_( 0 ),
	decayedMaxTime_( /*bias*/ 0.99f ),
	averageTime_( /*bias*/ 0.99f ),
	averageTimeInQueue_( /*bias*/ 0.99f ),
	averageTimeInThread_( /*bias*/ 0.99f )
{
}


/**
 *	This task updates metrics.
 */
void TaskWatcher::update( double duration, double timeInQueue,
		double timeInThread )
{
	++numTasks_;
	totalTime_ += duration;
	totalTimeInQueue_ += timeInQueue;
	totalTimeInThread_ += timeInThread;

	decayedMaxTime_.highWaterSample( (float) duration );

	averageTime_.sample( (float) totalTime_ / numTasks_ );
	averageTimeInQueue_.sample( (float) totalTimeInQueue_ / numTasks_ );
	averageTimeInThread_.sample( (float) totalTimeInThread_ / numTasks_ );
}


WatcherPtr TaskWatcher::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();

		pWatcher->addChild( "numTasks",
			makeWatcher( &TaskWatcher::numTasks_ ) );

		pWatcher->addChild( "totalTime",
			makeWatcher( &TaskWatcher::totalTime_ ) );

		pWatcher->addChild( "totalTimeInQueue",
			makeWatcher( &TaskWatcher::totalTimeInQueue_ ) );

		pWatcher->addChild( "totalTimeInThread",
			makeWatcher( &TaskWatcher::totalTimeInThread_ ) );

		pWatcher->addChild( "decayedMaxTime",
			makeWatcher( &TaskWatcher::decayedMaxTime ) );

		pWatcher->addChild( "averageTime",
			makeWatcher( &TaskWatcher::averageTime ) );

		pWatcher->addChild( "averageTimeInQueue",
			makeWatcher( &TaskWatcher::averageTimeInQueue ) );

		pWatcher->addChild( "averageTimeInThread",
			makeWatcher( &TaskWatcher::averageTimeInThread ) );
	}

	return pWatcher;
}


TaskWatchers::TaskWatchers():
	isInitialised_( false ),
	warnThreshold_( 0 )
{
}


bool TaskWatchers::init( SimpleMutex & mutex, const char * bgPath,
		const char * mtPath, float warnThreshold )
{
	MF_ASSERT( !isInitialised_ );

	typedef MapWatcher< StringHashMap< TaskWatcher > > TaskMapWatcher;

	TaskWatchers * pNull = NULL;
	Watcher & watcher = Watcher::rootWatcher();

	// Background task watchers
	WatcherPtr pBackgroundWatcher = new TaskMapWatcher(
			pNull->backgroundWatchers_ );

	pBackgroundWatcher->addChild( "*", TaskWatcher::pWatcher() );

	// Make it thread safe
	WatcherPtr pSafeBackgroundWatcher = new SafeWatcher(
			pBackgroundWatcher, mutex );

	if (!watcher.addChild( bgPath, pSafeBackgroundWatcher, (void*)this ) )
	{
		return false;
	}

	// Main thread task watchers
	WatcherPtr pMainThreadWatcher = new TaskMapWatcher(
			pNull->mainThreadWatchers_ );

	pMainThreadWatcher->addChild( "*", TaskWatcher::pWatcher() );

	if (!watcher.addChild( mtPath, pMainThreadWatcher, (void*)this ) )
	{
		watcher.removeChild( bgPath );
		return false;
	}

	warnThreshold_ = warnThreshold;
	isInitialised_ = true;

	return isInitialised_;
}


/**
 *	This method updates task processing metrics. Metrics are collated by all
 *	and by task type.
 */
void TaskWatchers::update( const char * taskName, double duration,
		double timeInThread, bool isBackgroundThread )
{
	double timeInQueue = duration - timeInThread;

	StringHashMap< TaskWatcher > & watchers =
		isBackgroundThread ?  backgroundWatchers_ : mainThreadWatchers_;

	TaskWatcher & watcher = watchers[ taskName ];
	watcher.update( duration, timeInQueue, timeInThread );

	TaskWatcher & watcherAll = watchers[ "All" ];
	watcherAll.update( duration, timeInQueue, timeInThread );

	if (!isZero<float>( warnThreshold_ ) && duration > warnThreshold_)
	{
		WARNING_MSG( "%s took %.2f seconds:\n"
					"\tTime in queue:             %.2f seconds\n"
					"\tTime in %s thread: %.2f seconds\n",
				taskName, duration, timeInQueue,
				isBackgroundThread ? "background" : "main",
				timeInThread );
	}
}

#endif // ENABLE_WATCHERS

BW_END_NAMESPACE

// task_watchers.cpp
