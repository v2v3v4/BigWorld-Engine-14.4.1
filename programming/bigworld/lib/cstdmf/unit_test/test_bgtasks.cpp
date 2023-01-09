#include "pch.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/bgtask_manager.hpp"

#include <stdio.h>


BW_BEGIN_NAMESPACE

class MyTask : public BackgroundTask
{
public:
	MyTask(
		int & bgCount,
		int & fgCount,
		int & delCount,
		int & stopCount,
		int & cancelCount,
		bool doSleep = false ) :
		BackgroundTask( "MyTask" ),

		bgCount_( bgCount ),
		fgCount_( fgCount ),
		delCount_( delCount ),
		stopCount_( stopCount ),
		cancelCount_( cancelCount ),
		doSleep_( doSleep )
	{}

	virtual ~MyTask()
	{
		SimpleMutexHolder lock( delCountMutex_ );
		++delCount_;
	}

	virtual void doBackgroundTask( TaskManager & mgr )
	{
		// Make it sleep so that hopefully some of the tests have not finished
		// before stopAll is called
		if (doSleep_)
		{
#ifdef _WIN32
			Sleep( 1 );
#else
			usleep( 1000 );
#endif
		}

		SimpleMutexHolder lock( bgCountMutex_ );
		++bgCount_;

		if (TaskManager::shouldAbortTask())
		{
			SimpleMutexHolder lock( stopCountMutex_ );
			++stopCount_;
			mgr.addMainThreadTask( this );
			return;
		}

		mgr.addMainThreadTask( this );
	}

	virtual void doMainThreadTask( TaskManager & mgr )
	{
		// Sleep
		if (doSleep_)
		{
#ifdef _WIN32
			Sleep( 1 );
#else
			usleep( 1000 );
#endif
		}

		SimpleMutexHolder lock( fgCountMutex_ );
		++fgCount_;
	}

	virtual void cancel( TaskManager & mgr )
	{
		SimpleMutexHolder lock( cancelCountMutex_ );
		++cancelCount_;
	}

	int bgCount() const
	{
		SimpleMutexHolder lock( bgCountMutex_ );
		return bgCount_;
	}

	int fgCount() const
	{
		SimpleMutexHolder lock( fgCountMutex_ );
		return fgCount_;
	}

private:
	int & bgCount_;
	static SimpleMutex bgCountMutex_;

	int & fgCount_;
	static SimpleMutex fgCountMutex_;

	int & delCount_;
	static SimpleMutex delCountMutex_;

	int & stopCount_;
	static SimpleMutex stopCountMutex_;

	int & cancelCount_;
	static SimpleMutex cancelCountMutex_;

	bool doSleep_;
};

SimpleMutex MyTask::bgCountMutex_;
SimpleMutex MyTask::fgCountMutex_;
SimpleMutex MyTask::delCountMutex_;
SimpleMutex MyTask::stopCountMutex_;
SimpleMutex MyTask::cancelCountMutex_;

/**
 *	Check that TaskManager runs as would be used in a typical program.
 */

TEST( TaskManager_run )
{
	int bgCount = 0;
	int fgCount = 0;
	int delCount = 0;
	int stopCount = 0;
	int cancelCount = 0;
	TaskManager mgr;

	const int THREAD_COUNT = 10;
	mgr.startThreads( "Test thread", THREAD_COUNT );

	const int TASK_COUNT = 1000;
	for (int i = 0; i < TASK_COUNT; ++i)
	{
		mgr.addBackgroundTask( new MyTask(
			bgCount, fgCount, delCount, stopCount, cancelCount ) );
	}

	while (mgr.hasTasks())
	{
		mgr.tick();

#ifdef _WIN32
		Sleep( 100 );
#else
		usleep( 100000 );
#endif
	}

	// -- Threads
	// Tasks finished, threads waiting
	int numRunningThreads = mgr.numRunningThreads();
	CHECK_EQUAL( THREAD_COUNT, numRunningThreads );

	int numUnstoppedThreads = mgr.numUnstoppedThreads();
	CHECK_EQUAL( THREAD_COUNT, numUnstoppedThreads );

	// -- Tasks
	// Should be no tasks left
	bool hasTasks = mgr.hasTasks();
	CHECK( !hasTasks );

	// Background tasks should all be done
	CHECK_EQUAL( TASK_COUNT, bgCount );

	// Foreground/cleanup not all done yet
	CHECK( ( fgCount >= 0 ) && ( fgCount <= TASK_COUNT ) );
	CHECK( ( delCount >= 0 ) && ( delCount <= TASK_COUNT ) );
	CHECK_EQUAL( fgCount, delCount );

	// None were stopped/cancelled
	CHECK_EQUAL( 0, stopCount );
	CHECK_EQUAL( 0, cancelCount );
}

/**
 *	Check that stopAll can wait for all threads to finish.
 */
TEST( TaskManager_stopWait )
{
	int bgCount = 0;
	int fgCount = 0;
	int delCount = 0;
	int stopCount = 0;
	int cancelCount = 0;
	TaskManager mgr;

	const int THREAD_COUNT = 10;
	mgr.startThreads( "Test thread", THREAD_COUNT );

	const int TASK_COUNT = 1000;
	for (int i = 0; i < TASK_COUNT; ++i)
	{
		// Make half the tasks sleep
		bool doSleep = ( ( i % 2 ) == 0 );
		mgr.addBackgroundTask( new MyTask(
			bgCount, fgCount, delCount, stopCount, cancelCount, doSleep ) );
	}

	mgr.stopAll( /* discardPendingTasks: */false, /* waitForThreads: */true );

	// -- Threads
	CHECK_EQUAL( 0, mgr.numRunningThreads() );
	CHECK_EQUAL( 0, mgr.numUnstoppedThreads() );

	// -- Tasks
	// Should be no tasks left
	bool hasTasks = mgr.hasTasks();
	CHECK( !hasTasks );
	int numBgTasksLeft = mgr.numBgTasksLeft();
	CHECK_EQUAL( 0, numBgTasksLeft );

	// Everything was finished and cleaned up
	CHECK_EQUAL( TASK_COUNT, bgCount );
	CHECK_EQUAL( TASK_COUNT, fgCount );
	CHECK_EQUAL( TASK_COUNT, delCount );

	// None were stopped/cancelled
	CHECK_EQUAL( 0, stopCount );
	CHECK_EQUAL( 0, cancelCount );
}

/**
 *	Check that stopAll can cancel tasks and wait for all threads to finish.
 */
TEST( TaskManager_stopCancelWait )
{
	int bgCount = 0;
	int fgCount = 0;
	int delCount = 0;
	int stopCount = 0;
	int cancelCount = 0;
	TaskManager mgr;

	const int THREAD_COUNT = 10;
	mgr.startThreads( "Test thread", THREAD_COUNT );

	const int TASK_COUNT = 1000;
	for (int i = 0; i < TASK_COUNT; ++i)
	{
		// Make half the tasks sleep
		bool doSleep = ( ( i % 2 ) == 0 );
		mgr.addBackgroundTask( new MyTask(
			bgCount, fgCount, delCount, stopCount, cancelCount, doSleep ) );
	}

	mgr.stopAll( /* discardPendingTasks: */true, /* waitForThreads: */true );

	// -- Threads
	CHECK_EQUAL( 0, mgr.numRunningThreads() );
	CHECK_EQUAL( 0, mgr.numUnstoppedThreads() );

	// -- Tasks
	// Tasks were cleared
	bool hasTasks = mgr.hasTasks();
	CHECK( !hasTasks );
	int numBgTasksLeft = mgr.numBgTasksLeft();
	CHECK_EQUAL( 0, numBgTasksLeft );

	// Not all were run
	CHECK( bgCount <= TASK_COUNT );
	CHECK( fgCount <= TASK_COUNT );

	// All the ones that were run must have had a chance to clean up on the
	// main thread
	CHECK_EQUAL( fgCount, bgCount );

	// All must be deleted
	CHECK_EQUAL( TASK_COUNT, delCount );

	// Some of them might have been running
	CHECK( ( stopCount >= 0 ) && ( stopCount <= bgCount ) );

	// Some should be cancelled before they could be run
	CHECK_EQUAL( cancelCount + bgCount, TASK_COUNT );
}

/**
 *	Check that stopAll can stop all threads.
 */
TEST( TaskManager_stopNoWait )
{
	int bgCount = 0;
	int fgCount = 0;
	int delCount = 0;
	int stopCount = 0;
	int cancelCount = 0;
	TaskManager mgr;

	const int THREAD_COUNT = 10;
	mgr.startThreads( "Test thread", THREAD_COUNT );

	const int TASK_COUNT = 1000;
	for (int i = 0; i < TASK_COUNT; ++i)
	{
		// Make half the tasks sleep
		bool doSleep = ( ( i % 2 ) == 0 );
		mgr.addBackgroundTask( new MyTask(
			bgCount, fgCount, delCount, stopCount, cancelCount, doSleep ) );
	}

	mgr.stopAll( /* discardPendingTasks: */false, /* waitForThreads: */false );

	// -- Threads
	// May not have stopped yet
	int numRunningThreads = mgr.numRunningThreads();
	CHECK( ( numRunningThreads ) >= 0 && ( numRunningThreads <= THREAD_COUNT ) );
	CHECK_EQUAL( 0, mgr.numUnstoppedThreads() );

	// -- Tasks
	// Not all were run/cleared
	CHECK( bgCount <= TASK_COUNT );
	CHECK( fgCount <= TASK_COUNT );

	// None were stopped/cancelled
	CHECK_EQUAL( 0, stopCount );
	CHECK_EQUAL( 0, cancelCount ); 
}

/**
 *	Check that stopAll can stop all threads.
 */
TEST( TaskManager_stopCancelNoWait )
{
	int bgCount = 0;
	int fgCount = 0;
	int delCount = 0;
	int stopCount = 0;
	int cancelCount = 0;
	TaskManager mgr;

	const int THREAD_COUNT = 10;
	mgr.startThreads( "Test thread", THREAD_COUNT );

	const int TASK_COUNT = 1000;
	for (int i = 0; i < TASK_COUNT; ++i)
	{
		// Make half the tasks sleep
		bool doSleep = ( ( i % 2 ) == 0 );
		mgr.addBackgroundTask( new MyTask(
			bgCount, fgCount, delCount, stopCount, cancelCount, doSleep ) );
	}

	mgr.stopAll( /* discardPendingTasks: */true, /* waitForThreads: */false );

	// -- Threads
	// May not have stopped yet
	int numRunningThreads = mgr.numRunningThreads();
	CHECK( ( numRunningThreads ) >= 0 && ( numRunningThreads <= THREAD_COUNT ) );
	int numUnstoppedThreads = mgr.numUnstoppedThreads();
	CHECK_EQUAL( 0, numUnstoppedThreads );

	// -- Tasks
	// Could still have tasks running, because we didn't wait

	// Not all were run
	CHECK( bgCount <= TASK_COUNT );
	CHECK( fgCount <= TASK_COUNT );

	// All must be deleted
	CHECK( delCount <= TASK_COUNT );

	// Some of them might have been running
	CHECK( ( stopCount >= 0 ) && ( stopCount <= bgCount ) );

	// Some should be cancelled before they could be run
	// But the bgCounts might not have finished yet
	CHECK( cancelCount + bgCount <= TASK_COUNT );
}

/**
 *	Check starting and stopping threads.
 */
TEST( TaskManager_startStop )
{
	int bgCount = 0;
	int fgCount = 0;
	int delCount = 0;
	int stopCount = 0;
	int cancelCount = 0;
	TaskManager mgr;

	const int THREAD_COUNT = 10;
	mgr.startThreads( "Test thread", THREAD_COUNT );

	const int TASK_COUNT = 1000;
	for (int i = 0; i < TASK_COUNT / 2; ++i)
	{
		mgr.addBackgroundTask( new MyTask(
			bgCount, fgCount, delCount, stopCount, cancelCount ) );
	}
	mgr.stopThreads( THREAD_COUNT / 2 );
	for (int i = 0; i < TASK_COUNT / 2; ++i)
	{
		mgr.addBackgroundTask( new MyTask(
			bgCount, fgCount, delCount, stopCount, cancelCount ) );
	}

	while (mgr.hasTasks())
	{
		mgr.tick();

#ifdef _WIN32
		Sleep( 100 );
#else
		usleep( 100000 );
#endif
	}

	// -- Threads
	// Tasks finished, threads waiting
	int numRunningThreads = mgr.numRunningThreads();
	CHECK_EQUAL( THREAD_COUNT / 2, numRunningThreads );

	int numUnstoppedThreads = mgr.numUnstoppedThreads();
	CHECK_EQUAL( THREAD_COUNT / 2, numUnstoppedThreads );

	// -- Tasks
	// Should be no tasks left
	bool hasTasks = mgr.hasTasks();
	CHECK( !hasTasks );
	int numBgTasksLeft = mgr.numBgTasksLeft();
	CHECK_EQUAL( 0, numBgTasksLeft );

	// Background tasks should all be done
	CHECK_EQUAL( TASK_COUNT, bgCount );

	// Foreground/cleanup not all done yet
	CHECK( ( fgCount >= 0 ) && ( fgCount <= TASK_COUNT ) );
	CHECK( ( delCount >= 0 ) && ( delCount <= TASK_COUNT ) );
	CHECK_EQUAL( fgCount, delCount );

	// None were stopped/cancelled
	CHECK_EQUAL( 0, stopCount );
	CHECK_EQUAL( 0, cancelCount );
}

BW_END_NAMESPACE

// test_bgtasks.cpp
