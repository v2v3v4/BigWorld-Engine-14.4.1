#include "pch.hpp"
#include "py_event_project.hpp"
#include "sound_manager.hpp"

#if FMOD_SUPPORT
#include <fmod_errors.h>

DECLARE_DEBUG_COMPONENT2( "PyEventProject", 0 );


BW_BEGIN_NAMESPACE

/*~ class FMOD.EventProject
 *  
 *  An EventProject is a wrapper for an FMOD::EventProject.
 *
 *  Please see the FMOD Designer API documentation and the FMOD Designer
 *  User Manual, both are located in your FMOD install directory.
 */
PY_TYPEOBJECT( PyEventProject );

PY_BEGIN_METHODS( PyEventProject )

    /*~ function EventProject.stopAllEvents
	 *
	 *  Stop all events in 
	 *
	 *  @param	Bool immediately stop all sounds ignoring the "Fadeout time" property
	 */
    PY_METHOD( stopAllEvents )

    /*~ function EventProject.release
	 *
	 *  Release this event project and all the events/eventgroups that it contains. This object
     *  will no longer be valid once this is called.
	 */
    PY_METHOD( release )

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyEventProject )

    /*~ attribute EventProject.memoryUsed
	 *
	 *  Memory used by this event project.
	 *
	 *  @type	Unsigned Int
	 */
	PY_ATTRIBUTE( memoryUsed )

PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( PyEventProject, "EventProject", _FMOD )


PyEventProject::PyEventProject( SoundManager::EventProject * pEventProject, PyTypeObject * pType ) :
	PyObjectPlus( pType ),
    eventProject_( pEventProject )
{
	BW_GUARD;
	if (eventProject_)
	{
		FMOD_RESULT result = eventProject_->setUserData( this );
		SoundManager::FMOD_ErrCheck( result, "PyEventProject::PyEventProject");
	}
}

PyEventProject::~PyEventProject()
{
}

void PyEventProject::fini()
{
	eventProject_ = NULL;
}

void PyEventProject::stopAllEvents(bool immediate)
{
	BW_GUARD;
	if (eventProject_)
	{
		FMOD_RESULT result = eventProject_->stopAllEvents(immediate);
		SoundManager::FMOD_ErrCheck(result, "PyEventProject::stopAllEvents");
	}
}

void PyEventProject::release()
{
	BW_GUARD;
	if (eventProject_)
	{
		char * name;
		FMOD_RESULT result = eventProject_->getInfo( NULL, &name);
		if (!SoundManager::FMOD_ErrCheck(result, "PyEventProject::release: unable to unload event project"))
			return;

		SoundManager::instance().unloadEventProject( name );
		eventProject_ = NULL;
		this->decRef();
	}
}

/**
 *  Retrieve detailed memory usage information about this object.  
 */
unsigned int PyEventProject::memoryUsed()
{
    BW_GUARD;
    unsigned int mem = 0;
#if FMOD_SUPPORT_MEMORYINFO
# if 1
    FMOD_RESULT result = eventProject_->getMemoryInfo(FMOD_MEMBITS_ALL, FMOD_EVENT_MEMBITS_ALL, &mem, NULL);
# else
    /*
        TODO: use FMOD_MEMBITS and FMOD_EVENT_MEMBITS flags 
    to specifiy what kind of memory is wanted
    */
# endif
#endif
    return mem;
}

PyObject *PyEventProject::pyNew( PyObject *args )
{
	BW_GUARD;

	char* projectName;
	if (!PyArg_ParseTuple( args, "s", &projectName ))
	{
		PyErr_SetString( PyExc_TypeError, "FMOD.EventProject() "
			"expects a string project name argument" );
		return NULL;
	}

	PyObject* project = 
		SoundManager::instance().pyEventProject( projectName );

	if (SoundManager::errorLevel() != SoundManager::ERROR_LEVEL_EXCEPTION &&
		(project == Py_None || !project))
	{
		Py_XDECREF(project);

		// Always return a valid instance if we aren't throwing an exception
		// since we're in a type constructor!
		project = new PyEventProject(NULL);
	}

	return project;
}

BW_END_NAMESPACE

#endif // FMOD_SUPPORT

// pyeventproject.cpp
