#include "script/first_include.hpp"

#include "cstdmf/time_queue.hpp"

#include "pyscript/pyobject_plus.hpp" // For PY_ERROR_CHECK
#include "pyscript/script.hpp"

#include "script_timers.hpp"


BW_BEGIN_NAMESPACE

int AppScriptTimers_token = 1;

namespace
{

ScriptTimers * g_pTimers = NULL;

// -----------------------------------------------------------------------------
// Section: BigWorld timers
// -----------------------------------------------------------------------------

class TimerFini : public Script::FiniTimeJob
{
public:
	virtual void fini()
	{
		// Currently, this is not needed as the timeQueue is destroyed and all
		// timers cancelled before Script::fini. g_pTimers should already be
		// NULL.

		ScriptTimersUtil::cancelAll( g_pTimers );
		MF_ASSERT( g_pTimers == NULL );
	}
};

TimerFini g_timerFini;


/**
 *	This class implements an object used by addTimer.
 */
class ScriptTimerHandler : public TimerHandler
{
public:
	ScriptTimerHandler( PyObject * pObject ) : pObject_( pObject ) 
	{
	}

private:
	virtual void handleTimeout( TimerHandle handle, void * pUser )
	{
		int id = ScriptTimersUtil::getIDForHandle( g_pTimers, handle );

		PyObject * pObject = pObject_.get();
		// Reference count so that object is not deleted in middle of call.
		Py_INCREF( pObject );
		PyObject * pResult =
			PyObject_CallFunction( pObject, "ik", id, uintptr( pUser ) );

		PY_ERROR_CHECK();
		Py_XDECREF( pResult );

		Py_DECREF( pObject );
	}

	virtual void onRelease( TimerHandle handle, void * /*pUser*/ )
	{
		ScriptTimersUtil::releaseTimer( &g_pTimers, handle );
		delete this;
	}

	SmartPointer<PyObject> pObject_;
};


/**
 *	This method is exposed to scripting. It is used to register a timer function
 *	to be called with a given period.
 */
PyObject * py_addTimer( PyObject * args )
{
	static bool isFirst = true;

	if (isFirst)
	{
		NOTICE_MSG( "BigWorld.addTimer: "
			"Timers created with this method are not backed up. "
			"Consider using Base.addTimer instead.\n" );
		isFirst = false;
	}

	PyObject * pObject;
	float initialOffset;
	float repeatOffset = 0.f;
	int userArg = 0;

	if (!PyArg_ParseTuple( args, "Of|fi:addTimer",
				&pObject, &initialOffset, &repeatOffset, &userArg ))
	{
		return NULL;
	}

	Py_INCREF( pObject );

	if (!PyCallable_Check( pObject ))
	{
		// For backward compatibility
		PyObject * pOnTimer = PyObject_GetAttrString( pObject, "onTimer" );

		Py_DECREF( pObject );

		if (pOnTimer == NULL)
		{
			PyErr_SetString( PyExc_TypeError,
					"Callback function is not callable" );
			return NULL;
		}

		pObject = pOnTimer;
	}

	TimerHandler * pHandler = new ScriptTimerHandler( pObject );

	Py_DECREF( pObject );

	int id = ScriptTimersUtil::addTimer( &g_pTimers,
			initialOffset, repeatOffset,
			userArg, pHandler );

	if (id == 0)
	{
		PyErr_SetString( PyExc_ValueError, "Unable to add timer" );
		delete pHandler;

		return NULL;
	}

	return PyInt_FromLong( id );
}
/*~ function BigWorld.addTimer
 *	@components{ base, cell }
 *	Note: This method should only be used in rare situations. It is better to
 *	use Entity.addTimer as entity timers are backed up and restored on BaseApp
 *	failure.
 *
 *	The function addTimer registers a function to be called after
 *	"initialOffset" seconds. Optionally it can be repeated every "repeatOffset"
 *	seconds, and have optional user data (an integer that is simply passed to
 *	the timer function) "userArg".
 *
 *	The callback function must accept 2 arguments; the internal id of the timer
 *	(useful to remove the timer via the "delTimer" method), and the optional
 *	user supplied integer "userArg".
 *
 *	Example:
 *	@{
 *	import BigWorld
 *
 *	# Callback function that accepts 2 parameters; the id of the timer,
 *	# and the specified user argument.
 *
 *	# id is the internal id of the timer, and can be used to remove the
 *	# timer via the use of the BigWorld.delTimer function
 *	def onTimer( id, userArg ):
 *	    print "onTimer called: id %i, userArg: %i" % ( id, userArg )
 *	    # if this is a repeating timer then call:
 *	    #     BigWorld.delTimer( id )
 *	    # to remove the timer when it is no longer needed
 *
 *	# call onTimer after 5 seconds, and then every 1 second with a userArg of 9
 *	BigWorld.addTimer( onTimer, 5, 1, 9 )
 *
 *	# call onTimer after 1 second with no repeat and userArg defaulting to a
 *	# value of 0
 *	BigWorld.addTimer( onTimer, 1 )
 *	@}
 *
 *	@param callback Specifies the timer handler to be used. This is a function
 *	that takes 2 integer parameters.
 *	@param initialOffset initialOffset is a float and specifies the interval (in
 *	seconds) between registration of the timer, and the time of first execution.
 *	@param repeatOffset=0 repeatOffset is an optional float and specifies the
 *	interval (in seconds) between repeated executions after the first execution.
 *	BigWorld.delTimer must be used to remove the timer or this will repeat
 *	continuously.
 *	@param userArg=0 userArg is an optional integer which specifies an optional
 *	user supplied value to be passed to the callback method.
 *	@return Integer. This function returns the internal id of the timer. This
 *	can be used with BigWorld.delTimer to remove the timer.
 */
PY_MODULE_FUNCTION_WITH_DOC( addTimer, BigWorld,
"The function addTimer registers a callback function to be \n\
called after \"initialOffset\" seconds. Optionally it can be repeated every \n\
\"repeatOffset\" seconds, and have optional user data (an integer that is \n\
simply passed to the timer function) \"userArg\".\n\n\
\"callback\" must be a function that accepts 2 arguments; the internal id of the timer \n\
(useful to remove the timer via the \"delTimer\" method), and the optional \n\
user supplied integer \"userArg\"." )

/**
 *	This method is exposed to scripting. It is used to delete a timer.
 */
PyObject * py_delTimer( PyObject * args )
{
	int timerID;

	if (!PyArg_ParseTuple( args, "i", &timerID ))
	{
		return NULL;
	}

	if (!ScriptTimersUtil::delTimer( g_pTimers, timerID ))
	{
		// TODO: This should really raise a Python exception.
		ERROR_MSG( "BigWorld.delTimer: Unable to cancel timer %d\n",
				timerID );
	}

	Py_RETURN_NONE;
}
/*~ function BigWorld.delTimer
 *	@components{ base, cell }
 *	The function delTimer removes a registered timer function so that it no
 *	longer executes. Timers that execute only once are automatically removed
 *	and do not have to be removed using delTimer. If delTimer is called with
 *	an id that is no longer valid (already removed for example) then an error
 *	will be logged to file.
 *
 *	See BigWorld.addTimer for an example of timer usage.
 *	@param id id is an integer that specifies the internal id of the timer to
 *	be removed.
 */
PY_MODULE_FUNCTION( delTimer, BigWorld )

} // anonymous namespace

BW_END_NAMESPACE

// app_script_timers.cpp
