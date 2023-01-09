#ifndef PY_MUSIC_SYSTEM_HPP
#define PY_MUSIC_SYSTEM_HPP

#include "fmod_config.hpp"
#if FMOD_SUPPORT

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "sound_manager.hpp"

#include <fmod_event.hpp>


BW_BEGIN_NAMESPACE

/**
 *  A PyMusicSystem  is a wrapper for the FMOD::MusicSystem
 *
 *  Please see the FMOD Designer API documentation and the FMOD Designer
 *  User Manual, both are located in your FMOD install directory.
 */
class PyMusicSystem : public PyObjectPlus
{
	Py_Header( PyMusicSystem, PyObjectPlus )

public:
    PyMusicSystem( SoundManager::MusicSystem * pMusicSystem, PyTypeObject * pType = &PyMusicSystem::s_type_ );

	void fini();

    //  Methods
    void promptCue( BW::string name );
	PY_AUTO_METHOD_DECLARE( RETVOID, promptCue, ARG( BW::string, END ) )

    float getParameterValue( const BW::string& name );
    PY_AUTO_METHOD_DECLARE( RETDATA, getParameterValue, ARG( BW::string, END ) )

    void setParameterValue( const BW::string& name, float value );
    PY_AUTO_METHOD_DECLARE( RETVOID, setParameterValue, ARG( BW::string, ARG( float, END ) ) )

    void setCallback( PyObjectPtr type, PyObjectPtr callback);
	PY_AUTO_METHOD_DECLARE( RETVOID, setCallback, ARG( PyObjectPtr, ARG( PyObjectPtr, END ) ) );

    void reset();
	PY_AUTO_METHOD_DECLARE( RETVOID, reset, END );

    void loadSoundData( bool blocking = true );
	PY_AUTO_METHOD_DECLARE( RETVOID, loadSoundData, OPTARG( bool, true, END ) );

    void freeSoundData( /*bool waitUntilReady = true*/ );
	PY_AUTO_METHOD_DECLARE( RETVOID, freeSoundData, END/*OPTARG( bool, true, END)*/ );

	//  Attributes
    bool muted();
    void muted( bool newMute );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, muted, muted ); 

    bool paused();
    void paused( bool newPaused );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, paused, paused );

	float volume();
	void volume( float newValue );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, volume, volume );

    unsigned int memoryUsed();
	PY_RO_ATTRIBUTE_DECLARE( memoryUsed(), memoryUsed ); 


protected:
    BW::map<FMOD_MUSIC_CALLBACKTYPE, PyObjectPtr> callbackMap_;
    static FMOD_RESULT F_CALLBACK musicCallback(
      FMOD_MUSIC_CALLBACKTYPE  type, 
      void *  param1, 
      void *  param2, 
      void *  userdata);
    void doCallback( FMOD_MUSIC_CALLBACKTYPE type );

    SoundManager::MusicSystem* musicSystem_;

private:
	~PyMusicSystem();

    //  Hide copy constructor and assignment operator.
	PyMusicSystem(const PyMusicSystem&);
	PyMusicSystem& operator=(const PyMusicSystem&);

private:
	bool muted_;
	bool paused_;
	float volume_;
};

typedef SmartPointer< PyMusicSystem > PyMusicSystemPtr;

BW_END_NAMESPACE

#endif // FMOD_SUPPORT

#endif // PY_MUSIC_SYSTEM_HPP
