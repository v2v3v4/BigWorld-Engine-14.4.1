#include "script_events.hpp"

#include "script.hpp"

#include "pyobject_plus.hpp" // For Py_RETURN_NONE

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Script", 0 )

BW_BEGIN_NAMESPACE

namespace
{
ScriptEvents * g_pInstance = NULL;
}

// -----------------------------------------------------------------------------
// Section: ScriptEventList
// -----------------------------------------------------------------------------

/**
 *	This method calls the function associated with this element.
 */
PyObject * ScriptEventList::Element::call( PyObject * pArgs )
{
	Py_INCREF( pFunc_.get() );
	Py_INCREF( pArgs );

	return Script::ask( pFunc_.get(), pArgs );
}


/**
 *	This method calls all of the listeners in this list.
 */
bool ScriptEventList::triggerEvent( PyObject * pArgs, ScriptList resultsList )
{
	bool isOkay = true;
	ScriptEventList copy( *this );

	Container::iterator iter = copy.container_.begin();

	while (iter != copy.container_.end())
	{
		ScriptObject pResult = ScriptObject( iter->call( pArgs ),
			ScriptObject::FROM_NEW_REFERENCE );

		isOkay &= (pResult != NULL);

		if (resultsList)
		{
			resultsList.append( pResult ? pResult : ScriptObject::none() );
		}

		++iter;
	}

	return isOkay;
}


/**
 *	This method adds a listener to this list.
 */
bool ScriptEventList::add( PyObject * pListener, int level )
{
	Container::iterator iter = container_.begin();

	while ((iter != container_.end()) && (iter->level() <= level))
	{
		++iter;
	}

	container_.insert( iter, Element( pListener, level ) );

	return true;
}


/**
 *	This method removes a listener from this list.
 */
bool ScriptEventList::remove( PyObject * pListener )
{
	Container::iterator iter = container_.begin();

	while (iter != container_.end())
	{
		if (iter->matches( pListener ))
		{
			container_.erase( iter );
			return true;
		}
	}

	return false;
}


// -----------------------------------------------------------------------------
// Section: ScriptEvents
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ScriptEvents::ScriptEvents()
{
	if (g_pInstance == NULL)
	{
		g_pInstance = this;
	}
	else
	{
		WARNING_MSG( "ScriptEvents::ScriptEvents: "
				"Already have a ScriptEvents instance.\n" );
	}
}


/**
 *	Destructor.
 */
ScriptEvents::~ScriptEvents()
{
	if (g_pInstance == this)
	{
		g_pInstance = NULL;
	}
	else
	{
		WARNING_MSG( "ScriptEvents::~ScriptEvents: "
				"Was not singleton instance.\n" );
	}
}


/**
 *	This method creates a new event type. This must be called before
 *	addEventListener or triggerEvent is called for this event type.
 */
void ScriptEvents::createEventType( const char * eventName )
{
	container_[ eventName ] = ScriptEventList();
}


/**
 *	This method clears the script events list.
 */
void ScriptEvents::clear()
{
	container_.clear();
}


/**
 *	This method calls the listeners associated with an event type.
 */
bool ScriptEvents::triggerEvent( const char * eventName, PyObject * pArgs,
		ScriptList resultsList )
{
	PyObjectPtr pArgsHolder( pArgs, PyObjectPtr::STEAL_REFERENCE );

	Container::iterator iter = container_.find( eventName );

	if (iter == container_.end())
	{
		ERROR_MSG( "ScriptEvents::triggerEvent: Invalid event type '%s'\n",
				eventName );

		return false;
	}

	return iter->second.triggerEvent( pArgs, resultsList );
}


/**
 *	This is a simple convenience method that allows triggering two events with
 *	the same arguments.
 */
bool ScriptEvents::triggerTwoEvents( const char * event1, const char * event2,
		PyObject * pArgs )
{
	Py_INCREF( pArgs );

	bool result1 = this->triggerEvent( event1, pArgs );
	bool result2 = this->triggerEvent( event2, pArgs );

	return result1 && result2;
}


/**
 *	This method associates a listener with an event type.
 */
bool ScriptEvents::addEventListener( const char * eventName,
		PyObject * pListener, int level )
{
	MF_ASSERT( pListener );
	MF_ASSERT( PyCallable_Check( pListener ) );

	Container::iterator iter = container_.find( eventName );

	if (iter == container_.end())
	{
		ERROR_MSG( "ScriptEvents::addEventListener: Invalid event type '%s'\n",
				eventName );
		return false;
	}

	return iter->second.add( pListener, level );
}


/**
 *	This method removes a listener from an event type.
 */
bool ScriptEvents::removeEventListener( const char * eventName,
		PyObject * pListener )
{
	Container::iterator iter = container_.find( eventName );

	if (iter == container_.end())
	{
		ERROR_MSG( "ScriptEvents::removeEventListener: "
				"Invalid event type '%s'\n", eventName );
		return false;
	}

	return iter->second.remove( pListener );
}


/**
 *	This method retrieves all of the BwPersonality callbacks and adds them as
 *	event listeners.
 */
void ScriptEvents::initFromPersonality( ScriptModule personality )
{
	MF_ASSERT( personality );

	Container::iterator iter = container_.begin();

	while (iter != container_.end())
	{
		ScriptObject ret = personality.getAttribute( iter->first.c_str(),
			ScriptErrorClear() );

		if (ret)
		{
			iter->second.add( ret.get(), 0 );
		}

		++iter;
	}
}


/*~ function BigWorld.addEventListener
 *	@components{ base, cell, db }
 *
 *	The function addEventListener associates a callback with an event. The
 *	callback will be called whenever the event is triggered.
 *
 *	The events that can be listened to are all of the BWPersonality callbacks
 *	that do not have a return value. For example, "onInit", "onFini" etc.
 *
 *	Instead of calling this function directly, callbacks can also be decorated
 *	using the eventListener decorator function in the bwdecorators module.
 *
 *	@param eventName The name of the event to listen for.
 *	@param callback A callable object that will be called on this event.
 *	@param level An optional integer level. Listeners with a higher level are
 *		called later. The default value is 0.
 */
bool addEventListener( BW::string eventName, PyObjectPtr pCallback, int level )
{
	if (!PyCallable_Check( pCallback.get() ))
	{
		PyErr_SetString( PyExc_TypeError, "Argument is not callable" );

		return false;
	}

	if (!g_pInstance)
	{
		PyErr_SetString( PyExc_SystemError, "ScriptEvents not initialised" );
		return false;
	}

	if (!g_pInstance->addEventListener( eventName.c_str(),
				pCallback.get(), level ))
	{
		PyErr_Format( PyExc_ValueError,
				"Invalid event type '%s'", eventName.c_str() );
		return false;
	}

	return true;
}
PY_AUTO_MODULE_FUNCTION_WITH_DOC( RETOK, addEventListener,
		ARG( BW::string, ARG( PyObjectPtr, OPTARG( int, 0, END ) ) ), BigWorld,
		"Associates an event listener with an event. The callback will be "
		"called whenever the event is triggered." );


/*~ function BigWorld.removeEventListener
 *	@components{ base, cell, db }
 *
 *	The function removeEventListener removes an event listener associated with
 *	an event previously added using BigWorld.addEventListener.
 *
 *	@param eventName The name of the event that was listened for.
 *	@param callback The callable object that should be removed.
 */
bool removeEventListener( BW::string eventName, PyObjectPtr pCallback )
{
	if (!PyCallable_Check( pCallback.get() ))
	{
		PyErr_SetString( PyExc_TypeError, "Argument is not callable" );

		return false;
	}

	if (!g_pInstance)
	{
		PyErr_SetString( PyExc_SystemError, "ScriptEvents not initialised" );
		return false;
	}

	if (!g_pInstance->removeEventListener( eventName.c_str(), pCallback.get() ))
	{
		PyErr_Format( PyExc_ValueError,
				"Invalid event type '%s'", eventName.c_str() );
		return false;
	}

	return true;
}
PY_AUTO_MODULE_FUNCTION_WITH_DOC( RETOK, removeEventListener,
		ARG( BW::string, ARG( PyObjectPtr, END ) ), BigWorld,
		"Removes an event listener previously added via addEventListener" );

BW_END_NAMESPACE

// script_events.cpp
