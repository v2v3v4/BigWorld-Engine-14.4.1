#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include "fmod_config.hpp"

#if FMOD_SUPPORT
#include "cstdmf/singleton.hpp"
#include "cstdmf/concurrency.hpp"
#include "pyscript/script.hpp"
#include "math/vector3.hpp"
#include "resmgr/datasection.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_list.hpp"

#include <fmod_event.hpp>

BW_BEGIN_NAMESPACE

class PySound;
class PyMusicSystem;
class PyEventGroup;
class PyEventProject;
class PyEventCategory;
class PyEventReverb;

/**
 *	This is the main entry point for the FMOD sound integration. Note
 *	that the sound library has been primarily designed as a Python interface,
 *	so care must be taken when calling function from C++ that are usually
 *	called directly from Python. They can set a Python exception on error
 *	which may result in leaving the Python VM in an exception state (which
 *	should be cleared with PyErr_Print or PyErr_Clear).
 */
class SoundManager : public Singleton< SoundManager >
{
public:
	enum ErrorLevel
	{
		ERROR_LEVEL_SILENT,
		ERROR_LEVEL_WARNING,
		ERROR_LEVEL_EXCEPTION
	};

	typedef FMOD::Event          Event;
	typedef FMOD::EventGroup     EventGroup;
	typedef FMOD::EventParameter EventParameter;
    typedef FMOD::EventCategory  EventCategory;
    typedef FMOD::EventProject   EventProject;
    typedef FMOD::EventReverb    EventReverb;
    typedef FMOD::MusicSystem    MusicSystem;
	typedef FMOD_EVENT_STATE	 EventState;

public:
	SoundManager();
	~SoundManager();

    bool initialise( DataSectionPtr config = NULL );
	void fini();
	bool update( float deltaTime );
	bool isInitialised();
	bool setPath( const BW::string &path );

	// Controls whether projects can be unloaded
	void allowUnload( bool b );
	bool allowUnload() const;

	// Occlusion settings
	void terrainOcclusionEnabled( bool b ) { terrainOcclusionEnabled_ = b; }
	bool terrainOcclusionEnabled() const { return terrainOcclusionEnabled_; }
	void modelOcclusionEnabled( bool b ) { modelOcclusionEnabled_ = b; }
	bool modelOcclusionEnabled() const { return modelOcclusionEnabled_; }

	// Velocity checking
	float maxSoundSpeed() const { return maxSoundSpeed_; }
	void maxSoundSpeed( float f ) { maxSoundSpeed_ = f; }

	// if data is empty, register will queue the load in a background thread and then call back with the data
	void registerSoundBank( const BW::string &filename, DataSectionPtr data, bool calledFromMainThread = false );
	bool unregisterSoundBank( const BW::string &filename );

	void getSoundBanks( BW::list< BW::string > & soundBanks );
	bool hasSoundBank( const BW::string & sbname ) const;

	void getSoundProjects( BW::list< BW::string > & soundProjects );
	void getSoundGroups( const BW::string& project, BW::list< BW::string > & soundGroups );
	void getSoundNames( const BW::string& project, const BW::string& group, BW::list< BW::string > & soundNames );

	// Deprecated api
	bool loadSoundBank( const BW::string &project );
	bool unloadSoundBank( const BW::string &project );

	// Loading and unloading of event files and resources
	bool loadEventProject( const BW::string &project );
	bool unloadEventProject( const BW::string &project );

	bool loadWaveData( const BW::string &group );
	bool unloadWaveData( const BW::string &group );

	// Methods for sound related python objects. 
	// These always return a new reference.
    PyMusicSystem*	pyMusicSystem();
    PyObject*		createPySound( const BW::string & path );
    PyObject*		pyEventGroup( const BW::string & groupPath );
    PyObject*		pyEventProject( const BW::string & projectName );
    PyObject*		pyEventCategory( const BW::string & categoryPath );
    PyObject*		pyEventReverb( const BW::string & name );

	Event* play( const BW::string &name );
	Event* play( const BW::string &name, const Vector3 &pos );
    Event* get( const BW::string &name, FMOD_EVENT_MODE mode = FMOD_EVENT_DEFAULT);
	Event* get( EventGroup * pGroup, int index );

	bool setDefaultProject( const BW::string &name );

	bool setListenerPosition( const Vector3& position, const Vector3& forward,
		const Vector3& up, float deltaTime );

	void getListenerPosition( Vector3 *pPosition, Vector3 *pVelocity = NULL );

	bool setMasterVolume( float vol );


	bool absPath( const BW::string &path, BW::string &ret );

	// Access to the internal event system object.
	FMOD::EventSystem* getEventSystemObject();

public:

	static float dbToLinearLevel( float db );

	static void errorLevel( ErrorLevel level ) { s_errorLevel_ = level; }
	static ErrorLevel errorLevel() { return s_errorLevel_; }
	static void logError( const char* msg, ... );

	static bool FMOD_ErrCheck( FMOD_RESULT result, const char * format, ... );
	static bool FMOD_PyErrCheck( FMOD_RESULT result, const char * format, ... );
	static PyObject* pyError();

#if ENABLE_JFIQ_HANDLING
private:
    typedef BW::list<PySound *> PySounds;
    PySounds justFailIfQuietestList_;
public:
    void addToJFIQList( PySound *pySound );
    void removeFromJFIQList( PySound *pySound );

#endif //ENABLE_JFIQ_HANDLING

private:
	bool loadUnload( const BW::string &group, bool load );

	bool parsePath( const BW::string &path, FMOD::EventProject **ppProject,
        FMOD::EventGroup **ppGroup, FMOD::Event **ppEvent,
		bool allowLoadProject = true, FMOD_EVENT_MODE mode = FMOD_EVENT_DEFAULT);

    void kill();

private:
	// Controls what happen when we hit an error
	static ErrorLevel s_errorLevel_;

    SimpleMutex	waitingSoundBankMutex_;
	typedef BW::map< BW::string, DataSectionPtr > SoundBankWaitingList;
    SoundBankWaitingList waitingSoundBanks_;

	struct SoundBank
	{
		BinaryPtr pBinary_;
		FMOD::Sound *pSoundData_;
	};
	typedef BW::map< BW::string, SoundBank > SoundBankMap;
	SoundBankMap soundBankMap_;
    typedef BW::list< FMOD::Sound* > SoundList;
    typedef BW::map< BW::string, SoundList > StreamingSoundBankMap;
    StreamingSoundBankMap streamingSoundBankMap_;

	BW::string mediaPath_;

	// Position listener was last set at
    Vector3 lastPosition_;

	// Velocity the listener was last going at (in units per second)
    Vector3 lastVelocity_;

	// Has the last position been set?
    bool lastSet_;

	FMOD::EventSystem* eventSystem_;
    // There can only be one loaded project with music data
    PyMusicSystem* musicSystem_;

	// The FMOD Project used for resolving relatively-named sounds
	FMOD::EventProject* defaultProject_;

	// Mapping used for caching FMOD::EventGroups
	typedef std::pair< FMOD::EventProject*, BW::string > Group;
	typedef BW::map< Group, EventGroup* > EventGroups;
	EventGroups eventGroups_;

    // List to keep track of all the PyEventCategories being created 
    // so we can clean them up later.
    typedef BW::list< PyEventCategory* > PyEventCategories;
    PyEventCategories pyEventCategories_;

    // List to keep track of all the PyEventReverbs being created 
    // so we can clean them up later.
    typedef BW::list< PyEventReverb* > PyEventReverbs;
    PyEventReverbs pyEventReverbs_;

	// Mapping of loaded FMOD::EventProjects
	typedef BW::map< BW::string, FMOD::EventProject* > EventProjects;
	EventProjects eventProjects_;


	// Indicates whether we have an FMOD net connection open
	bool listening_;

	// Can soundbanks be unloaded at runtime?
	bool allowUnload_;

	// Occlusion
	bool terrainOcclusionEnabled_;
	bool modelOcclusionEnabled_;

	// Velocity checking
	float maxSoundSpeed_;

	BW::vector< BW::string > projectFiles_;
};

BW_END_NAMESPACE

#endif // FMOD_SUPPORT

#endif // SOUND_MANAGER_HPP
