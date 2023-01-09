#include "pch.hpp"

#include "streaming_manager.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

StreamingManager::StreamingManager() :
	maxConcurrentTasks_( 10000 ),
	numConcurrentTasks_( 0 )
{
	diskBandwidthMB_ = 100.0f;
	diskBandwidthMonitor_.setQuotaPerSecond( 
		diskBandwidthMB_ * 1024.0f * 1024.0f );

	MF_WATCH( "TextureStreaming/Query_DiskBandwidthUsed_MB", 
		diskBandwidthUsedMB_, 
		Watcher::WT_READ_ONLY, 
		"Disk bandwidth used for streaming" );
	MF_WATCH( "TextureStreaming/Config_DiskBandwidth", 
		diskBandwidthMB_, 
		Watcher::WT_READ_WRITE, 
		"Disk bandwidth assigned to streaming (MB per second)" );
	MF_WATCH( "TextureStreaming/Query_ConcurrentTasks", 
		numConcurrentTasks_, 
		Watcher::WT_READ_ONLY, 
		"Current number of concurrent tasks" );
	MF_WATCH( "TextureStreaming/Config_MaxConcurrentTasks", 
		maxConcurrentTasks_, 
		Watcher::WT_READ_WRITE, 
		"Maximum number of concurrent tasks at a time" );
}

void StreamingManager::registerTask( BaseStreamingTask * pTask )
{
	queueTask( pTask );
}

void StreamingManager::clear()
{
	queue_.clear();
}

void StreamingManager::registerImmediateTask( BaseStreamingTask * pTask )
{
	MF_ASSERT( pTask->streamingManager() == NULL );
	pTask->streamingManager( this );
	pTask->forceExecute();
}

void StreamingManager::reportCompletion( BaseStreamingTask * pTask )
{
	// Completed yay!
	--numConcurrentTasks_;

	float bandwidthUsed = static_cast< float >( pTask->bandwidthUsed() );
	float bandwidthEstimate = static_cast< float >( pTask->bandwidthEstimate() );

	diskBandwidthMonitor_.reportQuotaUsage( bandwidthUsed - bandwidthEstimate );
}

void StreamingManager::queueTask( BaseStreamingTask * pTask )
{
	// Drop the task into a queue or 
	// queue it immediately if it there is bandwidth available
	SimpleMutexHolder smh(queueLock_);

	pTask->streamingManager( this );

	// Determine if we can execute any new tasks
	float remainingQuota = diskBandwidthMonitor_.quotaRemaining();
	if (remainingQuota > 0.0f &&
		numConcurrentTasks_ < maxConcurrentTasks_ &&
		queue_.empty())
	{
		diskBandwidthMonitor_.reportQuotaUsage( 
			static_cast< float >( pTask->bandwidthEstimate() ) );
		++numConcurrentTasks_;
		pTask->queue();
	}
	else
	{
		queue_.push_back( pTask );
	}
}

void StreamingManager::tick()
{
	SimpleMutexHolder smh(queueLock_);

	// Update the bandwidth configuration
	diskBandwidthMonitor_.setQuotaPerSecond( 
		diskBandwidthMB_ * 1024.0f * 1024.0f );
	const float bytesToMegaBytesConversion = 1.0f / 1024.0f / 1024.0f;
	diskBandwidthUsedMB_ = diskBandwidthMonitor_.currentUsage() *
		bytesToMegaBytesConversion;
	
	// Tick the bandwidth monitoring system
	double currentTime = TimeStamp::toSeconds( timestamp() );
	diskBandwidthMonitor_.tick( currentTime );

	// Determine if we can execute any new tasks
	float remainingQuota = diskBandwidthMonitor_.quotaRemaining();

	// Queue some tasks to be done until we're over the quota
	while (remainingQuota > 0.0f &&
		numConcurrentTasks_ < maxConcurrentTasks_ &&
		!queue_.empty())
	{
		// The entry is now at the back (since the pop)
		IStreamingTaskPtr pTask = queue_.front();
		queue_.pop_front();

		float estimate = static_cast< float >( pTask->bandwidthEstimate() );
		remainingQuota -= estimate;
		diskBandwidthMonitor_.reportQuotaUsage( estimate );
		++numConcurrentTasks_;
		pTask->queue();
	}
}

StreamingManager::QuotaMonitor::QuotaMonitor() :
	quotaPerSecond_( 1024.0f * 1024.0f * 1024.0f ), // 1 TB
	lastTickTime_( 0.0f ),
	usedQuota_( 0.0f )
{
	
}

void StreamingManager::QuotaMonitor::setQuotaPerSecond( float quota )
{
	quotaPerSecond_ = quota;
}

void StreamingManager::QuotaMonitor::reportQuotaUsage( float usage )
{
	usedQuota_ += usage;
}

float StreamingManager::QuotaMonitor::currentUsage()
{
	return usedQuota_;
}

void StreamingManager::QuotaMonitor::tick( double currentTime )
{
	if (lastTickTime_ != 0.0f)
	{
		float deltaTime = static_cast< float >( currentTime - lastTickTime_ );
		usedQuota_ = max( 0.0f, usedQuota_ - (deltaTime * quotaPerSecond_));
	}
	lastTickTime_ = currentTime;
}

float StreamingManager::QuotaMonitor::quotaRemaining()
{
	return max( 0.0f, quotaPerSecond_ - usedQuota_ );
}

BaseStreamingTask::BaseStreamingTask() : 
	BackgroundTask( "Streaming Task" ), 
	bandwidthUsed_( 0 ), 
	bandwidthEstimate_( 0 ), 
	streamingManager_( NULL )
{
}

BaseStreamingTask::~BaseStreamingTask()
{
}

void BaseStreamingTask::queue()
{
	QueueType queue = doQueue();
	switch (queue)
	{
	case MAIN_THREAD:
		BgTaskManager::instance().addMainThreadTask( this );
		break;

	case BACKGROUND_THREAD:
		BgTaskManager::instance().addBackgroundTask( this );
		break;

	case IO_THREAD:
		BgTaskManager::instance().addBackgroundTask( this );
		break;

	default:
		// No valid queue to assign to, flag it as completed.
		reportCompletion();
	}
}

void BaseStreamingTask::forceExecute()
{
	executeStreaming( MAIN_THREAD, true );
}

void BaseStreamingTask::streamingManager( StreamingManager * streamingManager )
{
	streamingManager_ = streamingManager;
}

StreamingManager * BaseStreamingTask::streamingManager() const
{
	return streamingManager_;
}

size_t BaseStreamingTask::doEstimateBandwidth()
{
	return 0;
}

size_t BaseStreamingTask::bandwidthEstimate()
{
	// Cache the estimate
	if (bandwidthEstimate_ == 0)
	{
		bandwidthEstimate_ = doEstimateBandwidth();
	}
	return bandwidthEstimate_;
}

size_t BaseStreamingTask::bandwidthUsed()
{
	return bandwidthUsed_;
}

void BaseStreamingTask::executeStreaming( QueueType currentQueue, 
	bool forceComplete )
{
	while (currentQueue == doQueue() || forceComplete)
	{
		bool completed = doStreaming( currentQueue );
		if (completed)
		{
			reportCompletion();
			return;
		}
	}
	
	// Not completed yet? but needs requeueing 
	queue();
}

void BaseStreamingTask::doBackgroundTask( TaskManager & mgr ) 
{
	executeStreaming( BACKGROUND_THREAD );
}

void BaseStreamingTask::doMainThreadTask( TaskManager & mgr ) 
{
	executeStreaming( MAIN_THREAD );
}

void BaseStreamingTask::doComplete()
{
	// Do nothing by default, may be used to report to another class
}

void BaseStreamingTask::reportCompletion()
{
	doComplete();

	if (streamingManager_)
	{
		streamingManager_->reportCompletion( this );
	}

	streamingManager_ = NULL;
}

void BaseStreamingTask::reportBandwidthUsed( size_t bytes )
{
	bandwidthUsed_ += bytes;
}

} // namespace Moo

BW_END_NAMESPACE

// texture_manager.cpp
