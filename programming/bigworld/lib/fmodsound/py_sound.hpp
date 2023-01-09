#ifndef PY_SOUND_HPP
#define PY_SOUND_HPP

#include "fmod_config.hpp"

#if FMOD_SUPPORT
#include <fmod.hpp>

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"
#include "pyscript/stl_to_py.hpp"

#include "sound_manager.hpp"


BW_BEGIN_NAMESPACE

class PySound;
class PyModel;

/**
 *  A PySound is a wrapper for an FMOD::Event.  It can be used to trigger and
 *  re-trigger a sound event, and provides an interface for inspecting various
 *  attributes of a sound event.
 */
class PySound : public PyObjectPlus
{
    friend class PySoundParameter;
    friend class SoundManager;
	Py_Header( PySound, PyObjectPlus )

private:
#if ENABLE_JFIQ_HANDLING
	PySound( SoundManager::Event * pEvent, const BW::string &path,
		bool JFIQ = false, PyTypeObject* pType = &PySound::s_type_ );
#else
	PySound( SoundManager::Event * pEvent, const BW::string &path,
		PyTypeObject* pType = &PySound::s_type_ );
#endif

	~PySound();

public:

	void fini();

	PyObject* param( const BW::string& name );
	PY_AUTO_METHOD_DECLARE( RETOWN, param, ARG( BW::string, END ) );

	PyObject* getParentProject();
	PY_AUTO_METHOD_DECLARE( RETOWN, getParentProject, END );

	PyObject* getParentGroup();
	PY_AUTO_METHOD_DECLARE( RETOWN, getParentGroup, END );

	PyObject* getParentCategory();
	PY_AUTO_METHOD_DECLARE( RETOWN, getParentCategory, END );

	bool play();
	PY_AUTO_METHOD_DECLARE( RETDATA, play, END );

	bool stop();
	PY_AUTO_METHOD_DECLARE( RETDATA, stop, END );

    bool muted();
    void muted( bool newMute );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, muted, muted );

    bool paused();
    void paused( bool newPaused );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, paused, paused );

	float pitch();
	void pitch( float newPitch );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, pitch, pitch );

	float volume();
	void volume( float newValue );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, volume, volume );

    bool isVirtual();
	PY_RO_ATTRIBUTE_DECLARE( isVirtual(), isVirtual );

	float duration();
	PY_RO_ATTRIBUTE_DECLARE( duration(), duration );

	const char *name() const;
	PY_RO_ATTRIBUTE_DECLARE( name(), name );

	BW::string state();
	PY_RO_ATTRIBUTE_DECLARE( state(), state );

	PyObject * pyGet_position();
	int pySet_position( PyObject * position );

	void setModel( PyModel *const pModel );

	bool refresh( SoundManager::EventState okMask = FMOD_EVENT_STATE_READY );

    bool unloaded();
    void unloaded( bool unload );

	Vector3 calculateVelocity(const Vector3& position, const float deltaTime );
    bool update(const Vector3& position, const Vector3& orientation = Vector3::zero(), const float deltaTime = 0.f );

    /*
            CALLBACK STUFF
    */

public:
	void doCallback( FMOD_EVENT_CALLBACKTYPE  type);

	void setCallback( PyObjectPtr type, PyObjectPtr callback);    
	PY_AUTO_METHOD_DECLARE( RETVOID, setCallback, ARG( PyObjectPtr, ARG( PyObjectPtr, END ) ) );

protected:
    BW::map<FMOD_EVENT_CALLBACKTYPE, PyObjectPtr> callbackMap_;
    static FMOD_RESULT F_CALLBACK eventCallback(
                  FMOD_EVENT *  event, 
                  FMOD_EVENT_CALLBACKTYPE  type, 
                  void *  param1, 
                  void *  param2, 
                  void *  userdata);



#if ENABLE_JFIQ_HANDLING
public:
    bool isJFIQ()     const;
    bool updateJFIQ();
	bool isValid();

protected:  
    bool justFailIfQuietest_;

#endif //ENABLE_JFIQ_HANDLING

	enum Status
	{
		STATUS_PLAYING,		
		STATUS_STOLEN,// When there are more PySounds than avaliable event handles,some will be stolen.
		STATUS_STOPPED
	};

	Status status_;
	bool isStopped() const {return status_ == STATUS_STOPPED;}	
	bool isPlaying()  const {return status_ == STATUS_PLAYING;}
	bool isStolen()  const {return status_ == STATUS_STOLEN;}
	bool isInfoOnly();



	SoundManager::Event * pEvent_;

	// The group this sound belongs to
	SoundManager::EventGroup * pGroup_;

	// The absolute path to this sound, only set when unloading is allowed
    // or for JustFailIfQuietest Events.
	BW::string path_;

	// The index this sound resides at in its EventGroup
	int index_;

	// For a 3D sound attached to a model, this is the PyModel that this sound
	// is attached to.
	PyModel * pModel_;

	// The position of a 3D sound not attached to a PyModel.
	Vector3 * pPosition_;
    // This is the previous position of the event, if it is 3D this will determine
    // the velocity so FMOD can calculate doppler effect.
    Vector3 lastPosition_;


    // When this PySound's event's parent project is unloaded, this will
    // tell it's PySoundList that's it should be removed.
    bool unloaded_;
};

typedef SmartPointer< PySound > PySoundPtr;

BW_END_NAMESPACE

#endif // FMOD_SUPPORT

#endif // PY_SOUND_HPP
