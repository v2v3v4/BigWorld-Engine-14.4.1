#include "pch.hpp"

#include "py_callback.hpp"
#include "script.hpp"

#include "cstdmf/bw_list.hpp"
#include "cstdmf/singleton_manager.hpp"

#ifndef MF_SERVER
// Not available on server yet, pending refactoring.

BW_BEGIN_NAMESPACE

namespace Script
{
	/**
	 * Store a callback function that can tell caller the total time that
	 * game has been running. The application should call setTotalGameTimeFn
	 * to set this up.
	 */
	TotalGameTimeFn s_totalGameTimeFn = NULL;

	TotalGameTimeFn & getTotalGameTimeFn()
	{
		SINGLETON_MANAGER_WRAPPER_FUNC( TotalGameTimeFn, getTotalGameTimeFn )
		return s_totalGameTimeFn;
	}

	/**
	 * Set the callback function.
	 */
	void setTotalGameTimeFn( TotalGameTimeFn fn )
	{
		s_totalGameTimeFn = fn;
		REGISTER_SINGLETON_FUNC( TotalGameTimeFn, getTotalGameTimeFn )
	}

	/**
	 * Get total amount of time that game has been running.
	 */
	double getTotalGameTime()
	{
		return getTotalGameTimeFn()();
	}

	TimerHandle lastTimerHandle_ = INVALID_TIMER_HANDLE;

	/**
	*	This structure is used by the BigWorld client callback
	*	system.  It records a single callback request.
	*/
	struct TimerRecord
	{
		/**
		*	This method returns whether or not the input record occurred later than
		*	this one.
		*
		*	@return True if input record is earlier (higher priority),
		*		false otherwise.
		*/
		bool operator <( const TimerRecord & b ) const
		{
			return b.time < this->time;
		}

		double		time;			///< The time of the record.
		PyObject	* function;		///< The function associated with the record.
		PyObject	* arguments;	///< The arguments associated with the record.
		const char	* source;		///< The source of this timer record
		TimerHandle	handle;			///< The handle issued for this callback.
	};


	typedef BW::list<TimerRecord>	Timers;
	Timers	gTimers;
	Timers::iterator gCurrent = gTimers.end();

	/**
	*	Clears and releases all existing timers.
	*/
	void clearTimers()
	{
		// this has to be called at a different time
		// than fini, that's why it's a separate method
		for( Timers::iterator iTimer = gTimers.begin(); iTimer != gTimers.end(); iTimer++ )
		{
			Py_DECREF( iTimer->function );
			Py_DECREF( iTimer->arguments );
		}
		gTimers.clear();
	}

	/**
	*	This function calls any script timers which have expired by now
	*/
	void tick( double timeNow )
	{
		gCurrent = gTimers.begin();

		while ( gCurrent != gTimers.end() )
		{
			if ( gCurrent->time <= timeNow )
			{
				TimerRecord& timer = *gCurrent;
				Script::call( timer.function, timer.arguments, timer.source );
				// Script::call decrefs timer.function and timer.arguments for us

				// NOTE - any call into Python can potentially insert / remove
				// callbacks, so we need to be careful with the above.
				//
				// If any new callbacks are inserted during the above call,
				// they are insert-sorted, which is fine since this is a list.
				//
				// However ones inserted with 'time == 0' are special-cased
				// to always go off next frame.  This is why we iterate using
				// gCurrent, a global iterator handle, and we never insert-sort
				// time == 0 timers after the global iterator handle.
				//
				// Callbacks may also be cancelled during the above call.  Either
				// side of the current iterator is fine, however the currently
				// called callback may be cancelled during the call, and so in
				// cancel callback we also need to be careful not to erase the
				// current call (again checking the global iterator handle)
				gCurrent = gTimers.erase( gCurrent );
			}
			else
			{
				// Just break, callbacks are always in order, so when we hit
				// a timer not yet ready to go off, we can ignore the rest too.
				// But we do need to set the global iterator to point to end.
				gCurrent = gTimers.end();
				break;
			}
		}
	}


	/**
	 *	Add a new timer to the gTimers list.  There are a couple of rules this
	 *	function must follow.
	 *	1. gTimers list must remain sorted, and stable.
	 *	2. if the Timer is due to go off immediately, then it must be made to
	 *	go off next frame.
	 */
	int32 addTimer( PyObject * fn, PyObject * args, const char * reason, double delay )
	{
		double t = getTotalGameTime() + delay;
		TimerHandle handle = getHandle();

		if ( handle != INVALID_TIMER_HANDLE )
		{
			TimerRecord newTR = { t, fn, args, reason, handle };

			Timers::iterator it = gTimers.begin();
			Timers::iterator end = (delay <= 0.0) ? gCurrent : gTimers.end();

			//Note about end iterator.  Usually gCurrent is the same as end().
			//However, if we are currently iterating through our timers and
			//calling callbacks, gCurrent will be 'anywhere' in the timers list.
			//
			//Due to the nature of 'callback', even if we are adding a timer
			//with <= 0.0 time left, we must always wait at least 1 frame.
			//
			//Therefore if the new timer's delay <= 0, and we are currently
			//calling timer callbacks (gCurrent != end), then we must not
			//insert the timer after gCurrent, or the timer will go off almost
			//immediately.
			//
			//This fulfills the desired behaviour of waiting a frame,
			//but crucially it allows the new timer to be cancelled during
			//the current iteration through gTimers.

			while ( it != end )
			{
				if ( it->time > t )
				{
					gTimers.insert( it, newTR );
					return handle;
				}
				else
				{
					it++;
				}
			}

			//If we get to here, the list was empty, or every item in the list
			//had an earlier timeout.  The iterator may be pointing to the end
			//of the list, or the current global iteration point.
			gTimers.insert( it, newTR );
		}

		return handle;
	}

	/**
	 *	This function returns a new timer handle.  Each timer handle is unique,
	 *	and the limit is MAX_TIMER_HANDLE, currently 2^32.  If the limit is
	 *	reached, an error message is displayed, and the timer handles begin
	 *	to be reused, which may cause problems if you are still using a very
	 *	old timer handle.
	 */
	TimerHandle getHandle()
	{
		if (lastTimerHandle_ == MAX_TIMER_HANDLE)
		{
			ERROR_MSG( "Script::getHandle - timer handles exhausted\n" );
			lastTimerHandle_ = 0;
		}
		return ++lastTimerHandle_;
	}

	/**
	*	This function adds a script 'timer' to be called next tick
	*
	*	It is used by routines which want to make script calls but can't
	*	because they're in the middle of something scripts might mess up
	*	(like iterating over the scene to tick or draw it)
	*
	*	The optional age parameter specifies the age of the call,
	*	i.e. how far in the past it wanted to be made.
	*	Older calls are called back first.
	*
	*	@note: This function steals the references to both fn and args
	*/
	void callNextFrame( PyObject * fn, PyObject * args,
								const char * reason, double age )
	{
		addTimer( fn, args, reason, -age );
	}
}


/*~ function BigWorld.callback
 *
 *  Registers a callback function to be called after a certain time,
 *  but not before the next tick. The callback is executed once and
 *  is not automatically repeated, so reset the callback again if 
 *  regular updates are needed. If multiple callbacks are due to be
 *  executed within a single frame, the chronological order in which
 *  they were added is maintained.
 *
 *  @param time A float describing the delay in seconds before function is
 *  called. If a time of 0 is given, then it is guaranteed to be called 
 *  on the next frame.
 *  @param function Function to call. This function must take 0 arguments.
 *  @return int A handle that can be used to cancel the callback.
 *
 *  @see BigWorld.cancelCallback
 */
/**
 *	Registers a callback function to be called after a certain time,
 *	 but not before the next tick. (If registered during a tick
 *	 and it has expired then it will go off still - add a miniscule
 *	 amount of time to BigWorld.time() to prevent this if unwanted)
 *	All times are interpreted as offsets from the current time.
 */
static PyObject * py_callback( PyObject * args )
{
	double		time = 0.0;
	PyObject *	function = NULL;

	if (!PyArg_ParseTuple( args, "dO", &time, &function ) ||
		function == NULL || !PyCallable_Check( function ) )
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.callback: "
			"Argument parsing error." );
		return NULL;
	}

	if (time < 0) time = 0.0;

	Py_INCREF( function );

	Script::TimerHandle handle = Script::addTimer( function, PyTuple_New(0), "BigWorld Callback: ", time );

	if ( handle == Script::INVALID_TIMER_HANDLE )
	{
		PyErr_SetString( PyExc_TypeError, "py_callback: Callback handle overflow." );
		return NULL;
	}

	PyObject * pyId = PyInt_FromLong(handle);
	return pyId;
}
PY_MODULE_FUNCTION( callback, BigWorld )


/*~ function BigWorld.cancelCallback
 *
 *  Cancels a previously registered callback.
 *  @param int An integer handle identifying the callback to cancel.
 *  @return None.
 *
 *  @see BigWorld.callback
 */
/**
 *	Cancels a previously registered callback.
 *   Safe behaviour is NOT guaranteed when cancelling an already executed
 *   or cancelled callback.
 */
static PyObject * py_cancelCallback( PyObject * args )
{
	Script::TimerHandle handle;

	if (!PyArg_ParseTuple( args, "i", &handle ) )
	{
		PyErr_SetString( PyExc_TypeError, "py_cancelCallback: Argument parsing error." );
		return NULL;
	}

	for( Script::Timers::iterator iTimer = Script::gTimers.begin(); 
		iTimer != Script::gTimers.end(); iTimer++ )
	{
		if( iTimer->handle == handle )
		{
			// Don't cancel the currently-being-called timer,
			// see comments in ::tick
			if (iTimer != Script::gCurrent)
			{
				Py_DECREF( iTimer->function );
				Py_DECREF( iTimer->arguments );
				Script::gTimers.erase( iTimer );
			}
			break;
		}
	}

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( cancelCallback, BigWorld )

BW_END_NAMESPACE

#endif // MF_SERVER


// py_callback.cpp

