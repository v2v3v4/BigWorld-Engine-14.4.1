#include "pch.hpp"

#include "sound_manager.hpp"
#include "py_sound.hpp"
#include "resmgr/bwresource.hpp"
#include "cstdmf/profiler.hpp"
#include "cstdmf/guard.hpp"

#if FMOD_SUPPORT

#include "cstdmf/bgtask_manager.hpp"
#include "py_music_system.hpp"
#include "py_event_group.hpp"
#include "py_event_project.hpp"
#include "py_event_category.hpp"
#include "py_event_reverb.hpp"


DECLARE_DEBUG_COMPONENT2( "SoundManager", 0 );	// debugLevel for this file


BW_BEGIN_NAMESPACE

#include "fmod_info_getters.ipp"

/// FMod SoundManager Singleton
BW_SINGLETON_STORAGE( SoundManager )

static const int JUST_FAIL_IF_QUIETEST = 5;
static const int STREAM_FROM_DISK      = 0;

// FMOD memory tracking
MEMTRACKER_DECLARE( FMOD, "ThirdParty - FMOD", 0 );

static void* F_CALLBACK fmod_malloc( unsigned int size, FMOD_MEMORY_TYPE type )
{
	MEMTRACKER_SCOPED( FMOD );
	void* mem = bw_malloc( size );
	if ( !mem )
	{
		CRITICAL_MSG( "Out of memory. Exiting.\n" );
	}
	return mem;
}

static void* F_CALLBACK fmod_realloc( void * ptr, unsigned int size, FMOD_MEMORY_TYPE type )
{
	MEMTRACKER_SCOPED( FMOD );
	void* mem = bw_realloc( ptr, size );
	if ( !mem )
	{
		CRITICAL_MSG( "Out of memory. Exiting.\n" );
	}
	return mem;
}

static void F_CALLBACK fmod_free( void * ptr, FMOD_MEMORY_TYPE type )
{
	bw_free( ptr );
}

/*~ module FMOD
 *	@components{ client, tools }
 *
 *	The FMOD module provides access to the FMOD Ex integration, if
 *	it is enabled.
 */

namespace
{
// TODO: Move this out of static initialisation if it causes problems
SoundManager s_soundManagerInstance;
}

SoundManager::ErrorLevel SoundManager::s_errorLevel_ = SoundManager::ERROR_LEVEL_WARNING;

/**
 *  Gets the music Python object associated with this EventSystem.
 *	There is only ever one music object instance.
 */
PyMusicSystem * SoundManager::pyMusicSystem()
{
    BW_GUARD;
    if (!musicSystem_)
    {
		if (eventSystem_)
		{
			FMOD::MusicSystem* pMusicSystem;
			FMOD_RESULT result = eventSystem_->getMusicSystem( &pMusicSystem );
			FMOD_ErrCheck( result, "SoundManager::pyMusicSystem: Couldn't get music system object" );
			musicSystem_ = new PyMusicSystem( pMusicSystem );
		}
		else
		{
			musicSystem_ = new PyMusicSystem( NULL );
		}
    }

	Py_XINCREF( musicSystem_ );
	return musicSystem_;
}

PyObject* SoundManager::createPySound( const BW::string &path )
{   
	BW_GUARD;
	if (!eventSystem_)
	{
		PyErr_Format( PyExc_RuntimeError, 
			"Sound system is not initialised." );
		return pyError();
	}

#if ENABLE_JFIQ_HANDLING

    bool JFIQ = false;
    SoundManager::Event *pEvent = NULL;
    pEvent = get( path, FMOD_EVENT_INFOONLY );
    if (!pEvent)
	{
		PyErr_Format( PyExc_KeyError, "Could not load sound event '%s'. "
			"See logs for details.", path.c_str() );
        return pyError();
	}

    // Check if event has MAX_PLAYBACK_BEHAVIOUR = 5 ("just fail if quietest")
    {
        int maxPlaybackBehaviour = 0;
        FMOD_RESULT result = pEvent->getPropertyByIndex(FMOD_EVENTPROPERTY_MAX_PLAYBACKS_BEHAVIOR, 
                                             &maxPlaybackBehaviour);
        if (!SoundManager::FMOD_PyErrCheck(result, "PySound::PySound()"))
		{
            return pyError();
		}
        JFIQ = (maxPlaybackBehaviour == JUST_FAIL_IF_QUIETEST);
    }

    // If the Max Playback Behaviour isn't "just fail if quietest" attempt to get
    // a event handle using FMOD_EVENT_MODE = FMOD_EVENT_DEFAULT
    if (!JFIQ)
    {
        pEvent = get( path );
        if (!pEvent)
		{
			PyErr_Format( PyExc_KeyError, "Could not load sound event '%s'. "
				"See logs for details.", path.c_str() );
            return pyError();
		}
    }

    return new PySound( pEvent, path, JFIQ );
#else
    if (SoundManager::Event *pEvent = get( path ))
	{
        return new PySound( pEvent, path );
	}
    else
	{
		PyErr_Format( PyExc_KeyError, "Could not load sound event '%s'.", path.c_str() );
        return pyError();
	}
#endif
}

PyObject* SoundManager::pyEventGroup(const BW::string & groupPath)
{
    BW_GUARD;
	if (!eventSystem_)
	{
		PyErr_Format( PyExc_RuntimeError, 
			"Sound system is not initialised." );
		return pyError();
	}

    // Attempt to parse path to load from disk
    FMOD::EventProject * pProject = NULL;
    FMOD::EventGroup   * pGroup   = NULL;

    //BW::string path = '/';
    //path += groupPath;
    if (!parsePath( BW::string(groupPath), &pProject, &pGroup, NULL, true ) ||
        pGroup == NULL)
	{
		PyErr_Format( PyExc_KeyError, "Could not get event group '%s'. "
			"See logs for details.", groupPath.c_str() );
        return pyError();
	}

    void * data = NULL;
    if (!FMOD_PyErrCheck( pGroup->getUserData(&data), "SoundManager::pyEventGroup" ))
	{
        return pyError();
	}

    PyEventGroup *pyEventGroup = NULL;
    if (data)
	{
        pyEventGroup = static_cast< PyEventGroup * >( data );
		Py_INCREF( pyEventGroup );
	}
    else
	{
        pyEventGroup = new PyEventGroup(pGroup);
	}

    return pyEventGroup;
}

PyObject* SoundManager::pyEventProject(const BW::string & projectName)
{
    BW_GUARD;
	if (!eventSystem_)
	{
		PyErr_Format( PyExc_RuntimeError, 
			"Sound system is not initialised." );
		return pyError();
	}

    FMOD::EventProject * pProject = NULL;
    BW::string path = "/";
    path += projectName;
    if (!parsePath( BW::string(projectName), &pProject, NULL, NULL, true ) ||
        pProject == NULL)
	{
		PyErr_Format( PyExc_KeyError, "Could not get event project '%s'. "
			"See logs for details.", projectName.c_str() );
        return pyError();
	}

    void * data = NULL;
    if (!FMOD_PyErrCheck(pProject->getUserData(&data), "SoundManager::pyEventGroup"))
	{
        return pyError();
	}

    PyEventProject *pyEventProject = NULL;
    if (data)
	{
        pyEventProject = static_cast< PyEventProject * >( data );
		Py_INCREF( pyEventProject );    
	}
    else
	{
        pyEventProject = new PyEventProject(pProject);
	}

    return pyEventProject;
}

PyObject * SoundManager::pyEventCategory(const BW::string & categoryPath)
{
    BW_GUARD;
	if (!eventSystem_)
	{
		PyErr_Format( PyExc_RuntimeError, 
			"Sound system is not initialised." );
		return pyError();
	}

    FMOD::EventCategory * pCategory = NULL;
    FMOD_RESULT result = eventSystem_->getCategory( categoryPath.c_str(), &pCategory );
    if(!SoundManager::FMOD_ErrCheck( result, "SoundManager::pyEventCategory" ) ||
        pCategory == NULL)
	{
		PyErr_Format( PyExc_KeyError, "Could not get event category '%s'. "
			"See logs for details.", categoryPath.c_str() );
        return pyError();
	}

    void * data = NULL;
    if (!FMOD_PyErrCheck( pCategory->getUserData( &data ), "SoundManager::pyEventCategory" ))
	{
        return SoundManager::pyError();
	}

    PyEventCategory *pyEventCategory = NULL;
    if (data)
	{
        pyEventCategory = static_cast< PyEventCategory * >( data );
	}
    else
    {
        pyEventCategory = new PyEventCategory( pCategory );
        pyEventCategories_.push_back( pyEventCategory );
    }    

    Py_INCREF( pyEventCategory );
    return pyEventCategory;
}

PyObject* SoundManager::pyEventReverb( const BW::string & name )
{
    BW_GUARD;
	if (!eventSystem_)
	{
		PyErr_Format( PyExc_RuntimeError, 
			"Sound system is not initialised." );
		return pyError();
	}

	FMOD_REVERB_PROPERTIES prop;
    FMOD_RESULT result = eventSystem_->getReverbPreset( name.c_str(), &prop, NULL );
    if (result == FMOD_ERR_INVALID_PARAM)
    {
		PyErr_Format( PyExc_KeyError, 
			"'%s' is not a valid reverb preset name.", name.c_str() );
		return pyError();
    }

    if(!FMOD_PyErrCheck( result, "SoundManager::pyEventReverb" ))
	{
        return pyError();
	}

    FMOD::EventReverb * pReverb = NULL;
    result = eventSystem_->createReverb( &pReverb );
    if(!SoundManager::FMOD_PyErrCheck(result, "SoundManager::pyEventReverb"))
	{
        return pyError();
	}

	if (!pReverb)
	{
		PyErr_Format( PyExc_RuntimeError, 
			"SoundManager::pyEventReverb: createReverb returned "
			"NULL pointer for preset '%s'.", name.c_str() );
		return pyError();
	}

    result = pReverb->setProperties( &prop );
    if(!SoundManager::FMOD_PyErrCheck( result, "SoundManager::pyEventReverb" ))
	{
        return pyError();
	}

    PyEventReverb* eventReverb = new PyEventReverb( pReverb );
    pyEventReverbs_.push_back( eventReverb );

	Py_INCREF( eventReverb );
	return eventReverb;
}

// Sound Bank Loader
class SoundBankLoader
{
public:
	SoundBankLoader( const char* name, const char* file );

	static void onLoadCompleted( void* loader );
	static void createSoundBank( void* loader );

protected:
	BW::string name_;
	BW::string file_;
	DataSectionPtr ref_;
};

SoundBankLoader::SoundBankLoader( const char* name, const char* file )
{
	BW_GUARD;
	name_ = name;
	file_ = file;

	FileIOTaskManager::instance().addBackgroundTask( new CStyleBackgroundTask( "SoundBank Loader Task",&SoundBankLoader::createSoundBank, this, &SoundBankLoader::onLoadCompleted, this ) );
}

void SoundBankLoader::onLoadCompleted( void* loader )
{
	BW_GUARD;
	SoundBankLoader* l = (SoundBankLoader*)loader;

	if( l->ref_.exists() )
		SoundManager::instance().registerSoundBank( l->name_, l->ref_ );
	else
		ERROR_MSG( "SoundBankLoader::onLoadCompleted: loading sound bank '%s' failed.\n", l->file_.c_str() );

	// clean up SoundBankLoader
	bw_safe_delete(l);
}

void SoundBankLoader::createSoundBank( void* loader )
{
	BW_GUARD;
	SoundBankLoader* l = (SoundBankLoader*)loader;

	l->ref_ = BWResource::openSection( l->file_ );
}

// Sound Manager
SoundManager::SoundManager() :
    lastSet_( false ),
	eventSystem_( NULL ),
	defaultProject_( NULL ),
	listening_( false ),
	allowUnload_( true ),
	terrainOcclusionEnabled_( false ),
	modelOcclusionEnabled_( false ),
	maxSoundSpeed_( 100.f )
{}

SoundManager::~SoundManager()
{
	BW_GUARD;

	fini();
}


#if ENABLE_JFIQ_HANDLING
void SoundManager::addToJFIQList( PySound *pySound )
{
    BW_GUARD;
    justFailIfQuietestList_.push_back(pySound);
}

void SoundManager::removeFromJFIQList( PySound *pySound )
{
	BW_GUARD;
	for (BW::list<PySound *>::iterator eit = justFailIfQuietestList_.begin();
         eit != justFailIfQuietestList_.end();
         ++eit)
    {
        if ( (*eit) == pySound )
        {
	        justFailIfQuietestList_.erase( eit );
            break;
        }
    }
}
#endif

FMOD::EventSystem * SoundManager::getEventSystemObject()
{
    BW_GUARD;
    return eventSystem_;
}

bool SoundManager::FMOD_ErrCheck(FMOD_RESULT result, const char * msg, ... )
{
    BW_GUARD;
	if (result != FMOD_OK)
    {
        char errorString[256];
        va_list  argPtr;
        va_start(argPtr, msg);
        vsprintf(errorString, msg, argPtr);
        ERROR_MSG( "%s: %s\n", errorString, FMOD_ErrorString( result ) );
        va_end(argPtr);
        return false;
    }

    return true;
}

bool SoundManager::FMOD_PyErrCheck(FMOD_RESULT result, const char * msg, ... )
{
	BW_GUARD;
    if (result != FMOD_OK)
    {
        char errorString[256];
        va_list  argPtr;
        va_start(argPtr, msg);
        vsprintf(errorString, msg, argPtr);
		PyErr_Format( PyExc_RuntimeError, 
			"FMOD error: %s (%s)", errorString, FMOD_ErrorString( result ) );
        va_end(argPtr);
        return false;
    }

    return true;
}

void SoundManager::kill()
{
    BW_GUARD;
	NOTICE_MSG( "SoundManager::initialise: "
		"Sound init has failed, suppressing all sound error messages\n" );

    SoundManager::errorLevel( SoundManager::ERROR_LEVEL_SILENT );

	if (eventSystem_ != NULL)
    {
        FMOD_RESULT result = eventSystem_->release();
        FMOD_ErrCheck( result, "SoundManager::kill: Unable to release EventSystem object" );
        eventSystem_ = NULL;
    }
}


/**
 *  Initialises the sound manager and devices.
 */
bool SoundManager::initialise( DataSectionPtr config )
{
	BW_GUARD;
    FMOD::System    *system;

	// check what we're gonna do when play()/get() calls fail
	if (config != NULL)
	{
		BW::string errorLevel = config->readString( "errorLevel", "warning" );
		if (errorLevel == "silent")
		{
			SoundManager::errorLevel( SoundManager::ERROR_LEVEL_SILENT );
		}
		else if (errorLevel == "warning")
		{
			SoundManager::errorLevel( SoundManager::ERROR_LEVEL_WARNING );
		}
		else if (errorLevel == "exception")
		{
			SoundManager::errorLevel( SoundManager::ERROR_LEVEL_EXCEPTION );
		}
		else
		{
			ERROR_MSG( "SoundManager::initialise: "
				"Unrecognised value for soundMgr/errorLevel: %s\n",
				errorLevel.c_str() );

			SoundManager::errorLevel( SoundManager::ERROR_LEVEL_WARNING );
		}
	}

    int channels;
	if (config == NULL)
    {
        channels = 64;
    }
    else
    {
        channels = config->readInt( "channels", 64 );   
    }

	// Set memory management functions and init FMOD
	FMOD_RESULT result;

	result = FMOD::Memory_Initialize(NULL, 0, fmod_malloc, fmod_realloc, fmod_free);
	if ( result == FMOD_OK )
	{
		result = FMOD::EventSystem_Create( &eventSystem_ );
	}

	// Warning, enabling this will make FMOD crawl as it outputs MB's of data
	// to 'FMOD.TXT' in the CWD. Also, to take advantage of the logging, you MUST
	// link to fmod_event_netL.lib instead of fmod_event_net.lib (note the 'L')
#if FMOD_DEBUG_LOGGING
	result = FMOD::Debug_SetLevel( FMOD_DEBUG_LEVEL_ALL | FMOD_DEBUG_TYPE_FILE | FMOD_DEBUG_TYPE_EVENT );
    FMOD_ErrCheck( result, "SoundManager::initialise: "
		"Couldn't set Logging information" )
        /*
	result = FMOD::Debug_SetLevel( FMOD_DEBUG_ALL );
    FMOD_ErrCheck( result, "SoundManager::initialise: "
		"Couldn't create event system" )
        */
#endif

	if (!FMOD_ErrCheck( result, "SoundManager::initialise: "
		"Couldn't create event system" ))
    {
        kill();
        return false;
    }

    {    
        FMOD_RESULT      result;
        unsigned int     version;
        FMOD_SPEAKERMODE speakermode;
        FMOD_CAPS        caps;

        result = eventSystem_->getSystemObject(&system);
        if (!FMOD_ErrCheck( result, "SoundManager::initialise: "
            "Couldn't get FMOD::System object" ))
        {
            kill();
            return false;
        }

        result = system->getVersion(&version);
        if (!FMOD_ErrCheck( result, "SoundManager::initialise: "
	            "Couldn't get version" ))
        {
            kill();
            return false;
        }

        if (version < FMOD_VERSION)
        {
            ERROR_MSG("Error!  You are using an old version"
               "of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);

            kill();
            return false;
        }

        result = system->getDriverCaps(0, &caps, 0, 0, &speakermode);
        if (!FMOD_ErrCheck( result, "SoundManager::initialise: "
                "Couldn't get speaker mode" ))
        {
            kill();
            return false;
        }

        result = system->setSpeakerMode(speakermode);
        if (!FMOD_ErrCheck( result, "SoundManager::initialise: "
                "Couldn't set speaker mode" ))
        {
            kill();
            return false;
        }        

        if (caps & FMOD_CAPS_HARDWARE_EMULATED)             /* The user has the 'Acceleration' slider set to off!  This is really bad for latency!. */
        {                                                   /* You might want to warn the user about this. */
            WARNING_MSG( "The control panel 'Acceleration' slider set"
                " to off!  This is really bad for latency!" );

            result = system->setDSPBufferSize(1024, 10);    /* At 48khz, the latency between issuing an fmod command and hearing it will now be about 213ms. */
            if (!FMOD_ErrCheck( result, "SoundManager::initialise: "
                    "Couldn't set DSP buffer size" ))
            {
                kill();
                return false;
            }   
        }
        int initFlags = FMOD_INIT_NORMAL;

#if !CONSUMER_CLIENT_BUILD
	    if (config->readBool( "enableProfiler", false ))
            initFlags |= FMOD_INIT_ENABLE_PROFILE;
#endif

        result = eventSystem_->init(channels, initFlags, NULL);
        if (result == FMOD_ERR_OUTPUT_CREATEBUFFER)         /* Ok, the speaker mode selected isn't supported by this soundcard.  Switch it back to stereo... */
        {
            INFO_MSG( "Selected speaker mode not supported by"
                " hardware: switching to stereo." );
            result = system->setSpeakerMode(FMOD_SPEAKERMODE_STEREO);
            if (!FMOD_ErrCheck( result, "SoundManager::initialise: "
				"Couldn't set speaker mode" ))
            {
                kill();
                return false;
            }    
            result = eventSystem_->init(channels, FMOD_INIT_NORMAL, 0);
            if (!FMOD_ErrCheck( result, "SoundManager::initialise: "
				"Couldn't initialise event system" ))
            {
                kill();
                return false;
            } 
        }
        else
        {
            if (!FMOD_ErrCheck( result, "SoundManager::initialise: "
				"Couldn't initialise event system" ))
            {
                kill();
                return false;
            }   
        }
    }  	

	// Break out now if XML config wasn't passed in
	if (config == NULL)
		return true;

	this->setPath( config->readString( "mediaPath", "" ) );

	DataSectionPtr banks = config->openSection( "soundbanks" );

	if (banks)
	{
		BW::list< BW::string > preloaded;
		BW::list< BW::string > streamed;
		BW::list< BW::string >::iterator it;

		//First of all build up 2 seperate lists for preloaded and streamed projects
		for (int i=0; i < banks->countChildren(); ++i)
		{
			DataSectionPtr file = banks->openChild( i );
			if (file)
			{
				BW::string name = file->readString( "name", file->asString() );
				projectFiles_.push_back( name );
				if (file->readBool( "preload", false ))
				{
					preloaded.push_back( name );
				}
				else
				{
					streamed.push_back( name );
				}
			}
		}

		//Now load all the preloaded projects
		it = preloaded.begin();
		while (it != preloaded.end())
		{
			if (this->loadEventProject( *it ))
			{
				INFO_MSG( "SoundManager::initialise: "
					"Loaded sound project %s\n",
					it->c_str() );
			}
			else
			{
				ERROR_MSG( "SoundManager::initialise: "
					"Failed to load sound project %s\n",
					it->c_str() );
			}
			it++;
		}

		//Re-use the preloaded list for the sound banks to preload
		preloaded.clear();
		getSoundBanks( preloaded );

		//Preload all the sound banks
		it = preloaded.begin();
		while (it != preloaded.end())
		{
			INFO_MSG( "SoundManager::initialise: "
				"Starting preload of sound bank %s\n",
				it->c_str() );
			DataSectionPtr data = NULL;
			registerSoundBank( *it, data );
			it++;
		}

		//Now load all the projects with non-preloaded soundbanks
		it = streamed.begin();
		while (it != streamed.end())
		{
			if (this->loadEventProject( *it ))
			{
				INFO_MSG( "SoundManager::initialise: "
					"Loaded sound project %s\n",
					it->c_str() );
			}
			else
			{
				ERROR_MSG( "SoundManager::initialise: "
					"Failed to load sound project %s\n",
					it->c_str() );
			}
			it++;
		}
	}
	else
	{
		WARNING_MSG( "SoundManager::initialise: "
			"No <soundMgr/soundbanks> config section found, "
			"no sounds have been loaded\n" );
	}

#if !CONSUMER_CLIENT_BUILD
	// Net event system stuff
	if (config->readBool( "networkUpdates", true ))
	{
		result = FMOD::NetEventSystem_Init( eventSystem_, 0 );

		if ( FMOD_ErrCheck( result, "SoundManager::initialise: "
				"Couldn't initialise net layer"))
		{
			listening_ = true;
        }
	}
#endif

	// Is unloading allowed?
	this->allowUnload( config->readBool( "allowUnload", this->allowUnload() ) );

	// Occlusion enabled?
	this->terrainOcclusionEnabled( config->readBool( "enableTerrainOcclusion", false ) );
	this->modelOcclusionEnabled( config->readBool( "enableModelOcclusion", false ) );

	// Speed checking
	this->maxSoundSpeed( config->readFloat( "maxSoundSpeed", 100.f ) );

    // maxWorldSize must be set before any occlusion geometry is created.
    system->setGeometrySettings( config->readFloat( "maxWorldSize", 1000.f ) );

	return true;
}

void SoundManager::fini()
{
	BW_GUARD;

	// Let go of any music system we have
	if (musicSystem_)
	{
		musicSystem_->decRef();
		musicSystem_ = NULL;
	}

	// Clean up PyEventProjects
    for (EventProjects::iterator itr = eventProjects_.begin();
        itr != eventProjects_.end();
        itr++)
    {
        void * data = NULL;
        FMOD_ErrCheck((*itr).second->getUserData(&data), 
            "SoundManager::fini(): Unable to get project user data");

        if (data)
        {
            PyEventProject *pyEventProject = static_cast< PyEventProject * >( data );
			pyEventProject->fini();
            pyEventProject->decRef();
        }
    }
    eventProjects_.clear();

    // Clean up PyEventGroups  
    for (EventGroups::iterator itr = eventGroups_.begin();
        itr != eventGroups_.end();
        itr++)
    {
        void * data;
        FMOD_ErrCheck((*itr).second->getUserData(&data), 
            "SoundManager::fini(): Unable to get group user data");

        if (data)
        {
            PyEventGroup *pyEventGroup = static_cast< PyEventGroup * >( data );
            pyEventGroup->fini();
            pyEventGroup->decRef();
        }
    }
    eventGroups_.clear();

    // Clean up PyEventCategories   
    for (PyEventCategories::iterator itr = pyEventCategories_.begin();
        itr != pyEventCategories_.end();
        itr++) 
    {
        (*itr)->fini();
        (*itr)->decRef();
    }
    pyEventCategories_.clear();

    // Clean up PyEventReverbs 
    for (PyEventReverbs::iterator itr = pyEventReverbs_.begin();
        itr != pyEventReverbs_.end();
        itr++) 
    {
        (*itr)->fini();
        (*itr)->decRef();
    }
    pyEventReverbs_.clear();


	// clear out cached groups, they may no longer be defined, we can 
	// re-cache them later as needed. don't need to clear out individual
	// groups, since that's done by EventProject::release()
	eventGroups_.clear();
#if 0
    // Projects will be unloaded when eventSystem_->release() is called.
	for (EventProjects::iterator it = eventProjects_.begin();
		it != eventProjects_.end(); ++it)
	{
		FMOD_RESULT result = it->second->release();
        FMOD_ErrCheck( result, "SoundManager::fini(): "
				"Failed to release project '%s'",
				it->first.c_str());
	}
#endif

    if (!eventSystem_) 
	{
		return;
	}

	// free all sound banks
    EventSystemInfo sysInfo(eventSystem_);

	SoundBankMap::iterator it  = soundBankMap_.begin();
	SoundBankMap::iterator end = soundBankMap_.end();
	for ( ; it != end; ++it )
	{
        FMOD_RESULT result = eventSystem_->unloadFSB( (it->first + ".fsb").c_str(), 0 );
        FMOD_ErrCheck( result, "SoundManager::fini(): unable to unload soundbank: %s\n" , (it->first + ".fsb").c_str() );

		// release the binary data
		it->second.pBinary_ = NULL;	

		// release FMOD::Sound
		it->second.pSoundData_->release();
		it->second.pSoundData_ = NULL;
    }
	soundBankMap_.clear();

	StreamingSoundBankMap::iterator itStream  = streamingSoundBankMap_.begin();
	StreamingSoundBankMap::iterator endStream = streamingSoundBankMap_.end();;
	for ( ; itStream != endStream; ++itStream )
    {
        int streamCount = 0;
        SoundList& soundList = itStream->second;
        SoundList::iterator itr = soundList.begin();
        SoundList::iterator end = soundList.end();

        for ( ; itr != end; ++itr)
        {
            FMOD_RESULT result = eventSystem_->unloadFSB(
				(itStream->first + ".fsb").c_str(), streamCount );
            FMOD_ErrCheck( result, "SoundManager::unregisterSoundBank(): "
					"unable to unload soundbank: %s.fsb (instance %d)\n",
				itStream->first.c_str(), streamCount );

			result = (*itr)->release();
            FMOD_ErrCheck( result, "SoundManager::unregisterSoundBank(): "
					"unable to release stream: %s.fsb (instance %d)\n",
				itStream->first.c_str(), streamCount );

			++streamCount;
        }

        soundList.clear();
    }
	streamingSoundBankMap_.clear();
        // confirm that all have been unloaded
#if 0
    for( int i = 0; i < sysInfo.numWavebanks(); ++i )
    {
        if ( sysInfo.wavebankInfo(i).type == STREAM_FROM_DISK )
            MF_ASSERT( 0 == sysInfo.wavebankInfo(i).numstreams );
        break;            
    }
#endif


	if ( eventSystem_ )
	{
		// removes all references and memory
        FMOD_RESULT result;
		result = eventSystem_->release();
        FMOD_ErrCheck( result, "SoundManager::fini(): unable to release EventSystem object");
	    eventSystem_ = NULL;

		if ( listening_ )
		{
			FMOD::NetEventSystem_Shutdown();
			listening_ = false;
		}
	}
}


bool SoundManager::isInitialised()
{
	return eventSystem_ != NULL;
}


/**
 *  Call the FMOD update() function which must be called once per main game
 *  loop.
 */
bool SoundManager::update(float deltaTime)
{
	BW_GUARD;
	if (eventSystem_ == NULL)
		return false;

    //  Call System::preloadFSB from the MAIN THREAD on any loaded soundbanks 
    // waiting to be registered with FMOD.
    waitingSoundBankMutex_.grab();
    for(SoundBankWaitingList::iterator itr = waitingSoundBanks_.begin();
         itr != waitingSoundBanks_.end();)
    {
        BW::string name = itr->first;
        DataSectionPtr data = itr->second;
        registerSoundBank(name, data, true);
        itr = waitingSoundBanks_.erase(itr);
    }
    waitingSoundBankMutex_.give();

    bool ok = true;

	FMOD_RESULT result = eventSystem_->update();

	ok &= FMOD_ErrCheck( result, "SoundManager::update: \n" );

	if (listening_)
	{
		result = FMOD::NetEventSystem_Update();

		ok &= FMOD_ErrCheck( result, "SoundManager::update( net ): \n");
	}

	PROFILER_SCOPED( SoundManager_update );

#if ENABLE_JFIQ_HANDLING
    // Update the list of JFIQ events
    for( BW::list<PySound *>::iterator itr = justFailIfQuietestList_.begin();
         itr != justFailIfQuietestList_.end();
         itr++)
    {
        (*itr)->updateJFIQ();
    }
#endif

	for(PyEventReverbs::iterator eit = pyEventReverbs_.begin(); 
		eit != pyEventReverbs_.end(); eit++)
	{
		PyEventReverb * reverb = (*eit);
		MF_ASSERT( reverb != NULL );
		reverb->update3DAttributes();
	}

	return ok;
}

/**
 *  Sets the path for the sound system to use when locating sounds.  This is
 *  just an interface to FMOD::EventSystem::setMediaPath().
 */
bool SoundManager::setPath( const BW::string &path )
{
	BW_GUARD;
	if (eventSystem_ == NULL)
		return false;

	if (path.length() == 0)
	{
		ERROR_MSG( "SoundManager::setPath: "
			"Called with an empty path\n" );
		return false;
	}

	// Resolve the res-relative path into a real filesystem path.  Be aware that
	// using this mechanism means that none of this will work with zip/packed
	// filesystems.  A possible way to make this work would be to leverage
	// FMOD::EventSystem::registerMemoryFSB() which can be used to reference
	// samples from memory, but that doesn't really address the issue of large
	// stream files.
	// Add a trailing slash as per FMOD 4.11.02
	BW::string realPath = BWResolver::resolveFilename( path ) + "\\";
	INFO_MSG( "Real path is %s\n", realPath.c_str() );

	FMOD_RESULT result;
    result = eventSystem_->setMediaPath( realPath.c_str() );

	mediaPath_ = path;

	return FMOD_ErrCheck( result, "SoundManager::setPath: Couldn't set media path to '%s': %s\n" );
}


void SoundManager::allowUnload( bool b )
{
	allowUnload_ = b;
}


bool SoundManager::allowUnload() const
{
	return allowUnload_;
}


void SoundManager::registerSoundBank( const BW::string &filename, DataSectionPtr data, bool calledFromMainThread )
{
	BW_GUARD;

    int maxStreams = 0;

    // Check if the data is loaded
	if( data.exists() == false )
	{
        EventSystemInfo sysInfo(eventSystem_);
        for( int i = 0; i < sysInfo.numWavebanks(); ++i )
        {
            if (!filename.compare( sysInfo.wavebankInfo(i).name ))
            {
                if ( sysInfo.wavebankInfo(i).type == STREAM_FROM_DISK )
                {                    
                    if (calledFromMainThread)
                    {
                        maxStreams = sysInfo.wavebankInfo(i).maxstreams;
                    }
                    else
                    {
                        //Queue this sound bank to be loaded from the main thread   
                        SimpleMutexHolder lock(waitingSoundBankMutex_);
                        waitingSoundBanks_[filename] = data;
                        return;
                    }
                }  
                else
                {
                    // Start background thread task to load sound data
		            new SoundBankLoader( filename.c_str(), (mediaPath_ + "/" + filename + ".fsb").c_str() );
		            return;
                }
            }
        }
	}

    if (!calledFromMainThread) //Queue this sound bank to be loaded from the main thread
    {
        if ( soundBankMap_.find(filename) != soundBankMap_.end() )
		{
			ERROR_MSG("Soundbank already queued for registration with FMOD '%s'\n", filename.c_str());
			return;	// already queued
        }

        SimpleMutexHolder lock(waitingSoundBankMutex_);     
        waitingSoundBanks_[filename] = data;
        return;
    }

    // If we get this far we're definately in the main thread and it's safe to 
    // call preloadFSB
	if( eventSystem_ != NULL )
	{
		if ( soundBankMap_.find(filename) != soundBankMap_.end() )
		{
			ERROR_MSG("Trying to load a soundbank that is already loaded '%s'\n", filename.c_str());
			return;	// already loaded
		}

        FMOD_RESULT result;
        FMOD::System * pSystem;
        FMOD_ErrCheck( result = eventSystem_->getSystemObject(&pSystem),
            "SoundManager::registerSoundBank Couldn't get system object.");

        if (maxStreams == 0) // Not Streaming
        {
            FMOD::Sound            *pSoundData = NULL;
            BinaryPtr               pBinary    = data->asBinary();
            FMOD_CREATESOUNDEXINFO  info1      = {0};

            info1.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
            info1.length = pBinary->len();
            // TODO: make sure this const_cast is safe
            result = pSystem->createSound((const char *)(pBinary->data()),
                                FMOD_OPENMEMORY_POINT, &info1, &pSoundData );
            FMOD_ErrCheck( result, "SoundManager::registerSoundBank Error registering"
                            " soundbank %s", filename.c_str());

            result = eventSystem_->preloadFSB( (filename + ".fsb").c_str(), 0, pSoundData);
            FMOD_ErrCheck(result, "SoundManager::registerSoundBank Error registering"
                            " soundbank %s", filename.c_str());

		    if ( FMOD_ErrCheck( result, "SoundManager::registerSoundBank: Couldn't "
                            "register sound bank '%s'", filename.c_str()) )
		    {
			    INFO_MSG( "SoundManager::registerSoundBank: Sound bank '%s' registered"
                            " successfully\n",	filename.c_str() );

				SoundBank soundBank = { pBinary, pSoundData };
	            // keep reference to the sound bank
	            soundBankMap_[filename] = soundBank;
		    }            
        }
        else  //Streaming
        {            
            BW::string fileNameEx = filename + ".fsb";
	        BW::string fileNameExWithPath = BWResolver::resolveFilename( mediaPath_ ) + "\\" + fileNameEx;
            for( int streamInstance = 0; streamInstance < maxStreams; ++streamInstance)
            {
                FMOD::Sound * pStream; 

                result = pSystem->createSound( fileNameExWithPath.c_str(),
					FMOD_CREATESTREAM, 0, &pStream );
                FMOD_ErrCheck( result, "SoundManager::registerSoundBank: "
						"Couldn't open stream for %s/%s.fsb",
					mediaPath_.c_str(), filename.c_str() );

                result = eventSystem_->preloadFSB( fileNameEx.c_str(),
					streamInstance, pStream );
                FMOD_ErrCheck( result, "SoundManager::registerSoundBank: "
						"Couldn't preload stream %s.fsb (instance %d)",
					filename.c_str(), streamInstance );

				streamingSoundBankMap_[filename].push_back( pStream );
            }
        }

        // confirm that the right number have been loaded  
#if 0
        EventSystemInfo sysInfo(eventSystem_);
        for( int i = 0; i < sysInfo.numWavebanks(); ++i )
        {
	        if ( !filename.compare( sysInfo.wavebankInfo(i).name ) )
            {
                if ( sysInfo.wavebankInfo(i).type == STREAM_FROM_DISK )
                {
                    MF_ASSERT( streamInstances == sysInfo.wavebankInfo(i).numstreams );
                }   
                break;
            }
        }
#endif
	}
}

bool SoundManager::unregisterSoundBank( const BW::string &filename )
{
	BW_GUARD;

	if (!this->allowUnload())
	{
		ERROR_MSG( "Unloading sound banks is disabled\n" );
		return false;
	}

	if( eventSystem_ == NULL )
	{
		return false;
	}

	SoundBankMap::iterator it = soundBankMap_.find(filename);
    StreamingSoundBankMap::iterator itStream = streamingSoundBankMap_.find(filename);

	if( it != soundBankMap_.end() )
	{

        FMOD_RESULT result = eventSystem_->unloadFSB( (it->first + ".fsb").c_str(), 0 );
        FMOD_ErrCheck( result, "SoundManager::unregisterSoundBank(): unable to unload soundbank: %s\n" , (it->first + ".fsb").c_str() );

		// release the binary data
		it->second.pBinary_ = NULL;	

		// release FMOD::Sound
		it->second.pSoundData_->release();
		it->second.pSoundData_ = NULL;

		soundBankMap_.erase(it);

		for ( EventGroups::iterator it = eventGroups_.begin() ; it != eventGroups_.end() ; ++it )
		{
			EventGroup* eventGroup = (*it).second;
			FMOD_RESULT result = eventGroup->freeEventData( NULL, true );

			FMOD_ErrCheck(result, "SoundManager::unregisterSoundBank: Couldn't freeEventData for Group '%s'\n", ((*it).first).second.c_str() );
		}
		// clear out cached groups, they may no longer be defined, we can 
		// re-cache them later as needed.
		eventGroups_.clear();
	}
	else if( itStream != streamingSoundBankMap_.end() )
    {
        int streamCount = 0;
        SoundList& soundList = itStream->second;
        SoundList::iterator itr = soundList.begin();
        SoundList::iterator end = soundList.end();

        for ( ; itr != end; ++itr)
        {
            FMOD_RESULT result = eventSystem_->unloadFSB(
				(itStream->first + ".fsb").c_str(), streamCount );
            FMOD_ErrCheck( result, "SoundManager::unregisterSoundBank(): "
					"unable to unload soundbank: %s.fsb (instance %d)",
				itStream->first.c_str(), streamCount );

			result = (*itr)->release();
            FMOD_ErrCheck( result, "SoundManager::unregisterSoundBank(): "
					"unable to release stream: %s.fsb (instance %d)",
				itStream->first.c_str(), streamCount );

			++streamCount;
        }

        soundList.clear();
		streamingSoundBankMap_.erase(itStream);

#if 0
        // Confirm that all the instances have been unloaded
        EventSystemInfo sysInfo(eventSystem_);
        for( int i = 0; i < sysInfo.numWavebanks(); ++i )
        {
	        if ( !filename.compare( sysInfo.wavebankInfo(i).name ) )
            {
                if ( sysInfo.wavebankInfo(i).type == STREAM_FROM_DISK )
                {
                    MF_ASSERT( 0 == sysInfo.wavebankInfo(i).numstreams );
                }   
                break;
            }
        }
#endif

    }
    else // Neither streaming or non-streaming filename match
	{
		ERROR_MSG( "SoundManager::unregisterSoundBank: sound bank '%s' not in sound bank mapping.",
			filename.c_str() );
		return false;
	}

	return true;
}

/**
 *	Deprecated API!
 *	Please use loadEventProject instead. 
 */
bool SoundManager::loadSoundBank( const BW::string &project )
{
	BW_GUARD;
	WARNING_MSG( "This method has been deprecated.\n"
		"Please use loadEventProject instead.\n" );

	return this->loadEventProject( project );
}


/**
 *	Deprecated API!
 *	Please use unloadEventProject instead.
 */
bool SoundManager::unloadSoundBank( const BW::string &project )
{
	BW_GUARD;
	WARNING_MSG( "This method has been deprecated.\n"
		"Please use unloadEventProject instead.\n" );

	return this->unloadEventProject( project );
}


/**
 *  Loads an event project from an FMOD .fev project file.  Note that the string
 *	Returns a list of soundbanks that are in use by the event system.
 */
void SoundManager::getSoundBanks( BW::list< BW::string > & soundBanks )
{
	BW_GUARD;
	if (!eventSystem_) return;

	EventSystemInfo sysInfo(eventSystem_);
	for( int i = 0; i < sysInfo.numWavebanks(); ++i )
		soundBanks.push_back( sysInfo.wavebankInfo(i).name );
}


/**
 * This method returns true if a sound bank matching the name has been loaded.
 */
bool SoundManager::hasSoundBank( const BW::string & sbname ) const
{
	BW_GUARD;
	return soundBankMap_.find( sbname ) != soundBankMap_.end();
}


/**
 *  Returns a list of sound projects that are used.
 */
void SoundManager::getSoundProjects( BW::list< BW::string > & soundProjects )
{
	BW_GUARD;
	for( unsigned i = 0; i < projectFiles_.size(); ++i )
	{
		soundProjects.push_back( projectFiles_[i] );
	}
}

/**
 *	Returns a list of event groups that are use by the project.
 */
void SoundManager::getSoundGroups( const BW::string& project, BW::list< BW::string > & soundGroups )
{
	BW_GUARD;	
	if (project == "")
	{
		return;
	}

	FMOD::EventProject* pProject = NULL;
	if ((!this->parsePath( "/" + project, &pProject, NULL, NULL ))|| (pProject == NULL))
	{
		return;
	}

	int ignore = 0;
	char* name;

	int numGroups = 0;
	pProject->getNumGroups( &numGroups );
	for (int i=0; i<numGroups; i++)
	{
		FMOD::EventGroup* pEventGroup = NULL;
		pProject->getGroupByIndex( i, false, &pEventGroup);
		if (pEventGroup == NULL) continue;
		pEventGroup->getInfo( &ignore, &name);
		if (name)
			{
			BW::string groupName( name );
			soundGroups.push_back( groupName );
		}
	}
}

/**
 *	Returns a list of events that are use by the project.
 */
void SoundManager::getSoundNames( const BW::string& project, const BW::string& group, BW::list< BW::string > & soundNames )
{
	BW_GUARD;	
	if ((project == "") || (group == ""))
	{
		return;
	}

	FMOD::EventProject* pProject = NULL;
	FMOD::EventGroup* pGroup = NULL;
	if ((!this->parsePath( "/" + project + "/" + group, &pProject, &pGroup, NULL )) || (pProject == NULL) || (pGroup == NULL))
	{
		return;
	}

	int ignore = 0;
	char* name;

	int numEvents = 0;
	pGroup->getNumEvents( &numEvents );
	for (int i=0; i<numEvents; i++)
	{
		FMOD::Event *pEvent = NULL;
		pGroup->getEventByIndex( i, FMOD_EVENT_INFOONLY, &pEvent );
		if (pEvent == NULL) continue;
		pEvent->getInfo( &ignore, &name, NULL );
		if (name)
		{
			BW::string soundName( name );
			soundNames.push_back( soundName );
		}
	}
}


/**
 *  Loads a sound bank from an FMOD .fev project file.  Note that the string
 *  that is passed in should be the prefix of the filename (i.e. everything but
 *  the .fev).
 */
bool SoundManager::loadEventProject( const BW::string &project )
{
	BW_GUARD;
	// Prepend leading slash and drop extension to conform to standard syntax
	BW::string path = "/";
	path += project;

	FMOD::EventProject *pProject = NULL;
	return this->parsePath( path, &pProject, NULL, NULL );
}


/**
 *  Unloads an event project.
 */
bool SoundManager::unloadEventProject( const BW::string &project )
{
	BW_GUARD;
	if (!this->allowUnload())
	{
		PyErr_Format( PyExc_RuntimeError,
			"Unloading soundbanks is disabled" );
		return false;
	}

	// Prepend leading slash to conform to parsePath() syntax
	BW::string path = "/";
	path += project;

	FMOD::EventProject *pProject = NULL;
	if (!this->parsePath( path, &pProject, NULL, NULL, false ))
	{
		PyErr_Format( PyExc_LookupError,
			"Soundbank '%s' is not currently loaded!", project.c_str() );
		return false;
	}

    // get PyEventProject
    void * data;    
    PyEventProject * pPyProject;
    pProject->getUserData( &data );
    if ( data )
        pPyProject = static_cast< PyEventProject * >( data );
    else
        pPyProject = new PyEventProject( pProject );

#if ENABLE_JFIQ_HANDLING

    // Clear JFIQ list of events using this project
    for (PySounds::iterator eit = justFailIfQuietestList_.begin();
         eit != justFailIfQuietestList_.end();)
    {
        if ( (*eit)->getParentProject() == pPyProject )
        {
            (*eit)->unloaded( true );
	        eit = justFailIfQuietestList_.erase( eit );
        }
        else
        {
            ++eit;
        }
    }

#endif //ENABLE_JFIQ_HANDLING


	// Clear internal mappings related to this soundbank
	EventGroups::iterator git = eventGroups_.begin();
	while (git != eventGroups_.end())
	{
		if (git->first.first == pProject)
		{
			EventGroups::iterator del = git++;
            // Clean up PyEventGroups
            void * data;
            del->second->getUserData( &data );
			if (data)
			{
				PyEventGroup * pGroup = 
					static_cast< PyEventGroup * >( data );
#if 1
	            pGroup->invalidate();
#else
				pGroup->decRef();
#endif
			}
			eventGroups_.erase( del );
		}
		else
        {
			++git;
        }
	}

	EventProjects::iterator pit = eventProjects_.begin();
	while (pit != eventProjects_.end())
	{
		if (pit->second == pProject)
		{
			EventProjects::iterator del = pit++;
			eventProjects_.erase( del );
		}
		else
        {
			++pit;
        }
	}

    pPyProject->decRef();
	FMOD_RESULT result = pProject->release();

	if (result != FMOD_OK)
	{
		PyErr_Format( PyExc_RuntimeError,
			"Couldn't release soundbank %s: %s",
			project.c_str(), FMOD_ErrorString( result ) );
		return false;
	}

	if (defaultProject_ == pProject)
		defaultProject_ = NULL;

	return true;
}


/**
 *  Helper method for loadWaveData() and unloadWaveData().
 */
bool SoundManager::loadUnload( const BW::string &group, bool load )
{
	BW_GUARD;
	FMOD::EventProject *pProject = NULL;
    FMOD::EventGroup *pGroup = NULL;
	FMOD_RESULT result;

	if (!this->parsePath( group, &pProject, &pGroup, NULL ))
	{
		PyErr_Format( PyExc_RuntimeError, "SoundManager::loadUnload: "
			"parsePath() failed for %s, see debug output for more info",
			group.c_str() );
		return false;
	}

#if 1
    // Subgroups are automatically loaded or freed
	if (load)
	{
		result = pGroup->loadEventData(
			FMOD_EVENT_RESOURCE_SAMPLES, FMOD_EVENT_DEFAULT );
	}
	else
	{
		result = pGroup->freeEventData();
	}

    return FMOD_ErrCheck(result, "SoundManager::loadUnload");

#else
	// Assemble a list of the sound groups we're working with
	BW::vector< FMOD::EventGroup* > groups;
	if (pGroup)
	{
		groups.push_back( pGroup );
	}
	else
	{
		int nGroups;
		pProject->getNumGroups( &nGroups );

		for (int i=0; i < nGroups; i++)
		{
			result = pProject->getGroupByIndex( i, false, &pGroup );
			if (result != FMOD_OK)
			{
				PyErr_Format( PyExc_RuntimeError, "SoundManager::loadUnload: "
					"Couldn't get project group #%d: %s\n",
					i, FMOD_ErrorString( result ) );
				return false;
			}

			groups.push_back( pGroup );
		}
	}

	bool ok = true;

	// Iterate across groups and perform load/unload
	for (unsigned i=0; i < groups.size(); i++)
	{
		if (load)
		{
			result = groups[i]->loadEventData(
				FMOD_EVENT_RESOURCE_SAMPLES, FMOD_EVENT_DEFAULT );
		}
		else
		{
			result = groups[i]->freeEventData( NULL );
		}

		if (result != FMOD_OK)
		{
			PyErr_Format( PyExc_RuntimeError, "SoundManager::loadUnload: "
				"%sEventData() failed: %s",
				load ? "load" : "free", FMOD_ErrorString( result ) );
			ok = false;
		}
	}

	return ok;
#endif
}



/**
 *  Trigger a sound event, returning a handle to the event if successful, or
 *  NULL on failure.  For details on the semantics of event naming, please see
 *  the Python API documentation for FMOD.playSound().
 */
SoundManager::Event* SoundManager::play( const BW::string &name )
{
	BW_GUARD;
	FMOD_RESULT result;

    SoundManager::Event *pEvent = this->get( name );

	if (pEvent == NULL)
		return NULL;

	result = pEvent->start();
	if (result != FMOD_OK)
	{
		PyErr_Format( PyExc_RuntimeError, "SoundManager::play: "
			"Failed to play '%s': %s",
			name.c_str(), FMOD_ErrorString( result ) );

		return NULL;
	}

	return pEvent;
}

/**
 *  Trigger a sound event, returning a handle to the event if successful, or
 *  NULL on failure.  For details on the semantics of event naming, please see
 *  the Python API documentation for FMOD.playSound().
 */
SoundManager::Event* SoundManager::play( const BW::string &name, const Vector3 &pos )
{
	BW_GUARD;
	FMOD_RESULT result;

	SoundManager::Event *pEvent = this->get( name );

	if (pEvent == NULL)
		return NULL;

	result = pEvent->set3DAttributes( (FMOD_VECTOR*)&pos, NULL, NULL );

	if (result != FMOD_OK)
	{
		ERROR_MSG( "SoundManager::play: "
			"Failed to set 3D attributes for %s: %s\n",
			name.c_str(), FMOD_ErrorString( result ) );
	}

	result = pEvent->start();
	if (result != FMOD_OK)
	{
		PyErr_Format( PyExc_RuntimeError, "SoundManager::play: "
			"Failed to play '%s': %s",
			name.c_str(), FMOD_ErrorString( result ) );

		return NULL;
	}

	return pEvent; 
}


/**
 *  Fetch a handle to a sound event.  For details on the semantics of event
 *  naming, please see the Python API documentation for FMOD.playSound().
 */
SoundManager::Event* SoundManager::get( const BW::string &name, FMOD_EVENT_MODE mode )
{
	BW_GUARD;
    FMOD::EventProject *pProject = NULL;
    FMOD::EventGroup *pGroup = NULL;
	FMOD::Event *pEvent = NULL;

	if (this->parsePath( name, &pProject, &pGroup, &pEvent, true, mode ))
        return pEvent;
	else
		return NULL;
}


/**
 * This is a helper method to get an Event by index from an EventGroup.
 * It's here so SoundManager can track when new Event's are 'instanced'
 */
SoundManager::Event* SoundManager::get( FMOD::EventGroup * pGroup, int index )
{
	BW_GUARD;
	MF_ASSERT( pGroup );

    Event *pEvent;
	FMOD_RESULT result = pGroup->getEventByIndex( index, FMOD_EVENT_DEFAULT, &pEvent );

	if (result != FMOD_OK)
	{
		PyErr_Format( PyExc_LookupError, "SoundManager::get: "
			"Couldn't get event #i from group: %s",
			index, FMOD_ErrorString( result ) );
	}

	return pEvent;
}


/**
 *  Set the project that will be used to resolve relatively-named sound events.
 */
bool SoundManager::setDefaultProject( const BW::string &name )
{
	BW_GUARD;
	FMOD::EventProject *pProject = NULL;
	BW::string path = "/";
	path.append( name );

	if (!this->parsePath( path, &pProject, NULL, NULL ))
		return false;

	defaultProject_ = pProject;
	return true;
}


/**
 *  Sets the microphone position of the listener.
 *
 *  @param position		The listener's position
 *  @param forward		The forward vector
 *  @param up			The up vector
 *  @param deltaTime	Time since last call to this method
 */
bool SoundManager::setListenerPosition( const Vector3& position,
	const Vector3& forward, const Vector3& up, float deltaTime )
{
	BW_GUARD;
	if (eventSystem_ == NULL)
		return false;

    // if the listener position has been set before
    if( lastSet_ )
    {
        if( deltaTime > 0 )
        {
            lastVelocity_ = ( position - lastPosition_ ) / deltaTime;
        }
        else
        {
            lastVelocity_ = Vector3( 0, 0, 0 );
        }

        lastPosition_ = position;
    }
    else
    {
        lastSet_ = true;
        lastPosition_ = position;
        lastVelocity_ = Vector3( 0, 0, 0 );
    }

	float maxSpeed = SoundManager::instance().maxSoundSpeed();
	if (maxSpeed > 0 && 
		lastVelocity_.lengthSquared() > (maxSpeed*maxSpeed))
    {
        lastVelocity_ = Vector3::zero();
    }

	FMOD_RESULT result = eventSystem_->set3DListenerAttributes( 0,
		(FMOD_VECTOR*)&lastPosition_,
		(FMOD_VECTOR*)&lastVelocity_,
		(FMOD_VECTOR*)&forward,
		(FMOD_VECTOR*)&up );

    FMOD_ErrCheck(result, "SoundManager::setListenerPosition: Failed to set 3D listener attributes");

    return this->update(deltaTime);
}


/**
 *  Get the most recent position of the listener.  Pass NULL for any of the
 *  parameters you aren't interested in.
 */
void SoundManager::getListenerPosition( Vector3 *pPosition, Vector3 *pVelocity )
{
	BW_GUARD;
	if (pPosition != NULL)
	{
		*pPosition = lastPosition_;
	}

	if (pVelocity != NULL)
	{
		*pVelocity = lastVelocity_;
	}
}


/**
 *  Set the master volume.  Returns true on success.  All errors are reported as
 *  Python errors, so if you are calling this from C++ you will need to extract
 *  error messages with PyErr_PrintEx(0).
 */
bool SoundManager::setMasterVolume( float vol )
{
	BW_GUARD;
	FMOD_RESULT result;
	FMOD::EventCategory *pCategory;

	if (eventSystem_ == NULL)
	{
		PyErr_Format( PyExc_RuntimeError, "SoundManager::setMasterVolume: ",
			"No sound subsystem, can't set master volume" );
		return false;
	}

	result = eventSystem_->getCategory( "master", &pCategory );
	if (result != FMOD_OK)
	{
		PyErr_Format( PyExc_RuntimeError, "SoundManager::setMasterVolume: ",
			"Couldn't get master EventCategory: %s\n",
			FMOD_ErrorString( result ) );

		return false;
	}

	result = pCategory->setVolume( vol );
	if (result != FMOD_OK)
	{
		PyErr_Format( PyExc_RuntimeError, "SoundManager::setMasterVolume: ",
			"Couldn't set master channel group volume: %s\n",
			FMOD_ErrorString( result ) );

		return false;
	}

	return true;
}


float SoundManager::dbToLinearLevel( float db )
{
	BW_GUARD;
	if (db > 0)
	{
		WARNING_MSG( "SoundManager::dbToLinearLevel: "
			"Level > 0dB passed in (%f) - capping to 0dB\n", db );
		db = 0;
	}

	return 1.f / (db / -3.f);
}


/**
 *  This is the catch-all method for parsing soundbank paths.  The general
 *  semantics are similar to those for filesystem paths, which gives two general
 *  forms of event name:
 *
 *  absolute: /project/group1/group2/event
 *  relative: group1/group2/event
 *
 *  The default project is used to look up relative paths.
 *
 *  The caller must pass in FMOD pointer pointers which the return values are
 *  written to, or NULL if the caller isn't interested in a particular pointer.
 *
 *  You cannot pass a non-NULL pointer after you have passed a NULL one,
 *  i.e. you can't pass NULL for ppProject and then pass a non-NULL pointer for
 *  ppGroup.
 *
 *  If you pass NULL for ppEvent, then the entire path is considered to be the
 *  name of the event group, rather than the usual 'path/to/group/eventname'
 *  semantics.
 */
bool SoundManager::parsePath( const BW::string &path,
	FMOD::EventProject **ppProject, FMOD::EventGroup **ppGroup,
    FMOD::Event **ppEvent, bool allowLoadProject, FMOD_EVENT_MODE mode )
{
	BW_GUARD;
	FMOD_RESULT result;
	ssize_t groupStart = 0;
	BW::string groupName, eventName;

	if (eventSystem_ == NULL)
	{
		return false;
	}

	// Sanity check for the path
	if (path.size() == 0)
	{
		ERROR_MSG( "SoundManager::parsePath: Invalid path '%s'\n",
			path.c_str() );

		return false;
	}

	// If the project isn't wanted, bail now
	if (!ppProject)
		return true;

	// If the leading character is a '/', then the project has been manually
	// specified
	if (path[0] == '/')
	{
		BW::string::size_type firstSlash = path.find( '/', 1 );
		bool gotFirstSlash = (firstSlash != BW::string::npos);

		groupStart = gotFirstSlash ? firstSlash + 1 : path.size();

		BW::string projectName( path, 1,
			gotFirstSlash ? firstSlash - 1 : path.size() - 1 );

		EventProjects::iterator it = eventProjects_.find( projectName );

		// If the project isn't loaded, do it now
		if (it == eventProjects_.end())
		{
			if (!allowLoadProject)
				return false;

			DataSectionPtr data = BWResource::openSection(mediaPath_ + "/" + projectName + ".fev");

			if( data.exists() == false )
			{
				ERROR_MSG( "SoundManager::parsePath: "
					"Failed to load '%s'\n", (mediaPath_ + "/" + projectName + ".fev").c_str() );
				return false;
			}

			BinaryPtr pBinary = data->asBinary();

			FMOD_EVENT_LOADINFO loadInfo;
			ZeroMemory( &loadInfo, sizeof(loadInfo) );
			loadInfo.size = sizeof(loadInfo);
			loadInfo.loadfrommemory_length = pBinary->len();

#if 0 // not going to load from disk
			// todo load from zip and pass data
			result = eventSystem_->load(
				(projectName + ".fev").c_str(), NULL, ppProject );
#else // load from memory
			result = eventSystem_->load(
				(const char*)pBinary->data(), &loadInfo, ppProject );
#endif	

			if (result == FMOD_OK)
			{
				eventProjects_[ projectName ] = *ppProject;

				// Set the default project if there isn't one already
				if (defaultProject_ == NULL)
					defaultProject_ = *ppProject;
			}
			else
			{
				ERROR_MSG( "SoundManager::parsePath: "
					"Failed to load project %s: %s\n",
					projectName.c_str(), FMOD_ErrorString( result ) );

				return false;
			}
		}

		// Otherwise just pass back handle to already loaded project
		else
		{
			*ppProject = it->second;
		}
	}

	// If no leading slash, then we're talking about the default project
	else
	{
		groupStart = 0;

		if (defaultProject_ == NULL)
		{
			ERROR_MSG( "SoundManager::parsePath: "
				"No project specified and no default project loaded: %s\n",
				path.c_str() );

			return false;
		}
		else
		{
			*ppProject = defaultProject_;
		}
	}

	// If the group isn't wanted, bail now
	if (!ppGroup)
		return true;

	// If ppEvent isn't provided, then the group name is the rest of the path
	if (!ppEvent)
	{
		groupName = path.substr( groupStart );
	}

	// Otherwise, we gotta split on the final slash
	else
	{
		ssize_t lastSlash = path.rfind( '/' );

		if (lastSlash == BW::string::npos || lastSlash < groupStart)
		{
			ERROR_MSG( "SoundManager::parsePath: "
				"Asked for illegal top-level event '%s'\n", path.c_str() );
			return false;
		}

		ssize_t eventStart = lastSlash + 1;
		groupName = path.substr( groupStart, lastSlash - groupStart );
		eventName = path.substr( eventStart );
	}

	// If the group name is empty, set ppGroup to NULL and we're done.
	if (groupName.empty())
	{
		*ppGroup = NULL;
		return true;
	}

	// If the event group hasn't been loaded yet, do it now.
	Group g( *ppProject, groupName );
	EventGroups::iterator it = eventGroups_.find( g );

	if (it != eventGroups_.end())
	{
		*ppGroup = it->second;
	}
	else
	{
		// We pass 'cacheevents' as false here because there is no Python API
		// exposure for FMOD::Group and precaching is all handled by
		// BigWorld.loadSoundGroup().
		result = (*ppProject)->getGroup(
			groupName.c_str(), false, ppGroup );

		if (result == FMOD_OK)
		{
			eventGroups_[ g ] = *ppGroup;
		}
		else
		{
			ERROR_MSG( "SoundManager::parsePath: "
				"Couldn't get event group '%s': %s\n",
				groupName.c_str(), FMOD_ErrorString( result ) );

			return false;
		}
	}

	// If the event isn't wanted, bail now
	if (!ppEvent)
		return true;

	// Get event handle
	result = (*ppGroup)->getEvent( eventName.c_str(), mode, ppEvent );

    if (result != FMOD_OK)
	{
		ERROR_MSG( "SoundManager::parsePath: "
			"Couldn't get event %s from group %s: %s\n",
			eventName.c_str(), groupName.c_str(), FMOD_ErrorString( result ) );

		return false;
	}

	return true;
}

/**
 *  Converts the provided sound path into an absolute path.
 */
bool SoundManager::absPath( const BW::string &path, BW::string &ret )
{
	BW_GUARD;
	// If the path is already absolute just copy it
	if (path.size() && path[0] == '/')
	{
		ret = path;
		return true;
	}

	// Otherwise, prepend the default project
	else if (defaultProject_)
	{
		char *pname;
		defaultProject_->getInfo( NULL, &pname );
		ret = "/";
		ret += pname;
		ret.push_back( '/' );
		ret += path;
		return true;
	}
	else
	{
		PyErr_Format( PyExc_RuntimeError,
			"Can't resolve absolute path with no default project" );
		return false;
	}
}


/**
 *  Precache the wavedata for a particular event group (and all groups and
 *  events below it).  See the documentation for
 *  FMOD::EventGroup::loadEventData() for more information.
 */
bool SoundManager::loadWaveData( const BW::string &group )
{
	BW_GUARD;
	return this->loadUnload( group, true );
}


/**
 *  Unload the wavedata and free the Event* handles for an event group.  See the
 *  documentation for FMOD::EventGroup::freeEventData() for more info.
 */
bool SoundManager::unloadWaveData( const BW::string &group )
{
	BW_GUARD;
	return this->loadUnload( group, false );
}

/**
 *  Return this if you are supposed to return an Event* from a function that is
 *  exposed to script and something goes wrong.
 */
PyObject* SoundManager::pyError()
{
	BW_GUARD;
	switch (instance().errorLevel())
	{
		case ERROR_LEVEL_EXCEPTION:
			return NULL;

		case ERROR_LEVEL_WARNING:
			PyErr_PrintEx(0);
			Py_RETURN_NONE;

		case ERROR_LEVEL_SILENT:
		default:
			PyErr_Clear();
			Py_RETURN_NONE;
	}
}

static PyObject* py_getSoundBanks( PyObject *args )
{
	BW_GUARD;
	BW::list< BW::string > soundbanks;

	SoundManager::instance().getSoundBanks( soundbanks );

	PyObject* result = PyList_New(0);

	MF_ASSERT( result != NULL)

	for( BW::list< BW::string >::iterator it = soundbanks.begin(); it != soundbanks.end(); ++it )
	{
		PyObject* str = PyString_FromString((*it).c_str());
		PyList_Append( result, str );
		Py_XDECREF(str);
	}

	return result;
}

/*~ function FMOD.getSoundBanks
 *
 *  Returns a list of sound banks that are referenced by FMod.
 *
 *  @return	list of sound bank names
 */
PY_MODULE_FUNCTION( getSoundBanks, _FMOD );

static PyObject* py_loadSoundBankIntoMemory( PyObject *args )
{
	BW_GUARD;
	const char *soundbank;

	if( PyArg_ParseTuple( args, "s", &soundbank ) == false )
		return NULL;

	SoundManager::instance().registerSoundBank( soundbank, NULL );

	Py_RETURN_NONE;
}

/*~ function FMOD.loadSoundBankIntoMemory
 *
 *  Loads a sound bank into memory.
 *
 *  @param	soundbank	name of soundbank
 */
PY_MODULE_FUNCTION( loadSoundBankIntoMemory, _FMOD );

static PyObject* py_unloadSoundBankFromMemory( PyObject *args )
{
	BW_GUARD;
	const char *soundbank;

	if( PyArg_ParseTuple( args, "s", &soundbank ) == false )
		return NULL;

	if ( !SoundManager::instance().unregisterSoundBank( soundbank ) )
	{
		PyErr_Format( PyExc_RuntimeError, 
			"Error unregistering soundbank '%s'", soundbank );
		return NULL;
	}

	Py_RETURN_NONE;
}

/*~ function FMOD.unloadSoundBankFromMemory
 *
 *  Unloads a sound bank from memory.
 *
 *  @param	soundbank	name of soundbank
 */
PY_MODULE_FUNCTION( unloadSoundBankFromMemory, _FMOD );

BW_END_NAMESPACE

#endif // FMOD_SUPPORT

// soundmanager.cpp
