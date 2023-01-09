#include "worker_thread.hpp"

#include "cstdmf/timestamp.hpp"

#include <algorithm>

DECLARE_DEBUG_COMPONENT2( "BaseApp", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: WorkerThread
// -----------------------------------------------------------------------------

#ifdef _WIN32
#pragma warning( disable: 4355 )  // 'this' used in base member initializer list
#endif

/**
 *	Constructor.
 */
WorkerThread::WorkerThread() :
	SimpleThread( &WorkerThread::s_run,
		(void*)(uintptr(this) | uintptr(ready_=false)) ),
	ready_( false ),
	done_( false )
{
	// we initialised ready with a trick above ... only other way I
	// can think of would have been to use multiple inheritance...
	ready_ = true;
}

/**
 *	Destructor.
 */
WorkerThread::~WorkerThread()
{
	// cannot delete a working thread from its own thread!
	MF_ASSERT( SimpleThread::current() != this->handle() );

	queueLock_.grab();
	for (DoleQueue::iterator it = queue_.begin(); it != queue_.end(); it++)
	{
		it->second->disowned();
	}
	queue_.clear();
	queueLock_.give();
	// TODO: what if a job is waiting to destroy itself from another thread?
	// could sleep a little... anyway, that's a race condition for another day.

	done_ = true;
}


#ifdef _WIN32
static void usleep( uint microseconds )
{
	Sleep( (microseconds+999)/1000 );
}
#endif

/**
 *	Run method
 */
void WorkerThread::run()
{
	while (!ready_) usleep( 100000 );

	uint64 sps = stampsPerSecond();

	uint32 sleepTime = 0;
	while (!done_)
	{
		if (sleepTime > 0)
		{
			usleep( sleepTime );
			sleepTime = 0;
		}

		queueLock_.grab();

		// see if anyone is waiting
		if (queue_.empty())
		{
			queueLock_.give();
			// none there but we can't sleep too long as we won't
			// get woken if some new job comes along
			sleepTime = 100000;
			continue;
		}

		// ok someone wants a job eventually
		DoleQueue::iterator beg = queue_.begin();
		uint64 now = timestamp();
		if (beg->first > now)
		{
			uint64 diff = beg->first - now;
			queueLock_.give();	// 'beg' is now invalid

			if (diff > sps)
				sleepTime = 100000;
			else	// calc below works for chips up to 16THz :)
				sleepTime = std::min( uint32(diff*1000000/sps), uint32(100000UL) );
			continue;
		}

		// yes someone wants a job now
		WorkerJob * pJob = beg->second;
		pJob->running_ = true;
		queue_.erase( beg );
		queueLock_.give();

		// run them
		float delaySeconds = (*pJob)();

		// and readd them if they want to be readded
		if (delaySeconds >= 0.f)
		{
			this->add( pJob, int64(double(delaySeconds)*double(sps)) );
		}
		else
		{
			queueLock_.grab();
			pJob->running_ = false;
			pJob->disowned();
			queueLock_.give();
			if (delaySeconds < -1.f)
				delete pJob;
		}
	}
}

/**
 *	Add a job
 */
void WorkerThread::add( WorkerJob * pJob, uint64 delay )
{
	queueLock_.grab();

	pJob->running_ = false;
	if (pJob->deleting_)
	{		// if it is being deleted then disown it
		pJob->disowned();
		queueLock_.give();
		return;
	}

	do { pJob->nextTime_ = timestamp() + delay; }
	while (queue_.find( pJob->nextTime_ ) != queue_.end());
	queue_[ pJob->nextTime_ ] = pJob;

	queueLock_.give();
}

/**
 *	Remove a job
 */
void WorkerThread::del( WorkerJob * pJob )
{
	// let it know we want to delete it
	pJob->deleting_ = true;

	// get a lock while it's not running (will not re-run after line above)
	while (1)
	{
		queueLock_.grab();
		if (!pJob->running_) break;
		queueLock_.give();
		// wait a bit then ... it will be disowned when it finishes
		usleep( 1000 );
	}

	// see if it hasn't already been removed
	if (pJob->pThread_ != this)
	{
		// it has already been disowned
		queueLock_.give();
		return;
	}

	// find it and remove it
	DoleQueue::iterator found = queue_.find( pJob->nextTime_ );
	MF_ASSERT( found != queue_.end() );
	queue_.erase( found );
	pJob->disowned();

	// and release the lock
	queueLock_.give();
}


// -----------------------------------------------------------------------------
// Section: WorkerJob
// -----------------------------------------------------------------------------

const float WorkerJob::DONT_RESCHEDULE = -0.5f;
const float WorkerJob::DONT_RESCHEDULE_AND_DESTROY = -1.5f;

/**
 *	Constructor.
 */
WorkerJob::WorkerJob() :
	pThread_( NULL ),
	nextTime_( 0 ),
	running_( false ),
	deleting_( false )
{
}

/**
 *	Destructor.
 */
WorkerJob::~WorkerJob()
{
	this->withdraw();
}

/**
 *	Submit this job to the given thread
 */
void WorkerJob::submit( WorkerThread * pThread )
{
	MF_ASSERT( pThread_ == NULL && pThread != NULL );
	pThread_ = pThread;
	pThread_->add( this );
}

/**
 *	Withdraw this job from the current thread
 */
void WorkerJob::withdraw()
{
	if (pThread_ != NULL)
	{
		pThread_->del( this );
	}
}

/**
 *	We have become permanently unemployed :(
 */
void WorkerJob::disowned()
{
	pThread_ = NULL;	// race condition with this and destructor...
}

BW_END_NAMESPACE

// worker_thread.cpp
