#ifndef EVENT_DISPATCHER_HPP
#define EVENT_DISPATCHER_HPP

#ifdef _WIN32
	#ifdef _XBOX360
		#include <winsockx.h>
	#else
		#include <Winsock2.h> // for fdset
	#endif
#endif

#include "interfaces.hpp"

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/timestamp.hpp"
#include "cstdmf/watcher.hpp"

// #include "cstdmf/time_queue.hpp"

BW_BEGIN_NAMESPACE

template <class TYPE> class TimeQueueT;
typedef TimeQueueT< uint64 > TimeQueue64;

namespace Mercury
{

class DispatcherCoupling;
class ErrorReporter;
class EventPoller;
class FrequentTask;
class FrequentTasks;


/**
 *	This class is responsible for dispatching events. These include timers,
 *	network activity and file activity events.
 */
class EventDispatcher
{
public:
	EventDispatcher();
	~EventDispatcher();

	void prepareForShutdown();

	void processContinuously();
	int  processOnce( bool shouldIdle = false );
	void processUntilBreak();
	void processUntilSignalled( bool & signal );

	void breakProcessing( bool breakState = true );
	bool processingBroken() const;

	void attach( EventDispatcher & childDispatcher );
	void detach( EventDispatcher & childDispatcher );

	bool registerFileDescriptor( int fd,
		InputNotificationHandler * handler,
		const char * name = "NotNamed" );
	bool deregisterFileDescriptor( int fd );
	
	bool registerWriteFileDescriptor( int fd,
		InputNotificationHandler * handler,
		const char * name = "NotNamed" );
	bool deregisterWriteFileDescriptor( int fd );

	INLINE TimerHandle addTimer( int64 microseconds, 
		TimerHandler * handler, void * arg = NULL, 
		const char * name = "UnnamedTimer" );
	INLINE TimerHandle addOnceOffTimer( int64 microseconds,
		TimerHandler * handler, void * arg = NULL,
		const char * name = "UnnamedTimer" );

	void addFrequentTask( FrequentTask * pTask );
	bool cancelFrequentTask( FrequentTask * pTask );

	uint64 timerDeliveryTime( TimerHandle handle ) const;
	uint64 timerIntervalTime( TimerHandle handle ) const;
	uint64 & timerIntervalTime( TimerHandle handle );

	uint64 getSpareTime() const;
	void clearSpareTime();
	double proportionalSpareTime() const;

	INLINE double maxWait() const;
	INLINE void maxWait( double seconds );

	ErrorReporter & errorReporter()	{ return *pErrorReporter_; }
	
	
	int numChildren() const
	{
		return static_cast<int>( childDispatchers_.size() );
	}

#if ENABLE_WATCHERS
	static WatcherPtr pTimingWatcher();
	static WatcherPtr pWatcher();
#endif

private:
	TimerHandle addTimerCommon( int64 microseconds,
		TimerHandler * handler,
		void * arg,
		bool recurrent,
		const char * name );

	void processFrequentTasks();
	void processTimers();
	void processStats();
	int processNetwork( bool shouldIdle );

	double calculateWait() const;

	void attachTo( EventDispatcher & parentDispatcher );
	void detachFrom( EventDispatcher & parentDispatcher );

	bool breakProcessing_;

	TimeQueue64 * pTimeQueue_;

	FrequentTasks * pFrequentTasks_;

	// Statistics
	TimeStamp		accSpareTime_;
	TimeStamp		oldSpareTime_;
	TimeStamp		totSpareTime_;
	TimeStamp		lastStatisticsGathered_;

	uint32 numTimerCalls_;

	double maxWait_;

	DispatcherCoupling * pCouplingToParent_;

	typedef BW::vector< EventDispatcher * > ChildDispatchers;
	ChildDispatchers childDispatchers_;

	EventPoller * pPoller_;

	ErrorReporter * pErrorReporter_;
};

} // namespace Mercury

#ifdef CODE_INLINE
#include "event_dispatcher.ipp"
#endif

BW_END_NAMESPACE

#endif // EVENT_DISPATCHER_HPP
