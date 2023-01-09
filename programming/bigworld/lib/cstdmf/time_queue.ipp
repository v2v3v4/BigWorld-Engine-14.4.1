// -----------------------------------------------------------------------------
// Section: TimeQueueT
// -----------------------------------------------------------------------------

/**
 *	This is the constructor.
 */
template< class TIME_STAMP >
TimeQueueT< TIME_STAMP >::TimeQueueT() :
	timeQueue_(),
	pProcessingNode_( NULL ),
	lastProcessTime_( 0 ),
	numCancelled_( 0 )
{
}

/**
 * 	This is the destructor. It walks the queue and cancels events,
 *	then deletes them. If the cancellation of events
 */
template <class TIME_STAMP>
TimeQueueT< TIME_STAMP >::~TimeQueueT()
{
	this->clear();
}


/**
 *	This method cancels all events in this queue.
 */
template <class TIME_STAMP>
void TimeQueueT< TIME_STAMP >::clear( bool shouldCallOnRelease )
{
	// Make sure we don't loop forever
	int maxLoopCount = static_cast<int>(timeQueue_.size());

	while (!timeQueue_.empty())
	{
		Node * pNode = timeQueue_.unsafePopBack();

		if (!pNode->isCancelled() && maxLoopCount > 0)
		{
			--maxLoopCount;

			--numCancelled_;
			pNode->cancel( shouldCallOnRelease );
		}
		else if (pNode->isCancelled())
		{
			--numCancelled_;
		}

		pNode->decRef();
	}

	numCancelled_ = 0;

	// Clear the queue
	timeQueue_ = PriorityQueue();
}


/**
 *	This method adds an event to the time queue. If interval is zero,
 *	the event will happen once and will then be deleted. Otherwise,
 *	the event will be fired repeatedly.
 *
 *	@param startTime	Time of the initial event, in game ticks
 *	@param interval		Number of game ticks between subsequent events
 *	@param pHandler 	Object that is to receive the event
 *	@param pUser		User data to be passed with the event.
 *	@return				A handle to the new event.
 */
template <class TIME_STAMP>
TimerHandle TimeQueueT< TIME_STAMP >::add( TimeStamp startTime,
		TimeStamp interval, TimerHandler * pHandler, void * pUser,
		const char * name )
{
	Node * pNode = new Node( *this, startTime, interval, pHandler, pUser, name );
	timeQueue_.push( pNode );
	pNode->incRef();

	return TimerHandle( pNode );
}


/**
 *	This method is called when a timer has been cancelled.
 */
template <class TIME_STAMP>
void TimeQueueT< TIME_STAMP >::onCancel()
{
	++numCancelled_;

	// If there are too many cancelled timers in the queue (more than half),
	// these are flushed from the queue immediately.

	if (numCancelled_ * 2 > int( timeQueue_.size() ))
	{
		this->purgeCancelledNodes();
	}
}


/**
 *	This class is used by purgeCancelledNodes to partition the timers and
 *	separate those that have been cancelled.
 */
template <class NODE>
class IsNotCancelled
{
public:
	bool operator()( const NODE * pNode )
	{
		return !pNode->isCancelled();
	}
};


/**
 *	This method removes all cancelled timers from the priority queue. Generally,
 *	cancelled timers wait until they have reached the top of the queue before
 *	being deleted.
 */
template <class TIME_STAMP>
void TimeQueueT< TIME_STAMP >::purgeCancelledNodes()
{
	typename PriorityQueue::Container & container = timeQueue_.container();

	typename PriorityQueue::Container::iterator newEnd =
		std::partition( container.begin(), container.end(),
			IsNotCancelled< Node >() );

	for (typename PriorityQueue::Container::iterator iter = newEnd;
		iter != container.end();
		++iter)
	{
		(*iter)->decRef();
	}

	size_t numPurgedFull = (container.end() - newEnd);;
	MF_ASSERT( numPurgedFull <= INT_MAX );
	const int numPurged = static_cast<int>(numPurgedFull);
	numCancelled_ -= numPurged;
	// numCancelled_ will be 1 when we're in the middle of processing a
	// once-off timer.
	MF_ASSERT( (numCancelled_ == 0) || (numCancelled_ == 1) );

	container.erase( newEnd, container.end() );
	timeQueue_.heapify();
}


/**
 *	This method processes the time queue and dispatches events.
 *	All events with a timestamp earlier than the given one are
 *	processed.
 *
 *	@param now		Process events earlier than or exactly on this.
 *
 *	@return The number of timers that fired.
 */
template <class TIME_STAMP>
int TimeQueueT< TIME_STAMP >::process( TimeStamp now )
{
	int numFired = 0;

	while ((!timeQueue_.empty()) && (
		timeQueue_.top()->time() <= now ||
		timeQueue_.top()->isCancelled()))
	{
		Node * pNode = pProcessingNode_ = timeQueue_.top();
		timeQueue_.pop();

		if (!pNode->isCancelled())
		{
			++numFired;
			pNode->triggerTimer();
		}

		if (!pNode->isCancelled())
		{
			timeQueue_.push( pNode );
		}
		else
		{
			pNode->decRef();

			MF_ASSERT( numCancelled_ > 0 );
			--numCancelled_;
		}
	}

	pProcessingNode_ = NULL;
	lastProcessTime_ = now;

	return numFired;
}


/**
 *	This method determines whether or not the given handle is legal.
 */
template <class TIME_STAMP>
bool TimeQueueT< TIME_STAMP >::legal( TimerHandle handle ) const
{
	typedef Node * const * NodeIter;

	Node * pNode = static_cast< Node* >( handle.pNode() );

	if (pNode == NULL)
	{
		return false;
	}

	if (pNode == pProcessingNode_)
	{
		return true;
	}

	NodeIter begin = &timeQueue_.top();
	NodeIter end = begin + timeQueue_.size();

	for (NodeIter it = begin; it != end; it++)
	{
		if (*it == pNode)
		{
			return true;
		}
	}

	return false;
}

/**
 *	This method returns the time until the next timer goes off.
 */
template <class TIME_STAMP>
TIME_STAMP TimeQueueT< TIME_STAMP >::nextExp( TimeStamp now ) const
{
	if (timeQueue_.empty() ||
		now > timeQueue_.top()->time())
	{
		return 0;
	}

	return timeQueue_.top()->time() - now;
}


/**
 *	This method returns information associated with the timer with the input
 *	handler.
 */
template <class TIME_STAMP>
bool TimeQueueT< TIME_STAMP >::getTimerInfo( TimerHandle handle,
					TimeStamp &			time,
					TimeStamp &			interval,
					void * &			pUser ) const
{
	Node * pNode = static_cast< Node * >( handle.pNode() );

	if (!pNode->isCancelled())
	{
		time = pNode->time();
		interval = pNode->interval();
		pUser = pNode->pUserData();

		return true;
	}

	return false;
}


/**
 *	This method returns the time that the given timer handle will be delivered,
 *	in timestamps.
 */
template <class TIME_STAMP>
TIME_STAMP
	TimeQueueT< TIME_STAMP >::timerDeliveryTime( TimerHandle handle ) const
{
	Node * pNode = static_cast< Node * >( handle.pNode() );
	return pNode->deliveryTime();
}


/**
 *	This method returns the time between deliveries of the given timer handle,
 *	in timestamps.
 */
template <class TIME_STAMP>
TIME_STAMP
	TimeQueueT< TIME_STAMP >::timerIntervalTime( TimerHandle handle ) const
{
	Node * pNode = static_cast< Node *>( handle.pNode() );
	return pNode->interval();
}


/**
 *	This method returns the time between deliveries of the given timer handle,
 *	in timestamps. The value returned may be modified.
 */
template <class TIME_STAMP>
TIME_STAMP & TimeQueueT< TIME_STAMP >::timerIntervalTime( TimerHandle handle )
{
	Node * pNode = static_cast< Node *>( handle.pNode() );
	return pNode->intervalRef();
}


// -----------------------------------------------------------------------------
// Section: TimeQueueNode
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
inline TimeQueueNode::TimeQueueNode( TimeQueueBase & owner,
		TimerHandler * pHandler, void * pUserData ) :
	ReferenceCount(),
	owner_( owner ),
	pHandler_( pHandler ),
	pUserData_( pUserData ),
	state_( STATE_PENDING )
{
	pHandler->incTimerRegisterCount();
}


/**
 *	This method cancels the timer associated with this node.
 */
inline void TimeQueueNode::cancel( bool shouldCallOnRelease )
{
	if (this->isCancelled())
	{
		return;
	}

	MF_ASSERT( (state_ == STATE_PENDING) || (state_ == STATE_EXECUTING) );

	state_ = STATE_CANCELLED;

	if (pHandler_)
	{
		if (shouldCallOnRelease)
		{
			pHandler_->release( TimerHandle( this ), pUserData_ );
		}
		else
		{
			pHandler_->decTimerRegisterCount();
		}
		pHandler_ = NULL;
	}

	owner_.onCancel();
}


// -----------------------------------------------------------------------------
// Section: TimeQueueT::Node
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
template <class TIME_STAMP>
TimeQueueT< TIME_STAMP >::Node::Node( TimeQueueBase & owner,
		TimeStamp startTime, TimeStamp interval,
		TimerHandler * _pHandler, void * pUser,
		const char * name ) :
	TimeQueueNode( owner, _pHandler, pUser ),
	time_( startTime ),
	interval_( interval ),
	name_( name )
{
}


/**
 *	This method cancels the time queue node.
 */
template <class TIME_STAMP>
TIME_STAMP TimeQueueT< TIME_STAMP >::Node::deliveryTime() const
{
	return this->isExecuting() ?  (time_ + interval_) : time_;
}


/**
 *	This method triggers the timer assoicated with this node. It also updates
 *	the state for repeating timers.
 */
template <class TIME_STAMP>
void TimeQueueT< TIME_STAMP >::Node::triggerTimer()
{
	if (!this->isCancelled())
	{
		state_ = STATE_EXECUTING;

#if ENABLE_PROFILER
		ScopedProfiler _timerProfiler( name_ );
#endif
		pHandler_->handleTimeout( TimerHandle( this ), pUserData_ );

		if ((interval_ == 0) && !this->isCancelled())
		{
			this->cancel();
		}
	}

	// This event could have been cancelled within the callback.

	if (!this->isCancelled())
	{
		time_ += interval_;
		state_ = STATE_PENDING;
	}
}



// time_queue.ipp
