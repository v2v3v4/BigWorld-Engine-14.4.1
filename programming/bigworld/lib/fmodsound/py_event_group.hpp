#ifndef PY_EVENT_GROUP_HPP
#define PY_EVENT_GROUP_HPP

#include "fmod_config.hpp"

#if FMOD_SUPPORT

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "sound_manager.hpp"
#include "py_sound.hpp"

#include <fmod_event.hpp>


BW_BEGIN_NAMESPACE

/**
 *  A PyEventGroup is a wrapper for an FMOD::EventGroup, and provides an
 *  interface to control the loading and unloading of data by group. Groups
 *  are created using the event hierachy in the FMOD Designer tool.
 */
class PyEventGroup : public PyObjectPlus
{
	Py_Header( PyEventGroup, PyObjectPlus )

public:
    PyEventGroup( SoundManager::EventGroup * pEventGroup, PyTypeObject * pType = &PyEventGroup::s_type_ );   

	void fini();

    //  Methods
    void loadEventData(bool blocking = true);
	PY_AUTO_METHOD_DECLARE( RETVOID, loadEventData, OPTARG( bool, true, END ) )

    void freeEventData();
	PY_AUTO_METHOD_DECLARE( RETVOID, freeEventData, END )

	//  Attributes
    unsigned int memoryUsed();
	PY_RO_ATTRIBUTE_DECLARE( memoryUsed(), memoryUsed ); 

    bool isPlaying();
	PY_RO_ATTRIBUTE_DECLARE( isPlaying(), isPlaying );

    bool isLoaded();
	PY_RO_ATTRIBUTE_DECLARE( isLoaded(), isLoaded );

	PY_FACTORY_DECLARE()

	void invalidate();

protected:
    SoundManager::EventGroup *eventGroup_;

private:
    PyEventGroup();
	~PyEventGroup();

	PyEventGroup(const PyEventGroup&);
	PyEventGroup& operator=(const PyEventGroup&);
};

typedef SmartPointer< PyEventGroup > PyEventGroupPtr;

BW_END_NAMESPACE

#endif // FMOD_SUPPORT

#endif // PY_EVENT_GROUP_HPP
