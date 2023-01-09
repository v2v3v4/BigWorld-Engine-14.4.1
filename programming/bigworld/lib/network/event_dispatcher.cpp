#include "pch.hpp"

#include "event_dispatcher.hpp"

#include "error_reporter.hpp"
#include "event_poller.hpp"
#include "dispatcher_coupling.hpp"
#include "frequent_tasks.hpp"

#include "cstdmf/concurrency.hpp"
#include "cstdmf/timestamp.hpp"
#include "cstdmf/time_queue.hpp"
#include "cstdmf/profiler.hpp"

#include <string.h>

#ifdef PLAYSTATION3
#include "ps3_compatibility.hpp"
#endif


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "event_dispatcher.ipp"
#endif

namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: Construction
// -----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning( push )
// C4355: 'this' : used in base member initializer list
#pragma warning( disable: 4355 )
#endif // _MSC_VER

/**
 *	Constructor.
 */
EventDispatcher::EventDispatcher() :
	breakProcessing_( false ),
	pTimeQueue_( new TimeQueue64 ),
	pFrequentTasks_( new FrequentTasks ),
	accSpareTime_( 0 ),
	oldSpareTime_( 0 ),
	totSpareTime_( 0 ),
	lastStatisticsGathered_( 0 ),
	numTimerCalls_( 0 ),
	maxWait_( 0.1 ),
	pCouplingToParent_( NULL ),
	pPoller_( EventPoller::create() ),
	pErrorReporter_( new ErrorReporter( *this ) )
{
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif // _MSC_VER

/**
 *	Destructor.
 */
EventDispatcher::~EventDispatcher()
{
	if (pCouplingToParent_ != NULL)
	{
		WARNING_MSG( "EventDispatcher::~EventDispatcher: "
				"Still coupled to parent dispatcher\n" );
		bw_safe_delete(pCouplingToParent_);
	}

	bw_safe_delete(pErrorReporter_);
	bw_safe_delete(pPoller_);
	bw_safe_delete(pFrequentTasks_);

	if (!pTimeQueue_->empty())
	{
		NOTICE_MSG( "EventDispatcher()::~EventDispatcher: Num timers = %d\n",
			pTimeQueue_->size() );
	}

	pTimeQueue_->clear( /*shouldCallOnRelease:*/ false );

	bw_safe_delete(pTimeQueue_);
}


/**
 * Prepare for shutdown
 */
void EventDispatcher::prepareForShutdown()
{
	const uint64 SECONDS_TO_ATTEMPT_SEND = 2;

	const uint64 startTime = timestamp();
	uint64 timePeriod = (stampsPerSecond() * SECONDS_TO_ATTEMPT_SEND);

	while (!pPoller_->isReadyForShutdown() &&
		(timestamp() - startTime < timePeriod))
	{
		this->processOnce();

		// Wait 100ms
#if defined( PLAYSTATION3 )
		sys_timer_usleep( 100000 );
#elif !defined( _WIN32 )
		usleep( 100000 );
#else
		Sleep( 100 );
#endif
	}
}


// -----------------------------------------------------------------------------
// Section: Coupling
// -----------------------------------------------------------------------------

/**
 *	This method attaches a child dispatcher to this dispatcher.
 */
void EventDispatcher::attach( EventDispatcher & childDispatcher )
{
	childDispatcher.attachTo( *this );

	childDispatchers_.push_back( &childDispatcher );
}


/**
 *	 This private method attaches this dispatcher to a parent dispatcher. It
 *	 is called by EventDispatcher::attach.
 */
void EventDispatcher::attachTo( EventDispatcher & parentDispatcher )
{
	MF_ASSERT( pCouplingToParent_ == NULL );
	pCouplingToParent_ = new DispatcherCoupling( parentDispatcher, *this );

	int fd = pPoller_->getFileDescriptor();

	if (fd != -1)
	{
		parentDispatcher.registerFileDescriptor( fd, pPoller_,
			"EventDispatcher" );
		parentDispatcher.registerWriteFileDescriptor( fd, pPoller_,
			"EventDispatcher" );
	}
}


/**
 *	This method detaches a child dispatcher to this dispatcher.
 */
void EventDispatcher::detach( EventDispatcher & childDispatcher )
{
	childDispatcher.detachFrom( *this );

	ChildDispatchers & d = childDispatchers_;
	d.erase( std::remove( d.begin(), d.end(), &childDispatcher ), d.end() );
}


 /**
 *	 This private method detaches this dispatcher to a parent dispatcher. It
 *	 is called by EventDispatcher::detach.
 */
void EventDispatcher::detachFrom( EventDispatcher & parentDispatcher )
{
	int fd = pPoller_->getFileDescriptor();

	if (fd != -1)
	{
		parentDispatcher.deregisterFileDescriptor( fd );
		parentDispatcher.deregisterWriteFileDescriptor( fd );
	}

	MF_ASSERT( pCouplingToParent_ != NULL );
	bw_safe_delete(pCouplingToParent_);
}


// -----------------------------------------------------------------------------
// Section: File descriptor related
// -----------------------------------------------------------------------------

/**
 * 	This method registers a file descriptor with the event dispatcher. The
 * 	handler is called every time input is detected on that file descriptor.
 * 	Unlike timers, these handlers are only called if you use
 * 	processContinuously
 *
 *	@param fd			The file descriptor to register
 *	@param handler		The handler to receive notification messages.
 *	@param name			The name of the input handler.
 *
 * 	@return true if descriptor could be registered
 */
bool EventDispatcher::registerFileDescriptor( int fd,
	InputNotificationHandler * handler, const char * name )
{
	return pPoller_->registerForRead( fd, handler, name );
}


/**
 * 	This method registers a file descriptor with the event dispatcher. The
 * 	handler is called every time a write event is detected on that file
 * 	descriptor. Unlike timers, these handlers are only called if you use
 * 	processContinuously.
 *
 *	@param fd			The file descriptor to register
 *	@param handler		The handler to receive notification messages.
 *	@param name			The name of the input handler.
 *
 * 	@return true if descriptor could be registered
 */
bool EventDispatcher::registerWriteFileDescriptor( int fd,
	InputNotificationHandler * handler, const char * name )
{
	return pPoller_->registerForWrite( fd, handler, name );
}


/**
 * 	This method stops watching out for input on this file descriptor.
 *
 *	@param fd	The fd to stop watching.
 *
 * 	@return true if descriptor could be deregistered.
 */
bool EventDispatcher::deregisterFileDescriptor( int fd )
{
	return pPoller_->deregisterForRead( fd );
}



/**
 * 	This method stops watching out for write events on this file descriptor.
 *
 *	@param fd	The fd to stop watching.
 *
 * 	@return true if descriptor could be deregistered.
 */
bool EventDispatcher::deregisterWriteFileDescriptor( int fd )
{
	return pPoller_->deregisterForWrite( fd );
}


// -----------------------------------------------------------------------------
// Section: Timer related
// -----------------------------------------------------------------------------

/**
 *	This is a helper method that creates timers.
 *	@see registerTimer
 *	@see registerCallback
 */
TimerHandle EventDispatcher::addTimerCommon( int64 microseconds,
	TimerHandler * handler,
	void * arg,
	bool recurrent,
	const char * name )
{
	MF_ASSERT( handler );

	if (microseconds <= 0)
		return TimerHandle();

	uint64 interval = TimeStamp::fromSeconds(
			((double)microseconds) / 1000000.0 );

	// TODO: Does not handle time wrap-around.
	TimerHandle handle = pTimeQueue_->add( timestamp() + interval,
			recurrent ? interval : 0,
			handler, arg, name );

	return handle;
}


/**
 * 	This method returns the amount of spare time in the last statistics period.
 * 	This should only be used for monitoring statistics, as it may
 * 	be up to a whole period (currently 1s) out of date, and does
 * 	not assure the same accuracy that the methods above do.
 */
double EventDispatcher::proportionalSpareTime() const
{
	double ret = (double)(int64)(totSpareTime_ - oldSpareTime_);
	return ret/stampsPerSecondD();
}


// -----------------------------------------------------------------------------
// Section: FrequentTasks
// -----------------------------------------------------------------------------


/**
 *	This method adds a task that will be called back regularly.
 */
void EventDispatcher::addFrequentTask( FrequentTask * pTask )
{
	pFrequentTasks_->add( pTask );
}


/**
 *	This method removes a FrequentTask.
 */
bool EventDispatcher::cancelFrequentTask( FrequentTask * pTask )
{
	return pFrequentTasks_->cancel( pTask );
}


// -----------------------------------------------------------------------------
// Section: Loop processing
// -----------------------------------------------------------------------------

/**
 *	This method calls doTask on all registered FrequentTasks objects.
 */
void EventDispatcher::processFrequentTasks()
{
	PROFILER_SCOPED( processFrequentTasks );
	pFrequentTasks_->process();
}


/**
 *	This method processes outstanding timers.
 */
void EventDispatcher::processTimers()
{
	PROFILER_SCOPED( processTimers );
	numTimerCalls_ += pTimeQueue_->process( timestamp() );
}


void EventDispatcher::processStats()
{
	PROFILER_SCOPED( processStats );
	// gather statistics if we haven't for a while
	if (timestamp() - lastStatisticsGathered_ >= stampsPerSecond())
	{
		oldSpareTime_ = totSpareTime_;
		totSpareTime_ = accSpareTime_ + pPoller_->spareTime();

		lastStatisticsGathered_ = timestamp();
	}
}

/**
 *	This method processes any activity on the network.
 *
 *	@param shouldIdle If set to true, this method will block until the next
 *	timer is due if there is nothing waiting on the network.
 *	@return	Number of file descriptors that triggered handlers.
 */
int EventDispatcher::processNetwork( bool shouldIdle )
{
	PROFILER_SCOPED( processNetwork );
	// select for network activity until earliest timer
	double maxWait = shouldIdle ? this->calculateWait() : 0.0;

	return pPoller_->processPendingEvents( maxWait );
}


/**
 *	This method returns the amount of time in seconds until the next timer
 *	event.
 */
double EventDispatcher::calculateWait() const
{
	double maxWait = maxWait_;

	if (!pTimeQueue_->empty())
	{
		maxWait = std::min( maxWait,
			pTimeQueue_->nextExp( timestamp() ) / stampsPerSecondD() );
	}

	ChildDispatchers::const_iterator iter = childDispatchers_.begin();

	while (iter != childDispatchers_.end())
	{
		maxWait = std::min( maxWait, (*iter)->calculateWait() );
		++iter;
	}

	return maxWait;
}


/**
 * 	This method processes events continuously until interrupted by a call to
 * 	breakProcessing.
 *
 *	@see breakProcessing
 */
void EventDispatcher::processContinuously()
{
	breakProcessing_ = false;

	while (!breakProcessing_)
	{
		this->processOnce( /* shouldIdle */ true );
	}
}


/**
 *	This method processes the current events.
 *
 *	@param shouldIdle	If set to true, this method will block until the next
 *						timer is due if there is nothing waiting on the network.
 *
 *	@return 			The number of network events processed.
 */
int EventDispatcher::processOnce( bool shouldIdle /* = false */ )
{
	breakProcessing_ = false;

	this->processFrequentTasks();

	if (!breakProcessing_)
	{
		this->processTimers();
	}

	this->processStats();

	if (!breakProcessing_)
	{
		return this->processNetwork( shouldIdle );
	}

	return 0;
}


/**
 *	This method call processContinuously until breakProcessing is called.
 */
void EventDispatcher::processUntilBreak()
{
	this->processContinuously();
	pErrorReporter_->reportPendingExceptions( true /* reportBelowThreshold */ );
}

/**
 *	This method calls @ref processOnce until @ref breakProcessing is called or
 *	the provided flag is signalled (set to @a true).
 *	@param singal	Flag to interrupt processing from within a handler.
 */
void EventDispatcher::processUntilSignalled( bool & signal )
{
	while (!breakProcessing_ && !signal)
	{
		this->processOnce( /* shouldIdle */ true );
	}
	pErrorReporter_->reportPendingExceptions( true /* reportBelowThreshold */ );
}


/**
 *	This method returns the time that the given timer handle will be delivered,
 *	in timestamps.
 */
uint64 EventDispatcher::timerDeliveryTime( TimerHandle handle ) const
{
	return pTimeQueue_->timerDeliveryTime( handle );
}


/**
 *	This method returns the time between deliveries of the given timer handle.
 */
uint64 EventDispatcher::timerIntervalTime( TimerHandle handle ) const
{
	return pTimeQueue_->timerIntervalTime( handle );
}


/**
 *	This method returns the time between deliveries of the given timer handle.
 *	The value returned may be modified.
 */
uint64 & EventDispatcher::timerIntervalTime( TimerHandle handle )
{
	return pTimeQueue_->timerIntervalTime( handle );
}


/**
 * 	This method returns the amount of time (in timestamps) that the event
 * 	dispatcher has been idle. Currently the only activity it does in its spare
 * 	time is 'select'. This can be used as a high-response measure of unused
 * 	processor time.
 */
uint64 EventDispatcher::getSpareTime() const
{
	return pPoller_->spareTime();
}


/**
 * 	This method resets the spare time counter.
 */
void EventDispatcher::clearSpareTime()
{
	accSpareTime_ += pPoller_->spareTime();
	pPoller_->clearSpareTime();
}


#if ENABLE_WATCHERS
/**
 * 	This method returns the generic watcher for nub/timing EventDispatcher.
 */
WatcherPtr EventDispatcher::pTimingWatcher()
{
	static DirectoryWatcherPtr watchMe = NULL;

	if (watchMe == NULL)
	{
		watchMe = new DirectoryWatcher();

		watchMe->addChild( "spareTime",
			makeWatcher( &EventDispatcher::proportionalSpareTime ) );
		watchMe->addChild( "totalSpareTime",
			makeWatcher( &EventDispatcher::totSpareTime_ ) );
		watchMe->addChild( "numTimerCalls",
			makeWatcher( &EventDispatcher::numTimerCalls_ ) );
	}

	return watchMe;
}

/**
 * 	This method returns the generic watcher for the EventDispatcher.
 */
WatcherPtr EventDispatcher::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();

		EventDispatcher * pNull = NULL;

		pWatcher->addChild( "numChildren",
			makeWatcher( &EventDispatcher::numChildren ) );

		pWatcher->addChild( "poller",
			new BaseDereferenceWatcher( EventPoller::pWatcher() ),
				&pNull->pPoller_ );
	}

	return pWatcher;
}
#endif /* #if ENABLE_WATCHER */


} // namespace Mercury

BW_END_NAMESPACE

// event_dispatcher.cpp
