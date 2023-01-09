#ifndef PY_EVENT_PROJECT_HPP
#define PY_EVENT_PROJECT_HPP

#include "fmod_config.hpp"

#if FMOD_SUPPORT

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "py_sound.hpp"

#include <fmod_event.hpp>


BW_BEGIN_NAMESPACE

/**
 *  A PyEventProject is a wrapper for an FMOD::EventGroup.
 */
class PyEventProject : public PyObjectPlus
{
	Py_Header( PyEventProject, PyObjectPlus )

public:
    PyEventProject( SoundManager::EventProject * pEventProject, PyTypeObject * pType = &PyEventProject::s_type_ );   

	void fini();

    //  Methods
    void stopAllEvents(bool immediate = true);
	PY_AUTO_METHOD_DECLARE( RETVOID, stopAllEvents, OPTARG( bool, true, END ) )

    void release();
	PY_AUTO_METHOD_DECLARE( RETVOID, release, END )

	//  Attributes
    unsigned int memoryUsed();
	PY_RO_ATTRIBUTE_DECLARE( memoryUsed(), memoryUsed ); 

	PY_FACTORY_DECLARE()

protected:
    SoundManager::EventProject *eventProject_;

private:
    PyEventProject();
	~PyEventProject();

	PyEventProject(const PyEventProject&);
	PyEventProject& operator=(const PyEventProject&);
};

typedef SmartPointer< PyEventProject > PyEventProjectPtr;

BW_END_NAMESPACE

#endif // FMOD_SUPPORT

#endif // PY_EVENT_PROJECT_HPP
