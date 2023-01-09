#ifndef PY_EVENT_REVERB_HPP
#define PY_EVENT_REVERB_HPP

#include "fmod_config.hpp"

#if FMOD_SUPPORT

#include "math/vector3.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"
#include "sound_manager.hpp"

#include <fmod_event.hpp>


BW_BEGIN_NAMESPACE

/**
 *  A PyEventReverb is a wrapper for an FMOD::EventCategory, and a
 *  virtual folder used to organize event data into generic categories
 *  such as music. Allows volume and pitch changes to be applied to
 *  all events within the group.
 */
class PyEventReverb : public PyObjectPlus
{
	Py_Header( PyEventReverb, PyObjectPlus )

public:
    PyEventReverb( SoundManager::EventReverb * pEventReverb, 
                   PyTypeObject * pType = &PyEventReverb::s_type_ );   

	void fini();

    //  Attributes
    bool active() const;
    void active( bool active );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, active, active ); 

    float minDistance() const;
    void minDistance( float newMinDistance);
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, minDistance, minDistance ); 

    float maxDistance() const;
    void maxDistance( float newMaxDistance);
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, maxDistance, maxDistance ); 

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, position, position )
	const Vector3 & position() const;
	void position( const Vector3 & v );

    PyObject * pyGet_source();
    int pySet_source( PyObject * value );

    void update3DAttributes();

	PY_FACTORY_DECLARE()

protected:
    SoundManager::EventReverb *eventReverb_;
	mutable bool active_;
    mutable float minDistance_;
    mutable float maxDistance_;

	Vector3 position_;
    MatrixProviderPtr source_;

private:
    PyEventReverb();
	~PyEventReverb();

	PyEventReverb(const PyEventReverb&);
	PyEventReverb& operator=(const PyEventReverb&);
};

typedef SmartPointer< PyEventReverb > PyEventReverbPtr;

BW_END_NAMESPACE

#endif // FMOD_SUPPORT


#endif // PY_EVENT_CATEGORY_HPP
