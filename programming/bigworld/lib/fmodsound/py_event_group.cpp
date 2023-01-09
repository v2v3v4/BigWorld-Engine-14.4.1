#include "pch.hpp"
#include "py_event_group.hpp"


#if FMOD_SUPPORT
#include <fmod_errors.h>

DECLARE_DEBUG_COMPONENT2( "PyEventGroup", 0 );


BW_BEGIN_NAMESPACE

/*~ class FMOD.EventGroup
 *  
 *  An EventGroup is a wrapper for an FMOD::EventGroup, and provides an
 *  interface to control the loading and unloading of data by group. Groups
 *  are created using the event hierachy in the FMOD Designer tool.
 *
 *  Please see the FMOD Designer API documentation and the FMOD Designer
 *  User Manual, both are located in your FMOD install directory.
 */
PY_TYPEOBJECT( PyEventGroup );

PY_BEGIN_METHODS( PyEventGroup )

	/*~ function EventGroup.loadEventData
	 *
	 *  Loads the resources for all events within an event group.  
	 *
	 *  @param	Optional Bool	whether or not the client should block 
	 *							until the events have been loaded. Defaults
	 *							to true.
	 */
    PY_METHOD( loadEventData )

	/*~ function EventProject.freeEventData
	 *
	 *  Frees the resources for an EventGroup and all subgroups under it.
	 *
	 */
    PY_METHOD( freeEventData )

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyEventGroup )

    /*~ attribute EventGroup.memoryUsed
	 *
	 *  Memory used by this event group.
	 *
	 *  @type	Read-only unsigned integer
	 */
	PY_ATTRIBUTE( memoryUsed )
    /*~ attribute EventGroup.isPlaying
	 *
	 *  Whether this group contains any events which are currently playing.
	 *
	 *  @type	Read-only boolean
	 */
	PY_ATTRIBUTE( isPlaying )
    /*~ attribute EventGroup.isLoaded
	 *
	 *  Whether this group is currently loading.
	 *
	 *  @type	Read-only boolean
	 */
	PY_ATTRIBUTE( isLoaded )

PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( PyEventGroup, "EventGroup", _FMOD )


PyEventGroup::PyEventGroup( SoundManager::EventGroup * pEventGroup, PyTypeObject * pType ) :
	PyObjectPlus( pType ),
    eventGroup_( pEventGroup )
{
	BW_GUARD;	
    MF_ASSERT( pEventGroup );

    SoundManager::FMOD_ErrCheck( eventGroup_->setUserData( this ) ,
        "PyEventGroup::PyEventGroup");
}

PyEventGroup::~PyEventGroup()
{
}

void PyEventGroup::fini()
{
	eventGroup_ = NULL;
}

/**
 *  Loads the resources for all events within an event group.  
 */
void PyEventGroup::loadEventData(bool blocking)
{
    if (eventGroup_ != NULL)
    {
        BW_GUARD;
        FMOD_RESULT result = eventGroup_->loadEventData(
            FMOD_EVENT_RESOURCE_STREAMS_AND_SAMPLES, 
            blocking ? FMOD_EVENT_DEFAULT : FMOD_EVENT_NONBLOCKING);
        SoundManager::FMOD_ErrCheck(result, "Error loading event data");
    }
    else
    {
		PyErr_Format( PyExc_RuntimeError,
			"EventGroup's parent project has been released!" );
    }
}

/** 
 *  Frees the resources for an EventGroup and all subgroups under it.  
 */
void PyEventGroup::freeEventData()
{
    BW_GUARD;
    if (eventGroup_ != NULL)
    {

        FMOD_RESULT result = eventGroup_->freeEventData();
        SoundManager::FMOD_ErrCheck(result, "Error freeing event data");
    }
    else
    {
		PyErr_Format( PyExc_RuntimeError,
			"EventGroup's parent project has been released!" );
    }
}

/**
 *  Retrieve detailed memory usage information about this object.  
 */
unsigned int PyEventGroup::memoryUsed()
{
    BW_GUARD;
    unsigned int mem = 0;

    if (eventGroup_ != NULL)
    {
#if FMOD_SUPPORT_MEMORYINFO
# if 1
        FMOD_RESULT result = eventGroup_->getMemoryInfo(FMOD_MEMBITS_ALL, FMOD_EVENT_MEMBITS_ALL, &mem, NULL);
# else
        /*
            TODO: use FMOD_MEMBITS and FMOD_EVENT_MEMBITS flags 
        to specifiy what kind of memory is wanted
        */
# endif
#endif
    }
    else
    {
		PyErr_Format( PyExc_RuntimeError,
			"EventGroup's parent project has been released!" );
    }

    return mem;
}

bool PyEventGroup::isPlaying()
{
    BW_GUARD;
    FMOD_EVENT_STATE state = 0;
    if (eventGroup_ != NULL)
    {
        FMOD_RESULT result = eventGroup_->getState(&state);
        SoundManager::FMOD_ErrCheck(result, "PyEventGroup::isPlaying");
    }
    else
    {
		PyErr_Format( PyExc_RuntimeError,
			"EventGroup's parent project has been released!" );
    }

    return ((state & FMOD_EVENT_STATE_PLAYING) != 0); 
}

bool PyEventGroup::isLoaded()
{
	BW_GUARD;
	FMOD_EVENT_STATE state = 0;
	if (eventGroup_ != NULL)
	{
		FMOD_RESULT result = eventGroup_->getState(&state);
		SoundManager::FMOD_ErrCheck(result, "PyEventGroup::isLoaded");
		if (state & FMOD_EVENT_STATE_ERROR)
		{
			ERROR_MSG( "PyEventGroup::isLoaded: An error has ocurred while loaded event group." );
		}
	}
	else
	{
		PyErr_Format( PyExc_RuntimeError,
			"EventGroup's parent project has been released!" );
	}

	return ((state & FMOD_EVENT_STATE_LOADING) == 0);//should FMOD_EVENT_STATE_READY be needed?
}

void PyEventGroup::invalidate()
{
    eventGroup_ = NULL;
}

PyObject *PyEventGroup::pyNew( PyObject *args )
{
	BW_GUARD;

	char* groupPath;
	if (!PyArg_ParseTuple( args, "s", &groupPath ))
	{
		PyErr_SetString( PyExc_TypeError, "FMOD.EventGroup() "
			"expects a string group path argument" );
		return NULL;
	}

	PyObject* group = 
		SoundManager::pInstance()->pyEventGroup(groupPath);

	if (SoundManager::errorLevel() != SoundManager::ERROR_LEVEL_EXCEPTION &&
		(group == Py_None || !group))
	{
		Py_XDECREF(group);

		// Always return a valid instance if we aren't throwing an exception
		// since we're in a type constructor!
		group = new PyEventGroup(NULL);
	}

	return group;
}

BW_END_NAMESPACE

#endif // FMOD_SUPPORT

// pyeventgroup.cpp
