#include "pch.hpp"
#include "py_music_system.hpp"

#include "fmod_config.hpp"
#include "cstdmf/guard.hpp"

#if FMOD_SUPPORT
#include <fmod_errors.h>

DECLARE_DEBUG_COMPONENT2( "PyMusicSystem", 0 );


BW_BEGIN_NAMESPACE

/*~ function FMOD.getMusicSystem
 *
 *  Returns the music system object associated with the 
 *	FMOD EventSystem.
 */
PyObject *py_getMusicSystem( PyObject *args )
{
	BW_GUARD;
	return SoundManager::instance().pyMusicSystem();
}

PY_MODULE_FUNCTION( getMusicSystem, _FMOD );

/*~ class FMOD.MusicSystem
 *
 *  A MusicSystem provides access to the FMOD::MusicSystem
 *  Please refer to "fmodex.chm" in your FMOD install directory
 *  for more information.
 *
 *	The MusicSystem singleton can be accessed via the 
 *	FMOD.getMusicSystem function.
 */
PY_TYPEOBJECT( PyMusicSystem );

PY_BEGIN_METHODS( PyMusicSystem )

    /*~ function MusicSystem.promptCue
	 *
	 *  Triggers an instantaneous cue. 
	 *
	 *  @param	name	The name of the cue.
	 *
	 *  @return			Nothing.
	 */
    PY_METHOD( promptCue )

    /*~ function MusicSystem.setParameterValue
	 *
	 *  Sets the value of a given music system parameter.  
	 *
	 *  @param	name	The name of the parameter.
	 *  @param	value	The value to set the parameter to.
	 *
	 *  @return			Nothing.
	 */
    PY_METHOD( setParameterValue )

    /*~ function MusicSystem.getParameterValue
	 *
	 *  Gets the value of a given music system parameter.
	 *
	 *  @param	name	The name of the parameter.
	 *
	 *  @return			Float.
	 */
    PY_METHOD( getParameterValue )

    /*~ function MusicSystem.setCallback
	 *
	 *  Sets a callback so that when certain music behaviours happen, they
     *  can be caught by the user. 
	 *
	 *  @param	callback	The callable python object, which will be called 
	 *						with a single parameter.
	 *
	 *  @return			Nothing.
	 */
    PY_METHOD( setCallback )

    /*~ function MusicSystem.reset
	 *
	 *  Resets the music system.
	 *
	 *  @param	type    	The type of callback "SEGMENT_START", "SEGMENT_END", "RESET" or "BEAT".
	 *  @param	callback    The python method to call.
	 *
	 *  @return			Nothing.
	 */
    PY_METHOD( reset )

    /*~ function MusicSystem.loadSoundData
	 *
	 *  Loads sound data for the music system.  
	 *
	 *  @param	blocking    Whether the loading should block or be asynchronous.
	 *
	 *  @return			    Nothing.
	 */
    PY_METHOD( loadSoundData )

    /*~ function MusicSystem.freeSoundData
	 *
	 *  Frees sound data for the music system.  
	 *
	 *  @return			Nothing.
	 */
    PY_METHOD( freeSoundData )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyMusicSystem )

    /*~ attribute MusicSystem.memoryUsed
	 *
	 *  Memory used by this event group.
	 *
	 *  @type	Unsigned Int
	 */
	PY_ATTRIBUTE( memoryUsed )

    /*~ attribute MusicSystem.muted
	 *
	 *  Mutes or unmutes the music system.  
	 *
	 *  @type	Bool
	 */
	PY_ATTRIBUTE( muted )

    /*~ attribute MusicSystem.paused
	 *
	 *  Pauses or unpauses the music system.  
	 *
	 *  @type	Bool
	 */
	PY_ATTRIBUTE( paused )

    /*~ attribute MusicSystem.volume
	 *
	 *  Volume of the music system
	 *
	 *  @type	Float
	 */
	PY_ATTRIBUTE( volume )

PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS_DECLARE( PyMusicSystem )
PY_SCRIPT_CONVERTERS( PyMusicSystem )

static PyFactoryMethodLink PyMusicSystem_MethodLink( "_FMOD", "MusicSystem",
	&PyMusicSystem::s_type_ );

/// Constructor
PyMusicSystem::PyMusicSystem( SoundManager::MusicSystem * pMusicSystem, PyTypeObject * pType ) :
	PyObjectPlus( pType ),
    musicSystem_( pMusicSystem ),
	muted_( false ),
	paused_( false ),
	volume_( 0.0f )
{
	BW_GUARD;	
    if (musicSystem_)
	{
		FMOD_RESULT result = musicSystem_->setCallback( musicCallback, NULL );
		SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::PyMusicSystem: Unable to set callback" );
	}
}


PyMusicSystem::~PyMusicSystem()
{
	fini();
}

void PyMusicSystem::fini()
{
	musicSystem_ = NULL;
}

void PyMusicSystem::promptCue( BW::string name )
{
    BW_GUARD;
	if (musicSystem_)
	{
		FMOD_MUSIC_ITERATOR itr;
		FMOD_RESULT result = musicSystem_->getCues( &itr, name.c_str() );
		if (!SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::promptCue" ))
		{
			return;
		}

		if (!itr.value)
		{
			//cue not found
			ERROR_MSG( "PyMusicSystem::promptCue(): "
				"Could not find parameter '%s'.\n", name.c_str() );
			return;
		}

		result = musicSystem_->promptCue( itr.value->id );
		SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::promptCue" );
	}
}

bool PyMusicSystem::muted()
{
    BW_GUARD;
    if (musicSystem_)
	{
		FMOD_RESULT result = musicSystem_->getMute( &muted_ );
		SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::muted" );
	}
    return muted_;
}

void PyMusicSystem::muted( bool newMute )
{
    BW_GUARD;
	muted_ = newMute;
	if (musicSystem_)
	{
		FMOD_RESULT result = musicSystem_->setMute( newMute );
		SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::muted" );
	}
}

bool PyMusicSystem::paused()
{
    BW_GUARD;
    if (musicSystem_)
	{
		FMOD_RESULT result = musicSystem_->getPaused( &paused_ );
		SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::paused" );
	}
    return paused_;
}

void PyMusicSystem::paused( bool newPaused )
{
    BW_GUARD;
	paused_ = newPaused;
	if (musicSystem_)
	{
		FMOD_RESULT result = musicSystem_->setPaused( paused_ );
		SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::paused" );
	}
}

float PyMusicSystem::volume()
{
    BW_GUARD;
    if (musicSystem_)
	{
	    FMOD_RESULT result = musicSystem_->getVolume( &volume_ );
		SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::volume" );
	}

    return volume_;
}

void PyMusicSystem::volume( float newValue )
{
    BW_GUARD;
	volume_ = newValue;
	if (musicSystem_)
	{
		FMOD_RESULT result = musicSystem_->setVolume( volume_ );
		SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::volume" );
	}
}

float PyMusicSystem::getParameterValue( const BW::string& name )
{
    BW_GUARD;
	if (musicSystem_)
	{
		FMOD_MUSIC_ITERATOR itr;
		FMOD_RESULT result = musicSystem_->getParameters( &itr, name.c_str() );
		if (!SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::getParameterValue" ))
		{
			return 0.f;
		}

		if (!itr.value)
		{
			char buffer[128];
			sprintf( buffer, "PyMusicSystem.getParameterValue() "
				"Could not find parameter: %s", name.c_str() );
				PyErr_SetString( PyExc_StandardError, buffer);
			return 0.f;
		}

		float paramValue;
		result = musicSystem_->getParameterValue( itr.value->id , &paramValue );
		SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::getParameterValue" );
		return paramValue;
	}
	else
	{
		return 0.0f;
	}
}

void PyMusicSystem::setParameterValue( const BW::string& name, float newParameterValue )
{
    BW_GUARD;
	if (musicSystem_)
	{
		FMOD_MUSIC_ITERATOR itr;
		FMOD_RESULT result = musicSystem_->getParameters( &itr, name.c_str() );

		if (!SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::setParameterValue" ))
		{
			return;
		}

		if (!itr.value)
		{
			char buffer[128];
			sprintf( buffer, "PyMusicSystem.setParameterValue() "
				"Could not find parameter: %s", name.c_str() );
				PyErr_SetString( PyExc_StandardError, buffer);
			return;
		}

		result = musicSystem_->setParameterValue( itr.value->id , newParameterValue );
		SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::setParameterValue" );
	}
}


void PyMusicSystem::reset()
{
	if (musicSystem_)
	{
	    FMOD_RESULT result = musicSystem_->reset();
	    SoundManager::FMOD_ErrCheck( result, "PyMusicSystem::reset" );
	}
}


void PyMusicSystem::loadSoundData(bool blocking)
{
	if (musicSystem_)
	{
		FMOD_EVENT_MODE mode = blocking ? FMOD_EVENT_DEFAULT : FMOD_EVENT_NONBLOCKING;
		FMOD_RESULT result = musicSystem_->loadSoundData( FMOD_EVENT_RESOURCE_SAMPLES, mode);    
		SoundManager::FMOD_ErrCheck(result, "PyMusicSystem::loadSoundData");
	}
}


void PyMusicSystem::freeSoundData(/*bool waitUntilReady*/)
{
	if (musicSystem_)
	{
		// TODO: add 'wait until ready' and then retry from SoundManager->update if it fails.
		FMOD_RESULT result = musicSystem_->freeSoundData(true);
		SoundManager::FMOD_ErrCheck(result, "PyMusicSystem::freeSoundData");
	}
}

/**
 *  Retrieve detailed memory usage information about this object.  
 */
unsigned int PyMusicSystem::memoryUsed()
{
    BW_GUARD;
    unsigned int mem = 0;

#if FMOD_SUPPORT_MEMORYINFO
# if 1
    FMOD_RESULT result = musicSystem_->getMemoryInfo(FMOD_MEMBITS_ALL, FMOD_EVENT_MEMBITS_ALL, &mem, NULL);
# else
    /*
        TODO: use FMOD_MEMBITS and FMOD_EVENT_MEMBITS flags 
    to specifiy what kind of memory is wanted
    */
# endif
#endif
    return mem;
}


/*
        CALLBACK STUFF
*/
static void FMOD_MUSIC_CALLBACKTYPE_initMaps();
PY_BEGIN_ENUM_MAP( FMOD_MUSIC_CALLBACKTYPE, FMOD_MUSIC_CALLBACKTYPE_ )
    PY_ENUM_VALUE( FMOD_MUSIC_CALLBACKTYPE_SEGMENT_START )
    PY_ENUM_VALUE( FMOD_MUSIC_CALLBACKTYPE_SEGMENT_END )
    PY_ENUM_VALUE( FMOD_MUSIC_CALLBACKTYPE_RESET )
    PY_ENUM_VALUE( FMOD_MUSIC_CALLBACKTYPE_BEAT )
PY_END_ENUM_MAP()

namespace EnumMusicCallback
{
    PY_ENUM_CONVERTERS_DECLARE( FMOD_MUSIC_CALLBACKTYPE )
    PY_ENUM_CONVERTERS_SCATTERED( FMOD_MUSIC_CALLBACKTYPE )
}

FMOD_RESULT F_CALLBACK PyMusicSystem::musicCallback(
  FMOD_MUSIC_CALLBACKTYPE  type, 
  void *  param1, 
  void *  param2, 
  void *  userdata)
{
	PyMusicSystem* music = SoundManager::instance().pyMusicSystem();
	if (music)
	{
		music->doCallback(type);
	}
    return FMOD_OK;
}

void PyMusicSystem::doCallback( FMOD_MUSIC_CALLBACKTYPE type )
{
    BW_GUARD;
    if (callbackMap_.count(type))
    {
		Py_INCREF( callbackMap_[type].get() );
        Script::call( callbackMap_[type].get(),
            PyTuple_New(0),
			"PyMusicSystem::doCallback: ", true );
    }
}

void PyMusicSystem::setCallback( PyObjectPtr type, PyObjectPtr callback)
{
    BW_GUARD;
    FMOD_MUSIC_CALLBACKTYPE enumType;
    if (EnumMusicCallback::Script::setData( type.get(), enumType, "PyMusicSystem::setCallback" ) )
    {
        ERROR_MSG("PySound::setCallback cannot convert "
            "argument %s to music callback type", type->ob_type->tp_name );
        return;
    }
    callbackMap_[enumType] = callback;
}

BW_END_NAMESPACE

#endif // FMOD_SUPPORT

// pymusicsystem.cpp
