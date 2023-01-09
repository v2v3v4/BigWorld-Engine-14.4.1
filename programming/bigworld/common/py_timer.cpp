#include "script/first_include.hpp"

#include "py_timer.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyTimerHandler
// -----------------------------------------------------------------------------

/**
 *	This method implements the TimerHandler method. It is called when a timer
 *	associated with this PyTimer goes off.
 */
void PyTimerHandler::handleTimeout( TimerHandle handle, void * pUser )
{
	pyTimer_.handleTimeout( handle, pUser );
}


/**
 *	This method is called when a timer associated with this entity is released
 *	by the TimeQueue.
 */
void PyTimerHandler::onRelease( TimerHandle handle, void * pUser )
{
	pyTimer_.onTimerReleased( handle );
}


// -----------------------------------------------------------------------------
// Section: PyTimer
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PyTimer::PyTimer( PyObject * pyObject, int ownerID ) :
	timerHandler_( *this ),
	ownerID_( ownerID ),
	isCancelled_( false ),
	pyObject_( pyObject ),
	pTimers_( NULL )
{
	MF_ASSERT( pyObject_ );
}


/**
 *	Destructor.
 */
PyTimer::~PyTimer()
{
	MF_ASSERT( isCancelled_ );
	MF_ASSERT( pTimers_ == NULL );
}


/**
 *	This method cancels all added timers.
 */
void PyTimer::cancelAll()
{
	ScriptTimersUtil::cancelAll( pTimers_ );
	isCancelled_ = true;
}


/**
 *	This method is exposed to scripting. It is used by a PyObject to register a
 *	timer function to be called with a given period.
 */
PyObject * PyTimer::addTimer( PyObject * args )
{
	float initialOffset;
	float repeatOffset = 0.f;
	int userArg = 0;

	if (!PyArg_ParseTuple( args, "f|fi",
				&initialOffset, &repeatOffset, &userArg ))
	{
		return NULL;
	}

	int id = ScriptTimersUtil::addTimer( &pTimers_, initialOffset, repeatOffset,
			userArg, &timerHandler_ );

	if (id == 0)
	{
		PyErr_SetString( PyExc_ValueError, "Unable to add timer" );
		return NULL;
	}

	return PyInt_FromLong( id );
}


/**
 *	This method is exposed to scripting. It is used by a PyObject to remove a
 *	timer from the time queue.
 */
PyObject * PyTimer::delTimer( PyObject * args )
{
	int timerID;

	if (!PyArg_ParseTuple( args, "i", &timerID ))
	{
		return NULL;
	}

	if (!ScriptTimersUtil::delTimer( pTimers_, timerID ))
	{
		WARNING_MSG( "PyTimer::delTimer: %d is not a valid timer handle\n",
				timerID );
	}

	Py_RETURN_NONE;
}


/**
 *	This method is called when a timer associated with the PyObject goes off.
 */
void PyTimer::handleTimeout( TimerHandle handle, void * pUser )
{
	MF_ASSERT( !isCancelled_ );

	int id = ScriptTimersUtil::getIDForHandle( pTimers_, handle );

	if (id != 0)
	{
		// Reference count so that object is not deleted in the middle of
		// the call.
		Py_INCREF( pyObject_ );

		PyObject * pResult = PyObject_CallMethod( pyObject_,
				"onTimer", "ik", id, uintptr( pUser ) );

		if (pResult == NULL)
		{
			WARNING_MSG( "PyTimer::handleTimeout(%d): onTimer failed\n",
					ownerID_ );
			PyErr_Print();
		}
		else
		{
			Py_DECREF( pResult );
		}

		Py_DECREF( pyObject_ );
	}
	else
	{
		ERROR_MSG( "PyTimer::handleTimeout: Invalid TimerQueueId\n" );
	}
}


/**
 *	This method is called when a timer associated with this entity is released
 *	by the time queue.
 */
void PyTimer::onTimerReleased( TimerHandle timerHandle )
{
	ScriptTimersUtil::releaseTimer( &pTimers_, timerHandle );
}


/**
 *	This method writes this entity's timers to a backup stream.
 */
void PyTimer::backUpTimers( BinaryOStream & stream )
{
	ScriptTimersUtil::writeToStream( pTimers_, stream );
}


/**
 *	This method restores this entity's timers from a backup stream.
 */
void PyTimer::restoreTimers( BinaryIStream & stream )
{
	ScriptTimersUtil::readFromStream( &pTimers_, stream, &timerHandler_ );
}


BW_END_NAMESPACE

// py_timer.cpp
