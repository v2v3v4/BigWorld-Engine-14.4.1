#ifndef STREAMING_MANAGER_HPP
#define STREAMING_MANAGER_HPP


#include "cstdmf/stdmf.hpp"
#include "cstdmf/bgtask_manager.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

class StreamingManager;

typedef SmartPointer< class BaseStreamingTask > IStreamingTaskPtr;

class BaseStreamingTask : public BackgroundTask
{
public:
	BaseStreamingTask();
	virtual ~BaseStreamingTask();

	enum QueueType
	{
		MAIN_THREAD,
		BACKGROUND_THREAD,
		IO_THREAD
	};

	void queue();
	void forceExecute();
	void streamingManager( StreamingManager * streamingManager );
	StreamingManager * streamingManager() const;

	size_t bandwidthEstimate();
	size_t bandwidthUsed();
	
protected:
	void executeStreaming( QueueType currentQueue, 
		bool forceComplete = false );

	virtual bool doStreaming( QueueType currentQueue ) = 0;
	virtual QueueType doQueue() = 0;
	virtual void doComplete();
	virtual size_t doEstimateBandwidth();
	
	virtual void doBackgroundTask( TaskManager & mgr );
	virtual void doMainThreadTask( TaskManager & mgr );

	void reportCompletion();
	void reportBandwidthUsed( size_t bytes );

	size_t bandwidthUsed_;
	size_t bandwidthEstimate_;
	StreamingManager * streamingManager_;
};

class StreamingManager
{
public:
	StreamingManager();

	void registerTask( BaseStreamingTask * pTask );
	void registerImmediateTask( BaseStreamingTask * pTask );
	void reportCompletion( BaseStreamingTask * pTask );
	void tick();
	void clear();

protected:

	void queueTask( BaseStreamingTask* pTask );

private:
	
	class QuotaMonitor
	{
	public:
		QuotaMonitor();

		void setQuotaPerSecond( float quota );

		void reportQuotaUsage( float usage );
		float currentUsage();
		float quotaRemaining();

		void tick( double currentTime );

		float quotaPerSecond_;
		float usedQuota_;
		double lastTickTime_;
	};
	
	QuotaMonitor diskBandwidthMonitor_;
	BW::list<IStreamingTaskPtr> queue_;
	mutable SimpleMutex queueLock_;
	
	// Watchers
	uint32 maxConcurrentTasks_;
	uint32 numConcurrentTasks_;
	float diskBandwidthMB_;
	float diskBandwidthUsedMB_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif // STREAMING_MANAGER_HPP
