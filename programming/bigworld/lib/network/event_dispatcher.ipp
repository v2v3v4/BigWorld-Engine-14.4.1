namespace Mercury
{

/**
 * Call the handler every microseconds.
 *
 * Timers cannot be longer than 30 minutes (size of int)
 *
 * @return ID of timer
 */
INLINE TimerHandle EventDispatcher::addTimer( int64 microseconds,
	TimerHandler * handler, void * arg, const char * name )
{
	return this->addTimerCommon( microseconds, handler, arg, true, name );
}


/**
 * Call the handler once after microseconds.
 *
 * Timers cannot be longer than 30 minutes (size of int)
 *
 * @return ID of timer
 */
INLINE TimerHandle EventDispatcher::addOnceOffTimer( int64 microseconds,
	TimerHandler * handler, void * arg, const char * name )
{
	return this->addTimerCommon( microseconds, handler, arg, false, name );
}


/**
 *	This method breaks out of 'processContinuously' at the next opportunity.
 *	Any messages in bundles that are being processed or timers that have
 *	expired will still get called. Note: if this is called from another
 *	thread it will NOT interrupt a select call if one is in progress, so
 *	processContinuously will not return. Try sending the process a (handled)
 *	signal if this is your intention.
 */
INLINE void EventDispatcher::breakProcessing( bool breakState )
{
	breakProcessing_ = breakState;
}


/**
 *	This method returns whether or not we have broken out of the
 *	processContinuously loop.
 *
 *	@see breakProcessing
 *	@see processContinuously
 */
INLINE bool EventDispatcher::processingBroken() const
{
	return breakProcessing_;
}


/**
 *	This method returns maximum number of seconds to wait in select.
 */
INLINE double EventDispatcher::maxWait() const
{
	return maxWait_;
}


/**
 *	This method sets maximum number of seconds to wait in select.
 */
INLINE void EventDispatcher::maxWait( double seconds )
{
	maxWait_ = seconds;
}

} // namespace Mercury

// event_dispatcher.ipp
