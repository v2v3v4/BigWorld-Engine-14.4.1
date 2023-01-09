#ifndef TASK_WATCHER_HPP
#define TASK_WATCHER_HPP

#include "config.hpp"

#if ENABLE_WATCHERS

#include "watcher.hpp"
#include "stringmap.hpp"

#include "math/ema.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class collate and watch task metrics.
 */
class TaskWatcher
{
public:

	TaskWatcher();

	void update( double duration, double timeInQueue, double timeInThread );

	static WatcherPtr pWatcher();

private:

	double decayedMaxTime() const { return decayedMaxTime_.average(); }
	double averageTime() const { return averageTime_.average(); }
	double averageTimeInQueue() const { return averageTimeInQueue_.average(); }
	double averageTimeInThread() const
	{
		return averageTimeInThread_.average();
	}

	uint numTasks_;
	double totalTime_;
	double totalTimeInQueue_;
	double totalTimeInThread_;

	EMA decayedMaxTime_;
	EMA averageTime_;
	EMA averageTimeInQueue_;
	EMA averageTimeInThread_;
};

class TaskWatchers
{
public:

	TaskWatchers();

	bool init( SimpleMutex & mutex, const char * bgPath, const char * mtPath,
			float warnThreshold );

	void update( const char * taskName, double duration, double timeInThread,
			bool isBackgroundThread );

private:

	bool isInitialised_;
	float warnThreshold_;

	BW::string backgroundPath_;
	BW::string mainThreadPath_;

	StringHashMap< TaskWatcher > backgroundWatchers_;
	StringHashMap< TaskWatcher > mainThreadWatchers_;
};

BW_END_NAMESPACE

#endif // ENABLE_WATCHERS

#endif // TASK_WATCHER_HPP
