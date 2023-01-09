#include "pch.hpp"

#define TIMESTAMP_UNRELIABLE	// for broken laptops

#include "action_matcher.hpp"	// for the 'debug_' static
#include "adaptive_lod_controller.hpp"
#include "app.hpp"
#include "app_config.hpp"
#include "bw_winmain.hpp"
#include "camera_app.hpp"
#include "canvas_app.hpp"
#include "client_camera.hpp"
#include "connection_control.hpp"
#include "debug_app.hpp"
#include "device_app.hpp"
#include "entity.hpp"
#include "entity_flare_collider.hpp"
#include "entity_manager.hpp"
#include "entity_picker.hpp"
#include "entity_type.hpp"
#include "filter.hpp"
#include "finale_app.hpp"
#include "message_time_prefix.hpp"
#include "physics.hpp"
#include "py_server.hpp"
#include "python_server.hpp"
#include "resource.h"
#include "script_bigworld.hpp"
#include "script_player.hpp"
#include "version_info.hpp"
#include "web_app.hpp"
#include "world_app.hpp"

#include "ashes/alpha_gui_shader.hpp"
#include "ashes/clip_gui_shader.hpp"
#include "ashes/frame_gui_component.hpp"
#include "ashes/gui_progress.hpp"
#include "ashes/simple_gui.hpp"
#include "ashes/text_gui_component.hpp"

#include "camera/annal.hpp"
#include "camera/camera_control.hpp"
#include "camera/direction_cursor.hpp"
#include "camera/free_camera.hpp"
#include "camera/projection_access.hpp"

#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_space.hpp"
#if UMBRA_ENABLE
#include "chunk/chunk_umbra.hpp"
#endif

#include "space/deprecated_space_helpers.hpp"
#include "space/client_space.hpp"
#include "space/space_manager.hpp"

#include "cstdmf/base64.h"
#include "cstdmf/config.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/dogwatch.hpp"
#include "cstdmf/log_msg.hpp"
#include "cstdmf/main_loop_task.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/profile.hpp"
#include "cstdmf/profiler.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/timestamp.hpp"
#include "cstdmf/watcher.hpp"

#include "connection/avatar_filter_helper.hpp"
#include "connection/replay_controller.hpp"
#include "connection/smart_server_connection.hpp"

#include "connection_model/bw_null_connection.hpp"
#include "connection_model/bw_replay_connection.hpp"
#include "connection_model/bw_server_connection.hpp"

#include "duplo/foot_trigger.hpp"
#include "duplo/py_loft.hpp"
#include "duplo/py_splodge.hpp"
#include "duplo/pymodel.hpp"

#include "input/py_input.hpp"
#include "input/input_cursor.hpp"
#include "input/ime.hpp"

#include "math/blend_transform.hpp"
#include "math/colour.hpp"

#include "model/super_model.hpp"

#include "moo/animation_manager.hpp"
#include "moo/animating_texture.hpp"
#include "moo/draw_context.hpp"
#include "moo/effect_material.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/interpolated_animation_channel.hpp"
#include "moo/texture_manager.hpp"
#include "moo/vertex_declaration.hpp"
#include "moo/visual_manager.hpp"
#include "moo/render_context.hpp"

#include "particle/particle_system_manager.hpp"

#ifndef ENABLE_WATCHERS
#include <Winsock2.h>	// for gethostname
#endif

#include "pyscript/personality.hpp"
#include "pyscript/py_callback.hpp"
#include "pyscript/py_data_section.hpp"
#include "pyscript/py_debug_message_file_logger.hpp"

#include "resmgr/access_monitor.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/data_section_census.hpp"
#include "resmgr/win_file_system.hpp"

#if ENABLE_CONSOLES
#include "romp/console.hpp"
#include "romp/console_manager.hpp"
#endif // ENABLE_CONSOLES
#include "romp/fog_controller.hpp"
#include "romp/engine_statistics.hpp"
#include "romp/enviro_minder.hpp"
#include "romp/frame_logger.hpp"
#include "moo/geometrics.hpp"
#include "romp/histogram_provider.hpp"
#include "romp/flora.hpp"
#include "romp/lens_effect_manager.hpp"
#include "romp/sea.hpp"
#include "romp/sky_gradient_dome.hpp"
#include "romp/sky_light_map.hpp"
#include "romp/sun_and_moon.hpp"
#include "romp/star_dome.hpp"
#include "romp/terrain_occluder.hpp"
#include "moo/texture_renderer.hpp"
#include "romp/time_of_day.hpp"
#include "romp/water.hpp"
#include "romp/weather.hpp"
#include "romp/z_buffer_occluder.hpp"

#include "waypoint/waypoint_stats.hpp"

#include "zip/zlib.h"

#if ENABLE_ASSET_PIPE
#include "asset_pipeline/asset_client.hpp"
#endif

#include <algorithm>

#ifndef TIMESTAMP_UNRELIABLE
#define frameTimerSetup() stampsPerSecond()
#define frameTimerValue() timestamp()
#define frameTimerFreq() stampsPerSecondD()
#define FRAME_TIMER_TYPE uint64
#else
#include <mmsystem.h>
#define frameTimerSetup() timeBeginPeriod(1)
#define frameTimerValue() timeGetTime()
#define frameTimerFreq() 1000.0
#define FRAME_TIMER_TYPE DWORD
#endif

#define NONZERO_PAUSE_TIME_DIFFERENCE 0.00001f
DECLARE_DEBUG_COMPONENT2( "App", 0 );

extern const wchar_t * APP_TITLE;


BW_BEGIN_NAMESPACE

class BackBufferEffect;

extern void setupTextureFeedPropertyProcessors();

#ifndef CODE_INLINE
#include "app.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: Config string
// -----------------------------------------------------------------------------

#if   defined( _DEBUG )
#define CONFIG_STRING "DEBUG"
#elif defined( _INSTRUMENTED )
#define CONFIG_STRING "INSTRUMENTED"
#elif defined( _HYBRID )
  #if defined( _EVALUATION )
    #define CONFIG_STRING "EVALUATION"
  #else
    #define CONFIG_STRING "HYBRID"
  #endif
#elif defined(_RELEASE)
#define CONFIG_STRING "RELEASE"
#else
#define CONFIG_STRING "UNKNOWN"
#endif

const char * configString		= CONFIG_STRING;

// -----------------------------------------------------------------------------
// Section: Statics and globals
// -----------------------------------------------------------------------------

App * App::pInstance_ = NULL;

static const float FAR_DISTANCE  = 10000.0f;
//there are 4 stages to progress;
//app startup, shader compilation, preloads + personality script init.
//in C++ the total progress goes to 100%, but the gui progress bar script
//rescales this value for display if it needs some leeway at the end of the
//progress bar for personality script initialisation.
const float PROGRESS_TOTAL = 3.f;
const float APP_PROGRESS_STEP = 1.f / 10.f;	//app startup has 10 steps

static DogWatch g_splodgeWatch( "Splodge" );

static DogWatch g_floraWatch( "Flora" );


bool g_drawWireframe = false;

/// Adjusting this allows run-time adjustment of the discrete level of detail.
float CLODPower = 10.f;

// reference the ChunkInhabitants that we want to be able to load.
extern int ChunkAttachment_token;
extern int ChunkModel_token;
extern int ChunkLight_token;
extern int ChunkTerrain_token;
extern int ChunkModelVLO_token;
extern int ChunkFlare_token;
extern int ChunkWater_token;
extern int ChunkEntity_token;
extern int ChunkParticles_token;
extern int ChunkTree_token;
extern int ChunkStationNode_token;
extern int ChunkUserDataObject_token;
extern int ChunkWaypointSet_token;
extern int ChunkModelVLORef_token;
extern int ChunkDeferredDecal_token;

static int s_chunkTokenSet = ChunkAttachment_token | ChunkModel_token | ChunkLight_token |
	ChunkTerrain_token | ChunkFlare_token | ChunkWater_token |
	ChunkEntity_token | ChunkParticles_token | ChunkTree_token | 
	ChunkStationNode_token | ChunkUserDataObject_token | ChunkWaypointSet_token |
	ChunkModelVLO_token | ChunkModelVLORef_token | ChunkDeferredDecal_token;

extern int PyMetaParticleSystem_token;
extern int PyParticleSystem_token;
static int PS_tokenSet = PyMetaParticleSystem_token | PyParticleSystem_token;

extern int IKConstraintSystem_token;
extern int Tracker_token;
static int fashionTokenSet =
	Tracker_token | IKConstraintSystem_token;

extern int FootTrigger_token;
extern int PySplodge_token;
extern int PyLoft_token;
static int attachmentTokenSet = FootTrigger_token | PySplodge_token | PyLoft_token;

extern int PyModelObstacle_token;
extern int ChunkDynamicObstacle_token;
static int embodimentTokenSet = PyModelObstacle_token | ChunkDynamicObstacle_token;

extern int PyAvatarFilter_token;
extern int PyAvatarDropFilter_token;
extern int PyBoidsFilter_token;
extern int PyDumbFilter_token;
extern int PyPlayerAvatarFilter_token;
static int filterTokenSet =	PyAvatarFilter_token |
							PyAvatarDropFilter_token |
							PyBoidsFilter_token |
							PyDumbFilter_token |
							PyPlayerAvatarFilter_token;

extern int Decal_token;
extern int PyModelRenderer_token;
extern int PySceneRenderer_token;
extern int PyResourceRefs_token;
extern int TextureFeeds_token;
extern int Oscillator_token;
extern int Servo_token;
extern int PyEntities_token;
extern int PyChunk_token;
extern int Homer_token;
extern int Bouncer_token;
extern int Propellor_token;
extern int LinearHomer_token;
extern int Orbitor_token;
extern int BoxAttachment_token;
extern int SkeletonCollider_token;
extern int PyGraphicsSetting_token;
extern int PyPhysics2_token;
extern int PyVOIP_token;
extern int ServerDiscovery_token;
extern int Pot_token;
extern int PyMaterial_token;
extern int PyRenderTarget_token;
extern int PyOmniLight_token;
extern int PySpotLight_token;
extern int PyChunkLight_token;
extern int PyChunkSpotLight_token;

static int miscTokenSet = PyModelRenderer_token | PySceneRenderer_token |
	PyEntities_token | PyChunk_token | Oscillator_token | Homer_token | Bouncer_token |
	Propellor_token | ServerDiscovery_token | Pot_token | TextureFeeds_token |
	Servo_token | LinearHomer_token | Orbitor_token | BoxAttachment_token |
	SkeletonCollider_token | Decal_token | PyPhysics2_token |
	PyVOIP_token | PyResourceRefs_token |
	PyMaterial_token | PyRenderTarget_token | 
	PyGraphicsSetting_token | PyOmniLight_token | PySpotLight_token |
	PyChunkLight_token | PyChunkSpotLight_token;

namespace PostProcessing
{
	extern int tokenSet;
	static int ppTokenSet = tokenSet;
}

extern int LatencyGUIComponent_token;
extern int Minimap_token;
static int guiTokenSet = LatencyGUIComponent_token | Minimap_token;

extern int EntityDirProvider_token;
extern int DiffDirProvider_token;
extern int ScanDirProvider_token;
extern int InvViewMatrixProvider_token;
static int dirProvTokenSet = EntityDirProvider_token | DiffDirProvider_token |
	ScanDirProvider_token | InvViewMatrixProvider_token;


extern int CanvasApp_token;
extern int DebugApp_token;
extern int DeviceApp_token;
extern int FacadeApp_token;
extern int FinalApp_token;
extern int GUIApp_token;
extern int LensApp_token;
extern int ProfilerApp_token;
extern int ScriptApp_token;
extern int VOIPApp_token;
extern int WebApp_token;
extern int WorldApp_token;
static int mainLoopTaskTokenSet =	CanvasApp_token |
									DebugApp_token |
									DeviceApp_token |
									FacadeApp_token |
									FinalApp_token |
									GUIApp_token |
									LensApp_token |
									ProfilerApp_token |
									ScriptApp_token |
									VOIPApp_token |
									WebApp_token | 
									WorldApp_token;


bool gWorldDrawEnabled = true;
const char * const gWorldDrawLoopTasks[] = {	"Canvas",
												"World",
												"Flora",
												"Facade",
												"Lens"		};

AutoConfigString s_engineConfigXML("system/engineConfigXML");
AutoConfigString s_scriptsConfigXML("system/scriptsConfigXML");
AutoConfigString loadingScreenName("system/loadingScreen");
AutoConfigString loadingScreenGUI("system/loadingScreenGUI");
AutoConfigString s_graphicsSettingsXML("system/graphicsSettingsXML");
AutoConfigString s_floraXML("environment/floraXML");
AutoConfigString s_blackTexture("system/blackBmp");
int  s_framesCounter              = -1;
bool s_usingDeprecatedBigWorldXML = false;
//static bool displayLoadingScreen();
//static void freeLoadingScreen();
//static void loadingText( const BW::string & s );
DataSectionPtr s_scriptsPreferences = NULL;
BW::string s_configFileName( "" );

namespace 
{
/**
 *	This function returns the total game time elapsed, used by callbacks from
 *	lower level modules, so they do not create circular dependencies back to
 *	the bwclient lib.
 */
double getGameTotalTime()
{
	BW_GUARD;
	return App::instance().getGameTimeFrameStart();
}


/**
 *	This function determines whether or not the desktop has been
 *	locked by the user.
 */
bool desktopLocked()
{
	HDESK hDesk = OpenInputDesktop( 0, 0, 0 );
	if (hDesk)
	{
		CloseDesktop( hDesk );
	}
	return (hDesk == NULL);
}


}

// -----------------------------------------------------------------------------
// Section: Initialisation and Termination
// -----------------------------------------------------------------------------

/**
 *	The constructor for the App class. App::init needs to be called to fully
 *	initialise the application.
 *
 *  @param configFilename		top level configuration file, eg "bigworld.xml".
 *  @param compileTime			build time stamp to display, if required.
 *
 *	@see App::init
 */
App::App( const BW::string &	configFilename,
		  const char *			compileTime	) :
	hWnd_( NULL ),
	appStartRenderTime_( 0 ),
	frameStartRenderTime_( 0 ),
	dRenderTime_( 0.f ),
	dGameTime_( 0.f ),
	accumulatedTimeSkip_( 0.0 ),
	lastFrameEndTime_( 0 ),
	minFrameTime_( 0 ),
	minimumFrameRate_( 8.f ),
#if !ENABLE_CONSOLES
	debugKeyEnable_( false ),
#else
	debugKeyEnable_( true ),
#endif
	activeCursor_( NULL ),
	handleKeyEventDepth_( -1 ),
	sleepTime_( 1 ),
	currentState_( STATE_UNINITIALISED ),
	inputDevices_(),
	quiting_( false ),
	pAssetClient_( NULL )
{
	BW_GUARD;

	CStdMf::checkUnattended();

	// If specified, copy in compile time string.
	if ( compileTime )
		compileTime_ = compileTime;

	frameTimerSetup();
	appStartRenderTime_ = frameTimerValue();
	frameStartRenderTime_ = appStartRenderTime_;

#if ENABLE_DPRINTF
	pMessageTimePrefix_ = new MessageTimePrefix;
#endif // ENABLE_DPRINTF

	// Set callback for PyScript so it can know total game time
	Script::setTotalGameTimeFn( getGameTotalTime );

	// Make sure that this is the only instance of the app.
	MF_ASSERT_DEV( pInstance_ == NULL );
	pInstance_ = this;

	// Clear key routing tables
	ZeroMemory( &keyRouting_, sizeof( keyRouting_ ) );

	// Run things that configure themselves from a config file
	if (!AutoConfig::configureAllFrom( AutoConfig::s_resourcesXML ))
	{
		criticalInitError(
			"Could not find resources.xml, which should "
			"contain the location of system resources!" );

		throw InitError( "Could not load resources XML" );
	}

	// Load engine_config.xml
	BW::string filename = s_engineConfigXML.value();
	if(!configFilename.empty())
	{
		INFO_MSG("Loading engine configuration file '%s' from command line.\n",
			configFilename.c_str());
		filename = configFilename;
	}

	DataSectionPtr configRoot =
		BWResource::instance().openSection(filename);

	if (AppConfig::instance().init(configRoot))
	{
		s_usingDeprecatedBigWorldXML = false;
		s_configFileName = s_engineConfigXML.value();
	}
	else
	{
		criticalInitError( "Could not load config file: %s!",
					filename.c_str() );

		throw InitError( "Could not load config file" );
	}

	DataSectionPtr configSection = AppConfig::instance().pRoot();

#if ENABLE_PROFILER
	int profilerMemorySize = 
		configSection->readInt( "debug/profilerBufferSize", 12*1024*1024 );
	g_profiler.init( profilerMemorySize );
#endif

	XMLSection::shouldReadXMLAttributes( configRoot->readBool( 
		"shouldReadXMLAttributes", XMLSection::shouldReadXMLAttributes() ));
	XMLSection::shouldWriteXMLAttributes( configRoot->readBool( 
		"shouldWriteXMLAttributes", XMLSection::shouldWriteXMLAttributes() ));

	// Initialise DebugFilter category suppression
	DataSectionPtr pCategorySuppressionSection = configSection->openSection(
		"logging/suppress/categories" );
	if (pCategorySuppressionSection)
	{
		typedef BW::vector< BW::string > stringVec;

		stringVec categories;

		pCategorySuppressionSection->readStrings( "category", categories,
			DS_TrimWhitespace );

		for (stringVec::const_iterator iCategory = categories.begin();
			iCategory != categories.end(); ++iCategory)
		{
			DebugFilter::instance().setCategoryFilter( *iCategory,
				NUM_MESSAGE_PRIORITY );
		}
	}
	
	// Read in the debug keys
	DataSectionPtr debugKeysSection = configSection->openSection( "debugKeys" );
	if ( debugKeysSection )
	{
		BW::vector< DataSectionPtr> combinations;
		debugKeysSection->openSections( "combination", combinations );

		for (size_t i = 0; i < combinations.size(); i++)
		{
			KeyCode::KeyArray debugKeyArray;

			BW::vector< DataSectionPtr> keys;
			combinations[i]->openSections( "key", keys );

			for(size_t k = 0; k < keys.size(); k++)
			{
				BW::string keyName = keys[k]->asString();
				KeyCode::Key key = KeyCode::stringToKey( keyName );
				if (key != KeyCode::KEY_NOT_FOUND)
				{
					debugKeyArray.push_back( key );
				}				
			}

			if ( !debugKeyArray.empty() )
			{
				debugKeys_.push_back( debugKeyArray );
			}
		}
	}

	// If we didn't get any valid debug keys, add a default.
	if ( debugKeys_.empty() )
	{
		KeyCode::KeyArray debugKeyArray;
		debugKeyArray.push_back( KeyCode::KEY_GRAVE );
		debugKeys_.push_back( debugKeyArray );
	}

	keyUpChar_.reset();

	lastFrameEndTime_ = frameTimerValue();
	int frameRate = configSection->readInt( "renderer/maxFrameRate", 0 );
	minFrameTime_ = frameRate != 0 ? uint64(frameTimerFreq() / frameRate) : 0;

	s_framesCounter = configSection->readInt( "debug/framesCount", 0 );

#if ENABLE_HITCH_DETECTION
	// read in vars for hitch detection
	int numTimeSamples = configSection->readInt( "hitchDetector/numTimeSamples", g_profiler.hitchDetector().numTimeSamples() );
	double frameMSLimit = configSection->readDouble( "hitchDetector/frameMSLimit", g_profiler.hitchDetector().frameMSLimit() );
	double overAverageFactor = configSection->readDouble( "hitchDetector/overAverageFactor", g_profiler.hitchDetector().overAverageFactor() );
	g_profiler.hitchDetector().init( numTimeSamples, frameMSLimit, overAverageFactor );
#endif

	// Initialise Access Monitoring.
	AccessMonitor::instance().active( configSection->readBool(
		"accessMonitor", false ) );

	// Check filenames:
#if ENABLE_FILE_CASE_CHECKING
	bool checkFilesCase = configSection->readBool( "debug/checkFileCase", false );
	BWResource::checkCaseOfPaths(checkFilesCase);
#endif // ENABLE_FILE_CASE_CHECKING

	// If there's only one core we need a small sleep every frame,
	// otherwise chunks would never get loaded.  If there's more than one
	// core, we don't need this
	if (!isSingleLogicalProcessor())
	{
		sleepTime_ = 0;
	}

	// initialise all required draw contexts
	for (uint32 i = 0; i < NUM_DRAW_CONTEXTS; i++)
	{
		drawContexts_[i] = NULL;
	}
}


/**
 *	Destructor for the App class.
 */
App::~App()
{
	BW_GUARD;
	for (uint32 i = 0; i < NUM_DRAW_CONTEXTS; i++)
	{
		bw_safe_delete(drawContexts_[i]);
	}

	BWResource::watchAccessFromCallingThread(false);
	fini();
}


/*~ function BigWorld.exit
 *
 *	This function closes the app.
 */
static PyObject *py_exit(PyObject * /*args*/)
{
	BW_GUARD;

    App::instance().quit();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(exit, BigWorld)


/*~ function BigWorld.criticalExit
 *
 *	This function closes the app with a critical massage without call stack.
 */
static void criticalExit( const BW::string & error )
{
	BW_GUARD;

	CriticalMsg( "PY_DEBUG" ).
		source( MESSAGE_SOURCE_SCRIPT ).
		write( "%s\n", error.c_str() );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, criticalExit, ARG( BW::string, END ), BigWorld )

/*~	function BigWorld.setCursor
 *
 *	Sets the active cursor. The active cursor will get all mouse,
 *	keyboard and axis events forwarded to it. Only one cursor can
 *	be active at a time.
 *
 *	@param	cursor	the new active cursor, or None to simply
 *					deactivate the current one.
 */
PyObject * App::py_setCursor( PyObject * args )
{
	BW_GUARD;
	PyObject* pyCursor = NULL;
	if (!PyArg_ParseTuple( args, "O", &pyCursor ))
	{
		PyErr_SetString( PyExc_TypeError, "py_setCursor: Argument parsing error." );
		return NULL;
	}

	// set to null
	InputCursor * cursor = NULL;
	if (pyCursor != Py_None)
	{
		// type check
		if (!InputCursor::Check( pyCursor ))
		{
			PyErr_SetString( PyExc_TypeError, "py_setCursor: Expected a Cursor." );
			return NULL;
		}

		cursor = static_cast< InputCursor * >( pyCursor );
	}

	App::instance().activeCursor( cursor );
	Py_RETURN_NONE;
}
PY_MODULE_STATIC_METHOD( App, setCursor, BigWorld )

extern void initNetwork();
extern void reloadChunks(); //script_bigworld.cpp


#if ENABLE_WATCHERS
uint32 memUsed();

uint32 memoryAccountedFor();
int32 memoryUnclaimed();
#endif




static DogWatch	g_watchTick("Tick");
static DogWatch	g_watchUpdate("Update");
static DogWatch	g_watchOutput("Output");

typedef ScriptObjectPtr<PyModel> PyModelPtr;



HINSTANCE DeviceApp::s_hInstance_ = NULL;
HWND DeviceApp::s_hWnd_ = NULL;
ProgressDisplay * DeviceApp::s_pProgress_ = NULL;
GUIProgressDisplay * DeviceApp::s_pGUIProgress_ = NULL;
ProgressTask * DeviceApp::s_pStartupProgTask_ = NULL;

/*~ function BigWorld.consumerBuild
 *	@components{ client }
 *
 *	Check if this is a consumer build.
 *
 *	@return true if this is a consumer build.
 */
bool consumerBuild()
{
	BW_GUARD;
#if CONSUMER_CLIENT_BUILD
	return true;
#else
	return false;
#endif
}
PY_AUTO_MODULE_FUNCTION( RETDATA, consumerBuild, END, BigWorld )

/*~	function BigWorld.worldDrawEnabled
 *	Sets and gets the value of a flag used to control if the world is drawn.
 *	Note: the value of this flag will also be used to turn the watching of
 *	files being loaded in the main thread on or off. That is, if enabled,
 *	warnings will be issued into the debug output whenever a file is
 *	accessed from the main thread.
 *	@param	newValue	(optional) True enables the drawing of the world, False disables.
 *	@return Bool		If the no parameters are passed the current value of the flag is returned.
 */
static PyObject * py_worldDrawEnabled( PyObject * args )
{
	BW_GUARD;
	if ( PyTuple_Size( args ) == 1 )
	{
		int newDrawEnabled;

		if (!PyArg_ParseTuple( args, "i:BigWorld.worldDrawEnabled",
				&newDrawEnabled ))
		{
			return NULL;
		}
		gWorldDrawEnabled = (newDrawEnabled != 0);

		for( uint i=0; i<sizeof(gWorldDrawLoopTasks)/sizeof(const char*); i++ )
		{
			MainLoopTask * task = MainLoopTasks::root().getMainLoopTask( gWorldDrawLoopTasks[i] );
			if( task )
				task->enableDraw_ = gWorldDrawEnabled;
		}

		// when turning world draw enabled off, turn fs
		// access watching off straight away to prevent
		// warning of files being accessed in this frame.
		if (!gWorldDrawEnabled)
		{
			BWResource::watchAccessFromCallingThread(false);
		}

		Py_RETURN_NONE;
	}
	else if ( PyTuple_Size( args ) == 0 )
	{
		PyObject * pyId = PyBool_FromLong(static_cast<long>(gWorldDrawEnabled));
		return pyId;
	}
	else
	{
		PyErr_SetString(
			PyExc_TypeError,
			"BigWorld.worldDrawEnabled expects one boolean or no arguments." );
		return NULL;
	}
}
PY_MODULE_FUNCTION( worldDrawEnabled, BigWorld )

class PythonServer;

#if ENABLE_WATCHERS
class SumWV : public WatcherVisitor
{
public:

	virtual bool visit( Watcher::Mode type,
		const BW::string & label,
		const BW::string & desc,
		const BW::string & valueStr )
	{
		BW_GUARD;
		if (label.substr( label.size()-4 ) == "Size")
		{
			//dprintf( "\t%s is %s\n", label.c_str(), valueStr.c_str() );
			sum_ += atoi( valueStr.c_str() );
		}
		return true;
	}
	uint32 sum_;
};
static SumWV s_sumWV;
static bool s_memoryAccountedFor_running = false;
uint32 memoryAccountedFor()
{
	BW_GUARD;
	if (s_memoryAccountedFor_running) return 0;
	s_memoryAccountedFor_running = true;
	s_sumWV.sum_ = 0;
#if ENABLE_WATCHERS
	Watcher::rootWatcher().visitChildren( NULL, "Memory/", s_sumWV );
#endif
	s_memoryAccountedFor_running = false;
	return s_sumWV.sum_ / 1024;
}
int32 memoryUnclaimed()
{
	BW_GUARD;
	// we are zero if memoryAccountedFor is running, even if
	// we didn't run it (since we are not accounted for!)
	if (s_memoryAccountedFor_running) return 0;
	memoryAccountedFor();
	return (memUsed() * 1024) - s_sumWV.sum_;
}
#endif

/**
 *	Shows a critical initialisation error message box.
 *	Except in release builds, explanations about what can
 *	be wrong and where to look for more information on running
 *	the client will be displayed in the message box.
 */
void criticalInitError( const char * format, ... )
{
	BW_GUARD;
	// build basic error message
	char buffer1[ BUFSIZ * 2 ];

	va_list argPtr;
	va_start( argPtr, format );
	bw_vsnprintf( buffer1, sizeof(buffer1), format, argPtr );
	buffer1[sizeof(buffer1)-1] = '\0';
	va_end( argPtr );

	// add additional explanations
	const char pathMsg[] =
#ifndef _RELEASE
		"%s\n\nThe most probable causes for this error are running "
		"the game executable from the wrong working directory or "
		"having a wrong BW_RES_PATH environment variable. For more "
		"information on how to correctly setup and run BigWorld "
		"client, please refer to the Client Installation Guide, "
		"in bigworld/doc directory.\n";
#else
		"%s\n";
#endif

	CRITICAL_MSG( pathMsg, buffer1 );
}

// memory currently used in KB
uint32 memUsed()
{
	BW_GUARD;
	VersionInfo	* pVI = DebugApp::instance.pVersionInfo_;
	if (pVI == NULL) return 0;
	return static_cast<uint32>(pVI->workingSetRefetched());
}


/*~	function BigWorld.totalPhysicalMemory
 *	@components{ client }
 *
 *	Return the amount of physical RAM in the machine.  This can
 *	help when a game automatically chooses graphics settings for
 *	a user.  There are a few graphics settings that affect the
 *	amount of system RAM used.  Two examples are:
 *	- texture detail (managed textures are used, and they are
 *	mirrored in RAM)
 *	- the far plane distance directly affects how many chunks are
 *	loaded and thus how many unique assets will be loaded at any time.
 *
 *	@return Integer	The amount of physical RAM, in bytes.
 */
uint32 totalPhysicalMemory()
{
	BW_GUARD;
	VersionInfo	* pVI = DebugApp::instance.pVersionInfo_;
	if (pVI == NULL) return 0;
	return static_cast<uint32>(pVI->totalPhysicalMemory());
}
PY_AUTO_MODULE_FUNCTION( RETDATA, totalPhysicalMemory, END, BigWorld );


/*~	function BigWorld.totalVirtualMemory
 *	@components{ client }
 *
 *	Return the amount of virtual memory, a.k.a the amount of address
 *	space available to the process.  Used in conjunction with the
 *	totalPhysicalMemory function, you should be able to determine whether
 *	or not the user running a 64 bit client and therefore whether or not
 *	your game has access to the full amount of physical RAM installed.
 *
 *	This can help a game automatically choose graphics settings for
 *	a user.  There are a few graphics settings that affect the
 *	amount of system RAM used.  Two examples are:
 *	- texture detail (managed textures are used, and they are
 *	mirrored in RAM)
 *	- the far plane distance directly affects how many chunks are
 *	loaded and thus how many unique assets will be loaded at any time.
 *
 *	@return Integer	The amount of virtual RAM, in bytes.
 */
uint32 totalVirtualMemory()
{
	BW_GUARD;
	VersionInfo	* pVI = DebugApp::instance.pVersionInfo_;
	if (pVI == NULL) return 0;
	return static_cast<uint32>(pVI->totalVirtualMemory());
}
PY_AUTO_MODULE_FUNCTION( RETDATA, totalVirtualMemory, END, BigWorld );


/**
 *  Device callback object to provide Personality.onRecreateDevice() hook.
 */
class RecreateDeviceCallback : public Moo::DeviceCallback
{
public:
	static void createInstance() 
	{ 
		if (s_instance_== NULL)
			s_instance_ = new RecreateDeviceCallback();
	}
	static void deleteInstance()
	{
		bw_safe_delete(s_instance_);
	}


	/*~ function BWPersonality.onRecreateDevice
	 *	@components{ client }
	 *	This callback method is called on the personality script when
	 *	the Direct3D device has asked the engine to recreate all
	 *	resources not managed by Direct3D itself.
	 */
	void createUnmanagedObjects()
	{
		BW_GUARD;
		if (Personality::instance())
		{
			ScriptObject func = Personality::instance().getAttribute(
				"onRecreateDevice", ScriptErrorClear() );

			if (func)
			{
				Script::callNextFrame(
					func.newRef(), PyTuple_New(0),
					"RecreateDeviceCallback::createUnmanagedObjects: " );
			}
		}
	}

	virtual bool recreateForD3DExDevice() const  { return true; }

	static RecreateDeviceCallback* s_instance_;
};

RecreateDeviceCallback* RecreateDeviceCallback::s_instance_ = NULL;


#if ENABLE_WATCHERS
/**
 *	WatcherVisitor that prints memory counters
 */
static class MemoryWV : public WatcherVisitor
{
	virtual bool visit( Watcher::Mode type,
		const BW::string & label,
		const BW::string & desc,
		const BW::string & valueStr )
	{
		BW_GUARD;
		dprintf( "%s\t%s\n", label.c_str(), valueStr.c_str() );
		return true;
	}
} s_mwv;

/*~ function BigWorld.dumpMemCounters
 *	@components{ client }
 *
 *	This debugging function prints out the current value of memory watchers
 *	found in "Memory/" watcher directory.
 */
void dumpMemCounters()
{
	BW_GUARD;
	Watcher::rootWatcher().visitChildren( NULL, "Memory/", s_mwv );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, dumpMemCounters, END, BigWorld );
#endif


bool App::initLoggers()
{
	typedef BW::vector<DataSectionPtr> FileLoggers;
	FileLoggers fileLoggerSections;

	DataSectionPtr configSection = AppConfig::instance().pRoot();

	configSection->openSections( "logging/file", fileLoggerSections );
	size_t idx = 0;
	FileLoggers::iterator sectIt = fileLoggerSections.begin();
	for (; sectIt < fileLoggerSections.end(); ++sectIt, ++idx)
	{
		PyDebugMessageFileLoggerPtr pFileLogger =  
			PyDebugMessageFileLoggerPtr( new PyDebugMessageFileLogger, 
			PyDebugMessageFileLoggerPtr::FROM_NEW_REFERENCE );
		pFileLogger->configFromDataSection( *sectIt );
		if (!configCreatedFileLoggers_.addFileLogger( pFileLogger ))
		{
			WARNING_MSG( "App::App: The logging files specified is more than allowed,"
				" only the first %d files will be loaded\n", idx );
			return false;
		}
	}

	return true;
}
/**
 *	This method initialises the application.
 *
 *	@param hInstance	The HINSTANCE associated with this application.
 *	@param hWnd			The main window associated with this application.
 *
 *	@return		True if the initialisation succeeded, false otherwise.
 */
bool App::init( HINSTANCE hInstance, HWND hWnd )
{
	BW_GUARD;
	MF_ASSERT( currentState_ == STATE_UNINITIALISED );
	currentState_ = STATE_INITIALISED;

	hWnd_ = hWnd;

	// Pass some parameters
	DeviceApp::s_hInstance_ = hInstance;
	DeviceApp::s_hWnd_ = hWnd;
	DeviceApp::instance.pInputHandler_ = this;

	bool enableLoggingAssetMsg = AppConfig::instance().pRoot()->readBool( "debug/enableLoggingAssetMsg", false );
	if (enableLoggingAssetMsg)
	{
		DebugFilter::setAlwaysHandleMessage( true );
		DebugFilter::instance().addMessageCallback( this, false );
	}
	
#if ENABLE_CONSOLES
	ConsoleManager::createInstance();
#endif // ENABLE_CONSOLES

#if ENABLE_ASSET_PIPE
	pAssetClient_ = AssetClientPtr( new AssetClient() );
	DeviceApp::instance.assetClient( pAssetClient_.get() );
	WorldApp::instance.assetClient( pAssetClient_.get() );
#else
	DeviceApp::instance.assetClient( pAssetClient_ );
	WorldApp::instance.assetClient( pAssetClient_ );
#endif

	// Compress animations on load so we'll only save the compressed versions
    Moo::InterpolatedAnimationChannel::inhibitCompression( false );

	pCameraApp_ = CameraAppPtr( new CameraApp() );

	// Set up the MainLoopTask groups and dependencies
	MainLoopTasks::root().add( NULL, "Device", NULL );
	MainLoopTasks::root().add( NULL, "VOIP",   ">Device", NULL );
	MainLoopTasks::root().add( NULL, "Web",   ">VOIP", NULL );
	MainLoopTasks::root().add( NULL, "Script", ">Web",   NULL );
	MainLoopTasks::root().add( NULL, "Camera", ">Script", NULL );
	MainLoopTasks::root().add( NULL, "Canvas", ">Camera", NULL );
	MainLoopTasks::root().add( NULL, "World",  ">Canvas", NULL );
	MainLoopTasks::root().add( NULL, "Flora",  ">World",  NULL );
	MainLoopTasks::root().add( NULL, "Facade", ">Flora",  NULL );
	MainLoopTasks::root().add( NULL, "Lens",   ">Facade", NULL );
	MainLoopTasks::root().add( NULL, "GUI",    ">Lens",   NULL );
	MainLoopTasks::root().add( NULL, "Debug",  ">GUI",    NULL );
	MainLoopTasks::root().add( NULL, "Finale", ">Debug",  NULL );

	// And initialise them all!
	if (!MainLoopTasks::root().init())
	{
		return false;
	}

	if (!initLoggers())
	{
		return false;
	}

	RecreateDeviceCallback::createInstance();

	MF_WATCH( "Debug/debugKeyEnable",
		debugKeyEnable_,
		Watcher::WT_READ_WRITE,
		"Toggle use of the debug key" );

	MF_WATCH( "Debug/activeConsole", *this, MF_ACCESSORS( BW::string, App, activeConsole ) );

	MF_WATCH( "Debug/Sleep time (ms)",
		sleepTime_,
		Watcher::WT_READ_WRITE,
		"Number of milliseconds to pause (yield) in-between frames." );

	// Only set a default cursor if the personality script didn't set one
	if ( this->activeCursor_ == NULL )
		this->activeCursor( &DirectionCursor::instance() );


	// Unload the loading screen
	freeLoadingScreen();

	// Make sure we setup the effect visual context constants here to make
	// sure that a space with only terrain will still render correctly
	Moo::rc().effectVisualContext().initConstants();

	// We reset to a new frame here, so that our dRenderTime_ and dGameTime_
	// do not see a huge first frame due to initialisation.
	frameStartRenderTime_ = frameTimerValue();

	// Disable file monitor if it's lower than vista
	bool enable = true;
	VersionInfo	* pVI = DebugApp::instance.pVersionInfo_;
	if (pVI->OSMajor() < 6)
	{
		enable = false;
	}

	BWResource::instance().enableModificationMonitor( enable );

#if ENABLE_GPU_PROFILER
	Moo::GPUProfiler::instance().init();
#endif

	drawContexts_[COLOUR_DRAW_CONTEXT] = new Moo::DrawContext( Moo::RENDERING_PASS_COLOR );
	drawContexts_[COLOUR_DRAW_CONTEXT]->attachWatchers( "Colour", false );
	drawContexts_[SHADOW_DRAW_CONTEXT] = new Moo::DrawContext( Moo::RENDERING_PASS_SHADOWS );
	drawContexts_[SHADOW_DRAW_CONTEXT]->attachWatchers( "Shadow", false );

	return true;
}


/**
 *	This method finalises the application.
 *	App finalisation involves the following steps:
 *	a. It should finalise in the reverse order of App::init().
 *	b. Finalise global objects which do not have an explicit init().
 */
void App::fini()
{
	BW_GUARD;

	// Check fini() is not called multiple times
	MF_ASSERT( currentState_ != STATE_FINALISED );

	// Check fini() is only called when App is in a valid state
	// This could happen if App failed to initialise,
	// so return instead of asserting
	if (currentState_ != STATE_INITIALISED)
	{
		return;
	}
	currentState_ = STATE_FINALISED;

	//
	// a. Finalise in the reverse order from App::init

#if ENABLE_GPU_PROFILER
	Moo::GPUProfiler::instance().fini();
#endif

	DebugFilter::instance().deleteMessageCallback( this );
	RecreateDeviceCallback::deleteInstance();
	MainLoopTasks::finiAll();

	//MainLoopTasks::finiAll() will have called fini in pCameraApp_
	pCameraApp_.reset();

#if ENABLE_CONSOLES
	ConsoleManager::deleteInstance();
#endif // ENABLE_CONSOLES

	//
	// b. Extra things to finalise which were not in App::init

	// DogWatchManager is created on first use, so delete at end of App
	DogWatchManager::fini();

	s_scriptsPreferences = NULL;

	BWResource::fini();

#if ENABLE_DPRINTF
	bw_safe_delete( pMessageTimePrefix_ );
#endif // ENABLE_DPRINTF

	// Do instance last
	MF_ASSERT( pInstance_ != NULL );
	pInstance_ = NULL;
}


/**
 *	This class stores loading screen information.
 *	This class is only used by the method App::loadingScreen()
 *
 *	@see App::loadingScreen
 */
class LoadingScreenInfo
{
public:
	LoadingScreenInfo( BW::string & name, bool fullScreen ) :
		name_( name ),
		fullScreen_( fullScreen )
	{
	}

	typedef BW::vector< LoadingScreenInfo >	Vector;

	BW::string & name( void )	{ return name_; }
	bool fullScreen( void )		{ return fullScreen_; }
private:
	BW::string name_;
	bool		fullScreen_;
};



/**
 *	This method displays the loading screen.
 *	Assumes beginscene has already been called.
 */
static BW::string lastUniverse = "/";
static Moo::Material loadingMat;

bool displayLoadingScreen()
{
	BW_GUARD;
	if (!BWProcessOutstandingMessages())
		return false;

	if (DeviceApp::s_pGUIProgress_)
	{
		return true;
	}

	static bool fullScreen = true;
	static CustomMesh<Moo::VertexTLUV> mesh( D3DPT_TRIANGLESTRIP );
	static bool s_inited = false;

	//HACK_MSG( "displayLoadingScreen: heap size is %d\n", heapSize() );

	if (s_inited)
	{
		if ( loadingScreenName.value() != "" )
		{
			loadingMat.set();
			mesh.draw();
		}

#if ENABLE_CONSOLES
		// and we draw the status console here too...
		XConsole * pStatCon = ConsoleManager::instance().find( "Status" );
		if (pStatCon != NULL) pStatCon->draw( 1.f );
#endif // ENABLE_CONSOLES
		return true;
	}

	Moo::BaseTexturePtr	loadingBack = Moo::TextureManager::instance()->get( loadingScreenName );

	loadingMat = Moo::Material();
	Moo::TextureStage ts;
	ts.pTexture( loadingBack );
	ts.useMipMapping( false );
	ts.colourOperation( Moo::TextureStage::SELECTARG1 );
	loadingMat.addTextureStage( ts );
	Moo::TextureStage ts2;
	ts2.colourOperation( Moo::TextureStage::DISABLE );
	ts2.alphaOperation( Moo::TextureStage::DISABLE );
	loadingMat.addTextureStage( ts2 );
	loadingMat.fogged( false );

	Moo::VertexTLUV vert;
	mesh.clear();

	vert.colour_ = 0xffffffff;
	vert.pos_.z = 0;
	vert.pos_.w = 1;

	vert.pos_.x = 0;
	vert.pos_.y = 0;
	vert.uv_.set( 0,0 );
	mesh.push_back( vert );

	vert.pos_.x = Moo::rc().screenWidth();
	vert.pos_.y = 0;
	vert.uv_.set( 1,0 );
	mesh.push_back( vert );

	vert.pos_.x = 0;
	vert.pos_.y = Moo::rc().screenHeight();
	vert.uv_.set( 0,1 );
	mesh.push_back( vert );

	vert.pos_.x = Moo::rc().screenWidth();
	vert.pos_.y = Moo::rc().screenHeight();
	vert.uv_.set( 1,1 );
	mesh.push_back( vert );

	//fix texel alignment
	for ( int i = 0; i < 4; i++ )
	{
		mesh[i].pos_.x -= 0.5f;
		mesh[i].pos_.y -= 0.5f;
	}

	s_inited = true;

	// call ourselves to draw now that we're set up
	return displayLoadingScreen();
}


/**
 *	This method ensures the resources we used just for loading are freed up.
 */
void freeLoadingScreen()
{
	lastUniverse = "/";
	loadingMat = Moo::Material();
}

/**
 *	Draw this loading text message. They appear beneath the progress bars
 */
void loadingText( const BW::string& s )
{
#if ENABLE_CONSOLES
	ConsoleManager::instance().find( "Status" )->print( s + "\n" );
#endif // ENABLE_CONSOLES
	if (DeviceApp::s_pProgress_ != NULL)
	{
		DeviceApp::s_pProgress_->draw( true );
	}

	INFO_MSG( "%s\n", s.c_str() );
}

// -----------------------------------------------------------------------------
// Section: Update and Draw functions
// -----------------------------------------------------------------------------


/**
 *	This method is called once per frame to do all the application processing
 *	for that frame.
 */
bool App::updateFrame(bool active)
{
	BW_GUARD;
#if ENABLE_PROFILER
	g_profiler.setScreenWidth( Moo::rc().screenWidth() );
	g_profiler.tick();
#endif

	// Timing
	this->calculateFrameTime();

	// Mouse clipping
	MouseCursor::updateMouseClipping();

	// Resource monitoring
	BWResource::instance().flushModificationMonitor();

	// Only tick and draw if some time has passed, this fixes an issue with 
	// minimising on intel cpus.
	if (dRenderTime_ > 0.f)
	{
		if (active)
		{
			// Now tick (and input)
			{
				GPU_PROFILER_SCOPE( App_Tick );
				PROFILER_SCOPED( App_Tick );
				g_watchTick.start();
				MainLoopTasks::root().tick( dGameTime_, dRenderTime_ );
				g_watchTick.stop();
			}

			
			if (Moo::rc().checkDevice())
			{
				// Update animations
				{
					GPU_PROFILER_SCOPE( App_UpdateAnimations );
					PROFILER_SCOPED( App_UpdateAnimations );
					ScopedDogWatch sdw( g_watchUpdate );

					MainLoopTasks::root().updateAnimations( dGameTime_ );
				}

				// And draw
				{
					PROFILER_SCOPED( App_Draw );
					ScopedDogWatch sdw( g_watchOutput );
					MainLoopTasks::root().draw();
				}
			}
		}
		else
		{
			MainLoopTasks::root().inactiveTick( dGameTime_, dRenderTime_ );
		}

		int sleepTime = 0;

		uint64 frameEndTime = frameTimerValue();
		if (minFrameTime_ > 0 && frameEndTime < lastFrameEndTime_ + minFrameTime_)
		{
			sleepTime = uint32((lastFrameEndTime_ + minFrameTime_ - frameEndTime) * 1000 / frameTimerFreq());
		}

		sleepTime = max( sleepTime, sleepTime_ );

		if ( sleepTime )
		{
			PROFILER_SCOPED( Sys_Sleep );
			::Sleep( sleepTime );
		}

		lastFrameEndTime_ = frameTimerValue();

		if (s_framesCounter <= 0)
		{
			return true;
		}
		else	
		{
			--s_framesCounter;
			if (s_framesCounter % 100 == 0)
			{
				DEBUG_MSG("s_framesCounter: %d\n", s_framesCounter);
			}
			return  s_framesCounter > 0;
		}
	}
	return true;
}





/**
 *	This method is called once a frame by updateFrame to update the scene.
 *
 *	@param dTime	The amount of time that has elapsed since the last update.
 */
void App::updateScene( float dTime )
{
	BW_GUARD;	
}

/**
 *	This method updates the pose of any cameras in the scene.
 */
void App::updateCameras( float dTime )
{
	BW_GUARD;	
}

/**
 *	This method renders a frame using the current camera and current scene.
 */
void App::renderFrame()
{
	BW_GUARD;	
}

/**
 *	This method draws the 3D world, i.e. anything that uses the Z buffer
 */
void App::drawWorld()
{
	BW_GUARD;	
}


/**
 *	This method draws what is considered to be the scene, i.e. everything
 *	that is placed at a definite transform.
 */
void App::drawScene()
{
	BW_GUARD;	
}

/**
 *	This method is called to start quitting the application.
 *	You may optionally restart the application.
 */
void App::quit( bool restart )
{
	BW_GUARD;
	if (restart)
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(pi) );
		::CreateProcess( NULL, ::GetCommandLine(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi );
	}

	INFO_MSG("Quitting app\n");
	PostMessage( hWnd_, WM_CLOSE, 0, 0 );
}


/**
 *	This method overrides the InputHandler method to handle keyboard events. 
 *
 *	@param event	The key event.
 *
 *	@return			True if the event was handled, false otherwise.
 */
bool App::handleKeyEvent(const KeyEvent & event)
{
	BW_GUARD;
	HandleKeyEventHolder holder(*this);

	// Create debug key event if the debug key is enabled.
	if ( debugKeyEnable_ && handleKeyEventDepth_ == 0 && 
		 event.key() != KeyCode::KEY_DEBUG && 
		 !event.isRepeatedEvent() && 
		 checkDebugKeysState( event.key() ) )
	{
		// checkDebugKeysState will pass if the OTHER keys in the debug key combination
		// are also pressed. It doesn't check the state of event.key, so we can get keyup
		// events from the debug key.
		KeyEvent thisEvent = KeyEvent::make( KeyCode::KEY_DEBUG, event.isKeyDown(), event.modifiers(), event.cursorPosition() );
		InputDevices::keyStates().setKeyStateNoRepeat( KeyCode::KEY_DEBUG, event.isKeyDown() );
		this->handleKeyEvent( thisEvent );
	}

	bool handled = false;

	// check if this is a key up event
	eEventDestination keySunk = EVENT_SINK_NONE;
	if (event.isKeyUp())
	{
		// post off a char event on keyup if there is one associated with it.
		if ( keyUpChar_.key() != KeyCode::KEY_NONE )
		{
			handleKeyEvent( KeyEvent::make(	keyUpChar_.key(), 
											InputDevices::keyRepeatCount(keyUpChar_.key())+1, 
											keyUpChar_.utf16Char(),
											InputDevices::modifiers(),
											keyUpChar_.cursorPosition() ) );
			keyUpChar_.reset();
		}

		// disallow it when there has been no recorded key down.
		// this can happen when events are lost
		if (keyRouting_[event.key()] == EVENT_SINK_NONE) return true;

		// otherwise cache where it when to
		keySunk = keyRouting_[event.key()];

		// and clear it in advance
		keyRouting_[event.key()] = EVENT_SINK_NONE;
	}

	// If the key associated with the character is a debug key, then only
	// send the auto-repeats. We will send the first char event event if
	// no repeats occured on the key-up.
	if ( debugKeyEnable_ && 
		 event.key() != KeyCode::KEY_DEBUG &&
		 wcscmp( event.utf16Char(), L"" ) != 0 &&
		 event.isKeyDown() && isDebugKey( event.key() ) )
	{
		if( event.repeatCount() == 0 && keyUpChar_.key() == KeyCode::KEY_NONE )
		{
			keyUpChar_.fill( event );
			handled = true;
		}
		else
		{
			keyUpChar_.reset();
		}
	}

	// disable IME while the debug key is held down.
	if (IME::pInstance() && event.key() == KeyCode::KEY_DEBUG)
	{
		IME::instance().allowEnable( event.isKeyUp() 
#if ENABLE_CONSOLES
			&& ConsoleManager::instance().pActiveConsole() == NULL
#endif // ENABLE_CONSOLES
			);
	}

	// only consider debug keys if it is down
	if (!handled && event.isKeyDown() &&
		InputDevices::isKeyDown( KeyCode::KEY_DEBUG ))
	{
		handled = this->handleDebugKeyDown( event );
		if (handled)
		{
			keyRouting_[event.key()] = EVENT_SINK_DEBUG;
			keyUpChar_.reset();
		}
	}

	// consume keyups corresponding to EVENT_SINK_DEBUG keydowns
	if (keySunk == EVENT_SINK_DEBUG) handled = true;

#if ENABLE_CONSOLES
	// give the active console and console manager a go
	if (!handled)
	{
		handled = ConsoleManager::instance().handleKeyEvent( event );
		if (handled && event.isKeyDown())
		{
			keyRouting_[event.key()] = EVENT_SINK_CONSOLE;
		}
	}
#endif // ENABLE_CONSOLES

	// consume keyups corresponding to EVENT_SINK_CONSOLE keydowns
	if (keySunk == EVENT_SINK_CONSOLE) handled = true;

	// post off to the script global hooks
	if ( !handled )
	{
		InputEvent ievent;
		ievent.type_ = InputEvent::KEY;
		ievent.key_ = event;

		handled = BigWorldClientScript::sinkKeyboardEvent(ievent);
	}

	// now give the personality script its go
	if (!handled )
	{
		ScriptObject ret= Personality::instance().callMethod( "handleKeyEvent",
			ScriptArgs::create( event ),
			ScriptErrorPrint( "Personality handleKeyEvent: " ),
			/*allowNullMethod*/ false );
		
		handled = ret && ret.isTrue( ScriptErrorPrint(
				"Personality handleKeyEvent retval" ) );

		if (handled && event.isKeyDown())
		{
			keyRouting_[event.key()] = EVENT_SINK_PERSONALITY;
		}
	}

	// consume keyups corresponding to EVENT_SINK_PERSONALITY keydowns
	if (keySunk == EVENT_SINK_PERSONALITY) handled = true;


	// give the app its chance (it only wants keydowns)
	if (!handled && event.isKeyDown())
	{
		handled = this->handleKeyDown( event );
		if (handled && event.isKeyDown())
		{
			keyRouting_[event.key()] = EVENT_SINK_APP;
		}
	}

	// consume keyups corresponding to EVENT_SINK_APP keydowns
	if (keySunk == EVENT_SINK_APP) handled = true;


	// finally let the script have the crumbs
	//  (has to be last since we don't extract 'handled' from it)
	if (!handled)
	{
		Entity * pPlayer = ScriptPlayer::entity();
		if (pPlayer != NULL && !pPlayer->isDestroyed())
		{
			pPlayer->pPyEntity().callMethod( "handleKeyEvent",
				ScriptArgs::create( event ), ScriptErrorPrint() );
		}

		// TODO: Look at return value and set 'handled'

		// For now, sink all key presses in scripts
		if (event.isKeyDown())
		{
			keyRouting_[event.key()] = EVENT_SINK_SCRIPT;
			handled = true;
		}
		else if (event.isKeyUp())
		{
			keyRouting_[event.key()] = EVENT_SINK_NONE;
			handled = true;
		}
	}


	// and for sanity make sure the key routing entry is cleared
	//  if we got a key up (it should already have been)
	if (keySunk != EVENT_SINK_NONE && !handled)
	{
		WARNING_MSG( "KeyUp for 0x%02x routed to %d was unclaimed!\n",
			int(event.key()), int(keySunk) );
	}

	return handled;
}

/**
 *	This method overrides the InputHandler method to handle the even raised
 *	when the user changes the current input language.
 *
 *	@return			True if the event was handled, false otherwise.
 */
bool App::handleInputLangChangeEvent()
{
	// Post it directly to the personality script if the function is defined.
	ScriptObject ret = Personality::instance().callMethod(
		"handleInputLangChangeEvent",
		ScriptErrorPrint( "Personality handleInputLangChangeEvent: " ),
		/*allowNullMethod*/ true );

	return ret && ret.isTrue( ScriptErrorPrint(
			"Personality handleInputLangChangeEvent retval" ) );
}

/**
 *	This method overrides the InputHandler method to handle IME related events. 
 *	IME events occur when the active Input Method Editor has changed state.
 *
 *	@param event	The current IME event.
 *
 *	@return			True if the event was handled, false otherwise.
 */
bool App::handleIMEEvent( const IMEEvent & event )
{
	// Post it directly to the personality script if the function is defined.

	ScriptObject ret = Personality::instance().callMethod( "handleIMEEvent",
		ScriptArgs::create( event ),
		ScriptErrorPrint( "Personality handleIMEEvent: " ),
		/*allowNullMethod*/ true );

	return ret && ret.isTrue( ScriptErrorPrint(
			"Personality handleIMEEvent retval" ) );
}

void App::clientChatMsg( const BW::string & msg )
{
	BW_GUARD;
	if (!Py_IsInitialized())
	{
		return;
	}

	if(!Personality::instance().hasAttribute( "addChatMsg" ))
	{
		DEBUG_MSG( "Personality script does not have 'addChatMsg' method"
			"to display output message." );
		return ;
	}
	
	Personality::instance().callMethod( "addChatMsg",
		ScriptArgs::create( -1, msg ), ScriptErrorPrint( "addChatMsg" ),
		/* allowNullMethod */ false );
}


/**
 *	This method triggers a personality script callback for the memory critical
 *	method.
 */
void App::memoryCriticalCallback()
{
	BW_GUARD;
	ScriptObject personality = Personality::instance();

	if (personality)
	{
		ScriptObject func = personality.getAttribute( "onMemoryCritical", ScriptErrorClear() );
		if (func)
		{
			func.callFunction( ScriptErrorClear() );
		}
		else
		{
			App::instance().clientChatMsg(
				"WARNING: Memory load critical, adjust your detail settings.\n" );
		}
	}
}

/**
 *	This function handles key and button down events.
 *	It is called by App::handleKeyEvent above.
 */
bool App::handleKeyDown( const KeyEvent & event )
{
	BW_GUARD;
	bool handled = true;

	if ( event.isRepeatedEvent() )
	{
		return false;
	}

	switch (event.key())
	{

	case KeyCode::KEY_F4:
		if (event.isAltDown())
		{
			this->quit();
		}
		else
		{
			handled = false;
		}
		break;

	case KeyCode::KEY_SYSRQ:
		{

// The super-shot functionality will only work it watchers are enabled
#if ENABLE_WATCHERS

			if (event.isCtrlDown()) //Toggle super screenshot settings in engine_config.xml/superShot.
			{
				static bool s_superShotEnabled = false;

				static BW::string s_backBufferWidthXML = AppConfig::instance().pRoot()->readString("superShot/hRes", "2048");
				static BW::string s_farPlaneDistXML = AppConfig::instance().pRoot()->readString("superShot/farPlaneDist", "1500");
				static uint32 s_floraVBSizeXML = AppConfig::instance().pRoot()->readInt("superShot/floraVBSize", 16000000);

				static BW::string s_backBufferWidth;
				static BW::string s_farPlaneDist;
				static uint32 s_floraVBSize;
				static bool isWindowed = Moo::rc().windowed();
				s_superShotEnabled = !s_superShotEnabled;

				if (s_superShotEnabled) //Store current settings and apply new settings.
				{
					//Store current settings.
					Watcher::Mode wMode;
					Watcher::rootWatcher().getAsString(NULL, "Render/backBufferWidthOverride", s_backBufferWidth, wMode);
					Watcher::rootWatcher().getAsString(NULL, "Render/Far Plane", s_farPlaneDist, wMode);
					s_floraVBSize = DeprecatedSpaceHelpers::cameraSpace()->enviro().flora()->vbSize();
					isWindowed = Moo::rc().windowed();

					if(!isWindowed) //set to windowed mode
					{
						Moo::rc().changeMode( Moo::rc().modeIndex(), true );
					}

					//Apply settings from resources.xml.
					Watcher::rootWatcher().setFromString(NULL, "Render/backBufferWidthOverride", s_backBufferWidthXML.c_str());
					Watcher::rootWatcher().setFromString(NULL, "Render/Far Plane", s_farPlaneDistXML.c_str());
					DeprecatedSpaceHelpers::cameraSpace()->enviro().flora()->vbSize(s_floraVBSizeXML);
					if(AppConfig::instance().pRoot()->readBool("superShot/disableGUI"))
					{
						SimpleGUI::instance().setUpdateEnabled(false);
					}
				}
				else
				{
					//Restore previous settings.
					Watcher::rootWatcher().setFromString(NULL, "Render/backBufferWidthOverride", s_backBufferWidth.c_str());
					Watcher::rootWatcher().setFromString(NULL, "Render/Far Plane", s_farPlaneDist.c_str());
					DeprecatedSpaceHelpers::cameraSpace()->enviro().flora()->vbSize(s_floraVBSize);
					if(AppConfig::instance().pRoot()->readBool("superShot/disableGUI"))
					{
						SimpleGUI::instance().setUpdateEnabled(true);
					}
					if(!isWindowed)
					{
						Moo::rc().changeMode( Moo::rc().modeIndex(), false );
					}
				}

				//Notify user know of toggle.
				if (s_superShotEnabled)
					clientChatMsg( "High Quality Screenshot Settings Enabled" );
				else
					clientChatMsg( "High Quality Screenshot Settings Disabled" );
			}
			else

#endif //ENABLE_WATCHERS

			{
				DataSectionPtr rootDS = AppConfig::instance().pRoot();

				PathedFilename pathedFile( 
								rootDS->openSection( "screenShot/path" ),
								"", PathedFilename::BASE_EXE_PATH );

				BW::string fullName = 
					pathedFile.resolveName() + "/" + 
					rootDS->readString( "screenShot/name", "shot" );

				BWResource::ensureAbsolutePathExists( fullName );

				BW::string fileName = Moo::rc().screenShot( 
					rootDS->readString("screenShot/extension", "bmp"),
					fullName );
				if( !fileName.empty() )
				{
					clientChatMsg( "Screenshot saved: " + BWResource::getFilename( fileName ) );
				}
			}
		}
		break;

	case KeyCode::KEY_RETURN:
		if (InputDevices::isAltDown( ))
		{
			if (Moo::rc().device())
			{
				Moo::rc().changeMode( Moo::rc().modeIndex(), !Moo::rc().windowed() );
			}
		}
		else
		{
			handled = false;
		}
		break;

	case KeyCode::KEY_TAB:
		if (!event.isAltDown())
		{
			handled = false;
		}
		break;

	default:
		handled = false;
	}

	return handled;
}


/**
 *	Handle a debugging key. Only gets called if debug key is pressed.
 */
bool App::handleDebugKeyDown( const KeyEvent & event )
{
	BW_GUARD;	
#if !ENABLE_CONSOLES

	return false;

#else // ENABLE_CONSOLES

	if ( event.isRepeatedEvent() )
	{
		return false;
	}


	bool handled = true;

	switch (event.key())
	{

	case KeyCode::KEY_F2:
	{
		RECT clientRect;
		GetClientRect( hWnd_, &clientRect );

		const int numSizes = 4;
		POINT sizes[numSizes] = {
			{ 512, 384 }, { 640, 480 },
			{ 800, 600 }, { 1024, 768 } };

		int i = 0;
		int width = this->windowSize().x;
		for (; i < numSizes; ++i)
		{
			if (sizes[i].x == width)
			{
				break;
			}
		}
		i = (i + (event.isShiftDown() ? numSizes-1 : 1)) % numSizes;
		this->resizeWindow(sizes[i].x, sizes[i].y);
		BW::stringstream s;
		s << "Resolution: " << sizes[i].x << " x " << sizes[i].y << std::endl; 
		clientChatMsg(s.str()); 
		break;
	}

	case KeyCode::KEY_F4:
		if( !( event.isCtrlDown() || event.isAltDown() ) )
		{
			ConsoleManager::instance().toggle( "Histogram" );
		}
		break;

	case KeyCode::KEY_F6:
	{
		int modsum =
				( event.isCtrlDown()  ? EnviroMinder::DRAW_SKY_GRADIENT : 0 ) |
				( event.isAltDown()   ? EnviroMinder::DRAW_SKY_BOXES    : 0 ) |
				( event.isShiftDown() ?
					EnviroMinder::DRAW_SUN_AND_MOON +
					EnviroMinder::DRAW_SUN_FLARE : 0 );

		if (!modsum)
		{
			// toggle all sky drawing options
			CanvasApp::instance.drawSkyCtrl_ =
				CanvasApp::instance.drawSkyCtrl_ ? 0 : EnviroMinder::DRAW_ALL;
			clientChatMsg( "toggle: all sky drawing options" );
		}
		else
		{
			// toggle the option formed by adding the modifier's binary values
			CanvasApp::instance.drawSkyCtrl_ = CanvasApp::instance.drawSkyCtrl_ ^ modsum;

			BW::string s = "toggle: ";
			s.append(event.isCtrlDown() ? "'Sky Gradient' " : "");
			s.append(event.isAltDown() ? "'Sky Boxes' " : "");
			s.append(event.isShiftDown() ? "'Sun & Moon Flare'" : "");
			clientChatMsg(s);
			// as in other areas (such as watcher menus),
			// shift is worth one, ctrl is worth 2, and alt is worth 4.
		}
		break;
	}


	case KeyCode::KEY_F8:
		{
			WorldApp::instance.wireFrameStatus_++;
			BW::stringstream s;
			s << "wireframe status: " << WorldApp::instance.wireFrameStatus_ << std::endl;
			clientChatMsg(s.str());
		}
		break;


	case KeyCode::KEY_F9:
		{
			bool darkBG = ConsoleManager::instance().darkenBackground();
			ConsoleManager::instance().setDarkenBackground(!darkBG);
		}
		break;

	case KeyCode::KEY_F10:
		{
			Moo::Camera cam = Moo::rc().camera();
			cam.ortho( !cam.ortho() );
			Moo::rc().camera( cam );
		}
		break;

	case KeyCode::KEY_F11:
	{
		DEBUG_MSG( "App::handleKeyDown: Reloading entity script classes...\n" );

		if (EntityType::reload())
		{
			DEBUG_MSG( "App::handleKeyDown: reload successful!\n" );
			clientChatMsg( "App: Script reload succeeded." );
		}
		else
		{
			DEBUG_MSG( "App::handleKeyDown: reload failed.\n" );
			clientChatMsg( "App: Script reload failed." );

			if (PyErr_Occurred())
			{
				PyErr_PrintEx(0);
				PyErr_Clear();
			}
		}
		break;
	}
#if UMBRA_ENABLE
	case KeyCode::KEY_O:
	{
		// Toggle umbra occlusion culling on/off using the watchers
		if (ChunkManager::instance().umbra()->occlusionCulling())
		{
			ChunkManager::instance().umbra()->occlusionCulling(false);
			clientChatMsg( "Umbra occlusion culling disabled" );
		}
		else
		{
			ChunkManager::instance().umbra()->occlusionCulling(true);
			clientChatMsg( "Umbra occlusion culling enabled" );
		}
		break;
	}
#endif

	case KeyCode::KEY_5:
	case KeyCode::KEY_H:
		{
			AvatarFilterHelper::isActive( !AvatarFilterHelper::isActive() );
			BW::string s = "App: AvatarFilter is ";
			s.append(AvatarFilterHelper::isActive() ? "on" : "off");
			clientChatMsg(s);
		}
		break;

	case KeyCode::KEY_I:
		{
			// Invert mouse verticals for DirectionCursor.
			CameraControl::isMouseInverted(
				!CameraControl::isMouseInverted() );
			BW::string s = "App: Mouse vertical movement ";
			s.append(CameraControl::isMouseInverted() ? "Inverted" : "Normal");
			clientChatMsg(s);
		}
		break;

	case KeyCode::KEY_P:
		if (!event.isCtrlDown())
		{
			// Toggle python console
			ConsoleManager::instance().toggle( "Python" );
		}
#if ENABLE_MSG_LOGGING
		else
		{
			// Toggle log console
			ConsoleManager::instance().toggle( "Log" );
		}
#endif // ENABLE_MSG_LOGGING
		break;

	case KeyCode::KEY_L:
		{
			// Toggle the static sky dome
			CanvasApp::instance.drawSkyCtrl_ =
				CanvasApp::instance.drawSkyCtrl_ ^
				EnviroMinder::DRAW_SKY_BOXES;
			clientChatMsg( "toggle: sky boxes" );
		}
		break;

	case KeyCode::KEY_LBRACKET:
	{
		if (DeprecatedSpaceHelpers::cameraSpace() != NULL)
		{
			EnviroMinder & enviro =
				DeprecatedSpaceHelpers::cameraSpace()->enviro();

			if (event.isShiftDown() )
			{
				// Move back an hour.
				enviro.timeOfDay()->gameTime(
					enviro.timeOfDay()->gameTime() - 1.0f );
				clientChatMsg( "Move backward one hour to " +
					enviro.timeOfDay()->getTimeOfDayAsString() );
			}
			else
			{
				// Move back by 10 minutes.
				enviro.timeOfDay()->gameTime(
					enviro.timeOfDay()->gameTime() - 10.0f/60.0f );
				clientChatMsg( "Move backward 10 minutes to " +
					enviro.timeOfDay()->getTimeOfDayAsString() );
			}
		}
		break;
	}


	case KeyCode::KEY_RBRACKET:
	{
		if (DeprecatedSpaceHelpers::cameraSpace() != NULL)
		{
			EnviroMinder & enviro =
				DeprecatedSpaceHelpers::cameraSpace()->enviro();

			if (event.isShiftDown() )
			{
				// Move forward an hour.
				enviro.timeOfDay()->gameTime(
					enviro.timeOfDay()->gameTime() + 1.0f );
				clientChatMsg( "Move forward one hour to " +
					enviro.timeOfDay()->getTimeOfDayAsString() );
			}
			else
			{
				// Move forward 10 minutes.
				enviro.timeOfDay()->gameTime(
					enviro.timeOfDay()->gameTime() + 10.0f/60.0f );
				clientChatMsg( "Move forward 10 minutes to " +
					enviro.timeOfDay()->getTimeOfDayAsString() );
			}
		}
		break;
	}

	case KeyCode::KEY_F5:
		if (!event.isCtrlDown())
		{
			ConsoleManager::instance().toggle( "Statistics" );
		}
		else
		{
			ConsoleManager::instance().toggle( "Special" );
		}
		break;
	case KeyCode::KEY_F7:
		if (!event.isCtrlDown())
		{
			ConsoleManager::instance().toggle( "Watcher" );
		}
		else
		{
			ParticleSystemManager::instance().active( !ParticleSystemManager::instance().active() );
		}
		break;

	case KeyCode::KEY_JOYB:
		ConsoleManager::instance().toggle( "Statistics" );
		break;
	case KeyCode::KEY_JOYX:
		ConsoleManager::instance().toggle( "Watcher" );
		break;
	case KeyCode::KEY_JOYY:
		ConsoleManager::instance().toggle( "Python" );
		break;
	case KeyCode::KEY_JOYLTRIGGER:
		if (InputDevices::isKeyDown( KeyCode::KEY_JOYRTRIGGER ))
		{
			DEBUG_MSG( "Reloading entity script classes...\n" );

			if (EntityType::reload())
			{
				DEBUG_MSG( "Script reload successful!\n" );
				clientChatMsg( "App: Script reload succeeded." );
			}
			else
			{
				DEBUG_MSG( "Script reload failed.\n" );
				clientChatMsg( "App: Script reload failed." );

				if (PyErr_Occurred())
				{
					PyErr_PrintEx(0);
					PyErr_Clear();
				}
			}
		}
		else
		{
			handled = false;
		}
		break;
	case KeyCode::KEY_JOYDUP:
	{
		EnviroMinder & enviro =
			DeprecatedSpaceHelpers::cameraSpace()->enviro();
		enviro.timeOfDay()->gameTime(
			enviro.timeOfDay()->gameTime() + 0.5f );
		break;
	}
	case KeyCode::KEY_JOYDDOWN:
	{
		EnviroMinder & enviro =
			DeprecatedSpaceHelpers::cameraSpace()->enviro();
		enviro.timeOfDay()->gameTime(
			enviro.timeOfDay()->gameTime() - 0.5f );
		break;
	}
	case KeyCode::KEY_JOYARPUSH:
	case KeyCode::KEY_F:	// F for flush
	{
		reloadChunks();
		clientChatMsg( "Reloading all chunks" );
		break;
	}
	case KeyCode::KEY_J: // J for Json dump
	{
#if ENABLE_PROFILER
		// unload all the visible chunks and time how long it takes to load
		// them in again.
		static bool jsonDump = false;
		jsonDump = !jsonDump; 
		g_profiler.setForceJsonDump( jsonDump );
		if (jsonDump)
		{
			g_profiler.setProfileMode( Profiler::SORT_BY_TIME, false );
			ConsoleManager::instance().find( "Python" )->print( "Continuous JSON dump ON\n" );
		}
		else
		{
			ConsoleManager::instance().find( "Python" )->print( "Continuous JSON dump OFF\n" );
			g_profiler.setProfileMode( Profiler::PROFILE_OFF, false );
		}
#endif
		break;

	}
	default:
		handled = false;
	}

	return handled;

#endif // ENABLE_CONSOLES
}

/**
 *	Checks to see if the given key belongs to a debug key combination
 *	and also checks if the other keys in the combination are currently down.
 */
bool App::checkDebugKeysState( KeyCode::Key key )
{
	for( size_t i = 0; i < debugKeys_.size(); i++ )
	{
		KeyCode::KeyArray& keys = debugKeys_[i];

		bool keyInCombo = false;
		bool otherKeysDown = true;
		for( size_t k = 0; k < keys.size(); k++ )
		{
			if ( key == keys[k] )
			{
				keyInCombo = true;
			}
			else if ( !InputDevices::isKeyDown( keys[k] ) )
			{
				otherKeysDown = false;
				break;
			}
		}

		// If this is true and we got here, then all other keys in 
		// the combination are currently pressed.
		if (keyInCombo && otherKeysDown)
		{
			return true;
		}
	}

	return false;
}

/**
 *	Checks to see if the given key belongs to any debug key combo.
 */
bool App::isDebugKey( KeyCode::Key key )
{
	for( size_t i = 0; i < debugKeys_.size(); i++ )
	{
		KeyCode::KeyArray& keys = debugKeys_[i];
		for( size_t k = 0; k < keys.size(); k++ )
		{
			if (keys[k] == key)
			{
				return true;
			}
		}
	}

	return false;
}


/**
 *	This method overrides the InputHandler method to handle mouse events.
 *
 *	@param event	The current mouse event.
 *
 *	@return			True if the event was handled, false otherwise.
 */
bool App::handleMouseEvent( const MouseEvent & event )
{
	BW_GUARD;

	//First give the personality script a go
	// We give the personality script any movements
	ScriptObject ret = Personality::instance().callMethod(
		"handleMouseEvent",
		ScriptArgs::create( event ),
		ScriptErrorPrint( "Personality handleMouseEvent: "),
		/*allowNullMethod*/ false );

	if (ret && ret.isTrue( ScriptErrorPrint(
		"Personality Script handleMouseEvent retval" )))
	{
		return true;
	}

	// And finally the active cursor gets its turn
	return activeCursor_ && activeCursor_->handleMouseEvent( event );
}



/**
 *	This method overrides the InputHandler method to joystick axis events.
 *
 *	@return			True if the event was handled, false otherwise.
 */
bool App::handleAxisEvent( const AxisEvent & event )
{
	BW_GUARD;
	bool handled = false;

#if ENABLE_CONSOLES
	// The debug consoles get in first
	if (!handled)
	{
		handled = ConsoleManager::instance().handleAxisEvent( event );
	}
#endif // ENABLE_CONSOLES
	// Now give the personality script a go, if it ever needs this
	if (!handled)
	{
		ScriptObject ret = Personality::instance().callMethod( "handleAxisEvent",
			ScriptArgs::create( event ),
			ScriptErrorPrint( "Personality handleAxisEvent: " ),
			/*allowNullMethod*/ false );

		handled = ret && ret.isTrue( ScriptErrorPrint(
				"Personality handleAxisEvent retval" ) );
	}

	// And finally the active cursor gets its turn
	if (!handled && activeCursor_ != NULL)
	{
		handled = activeCursor_->handleAxisEvent( event );
	}

	// Physics gets anything that's left
	if (!handled)
	{
		handled = Physics::handleAxisEventAll( event );
	}

	return handled;
}


/**
 *	Returns the current active cursor.
 */
InputCursorPtr App::activeCursor()
{
	return this->activeCursor_;
}


/**
 *	Sets the active cursor. The active cursor will get all mouse,
 *	keyboard and axis events forwarded to it. Only one cursor can
 *	be active at a time. The deactivate method will be called on
 *	the current active cursor (the one being deactivated), if any.
 *	The activate method will be called on the new active cursor.
 *	if any.
 *
 *	@param	cursor	the new active cursor, or NULL to simply
 *					deactivate the current one.
 *
 */
void App::activeCursor( InputCursor * cursor )
{
	BW_GUARD;
	if (this->activeCursor_)
	{
		this->activeCursor_->deactivate();
	}

	this->activeCursor_ = cursor;

	if (this->activeCursor_)
	{
		this->activeCursor_->activate();
		if ( InputDevices::instance().hasFocus() )
			activeCursor_->focus( true );
	}
}


/*
 *	Override from DebugMessageCallback.
 */
bool App::handleMessage(
		   DebugMessagePriority messagePriority, const char * /*pCategory*/,
		   DebugMessageSource messageSource, const LogMetaData & /*metaData*/,
		   const char * pFormat, va_list argPtr )
{
	if (messagePriority != MESSAGE_PRIORITY_ASSET)
	{
		return false;
	}

	DataSectionPtr configSection = AppConfig::instance().pRoot();
	if (configSection)
	{
		DataSectionPtr pSection = configSection->openSection( "debug/enableLoggingAssetMsg" );
		if (pSection)
		{
			if (!pSection->asBool( false ))
			{
				return false;
			}

			char buffer[1024];
			const int written = 
				bw_vsnprintf( buffer, sizeof(buffer), pFormat, argPtr );

			buffer[ sizeof(buffer)-1 ] = NULL;

			BW::string prefix = messagePrefix( messagePriority );
			BW::string fullMsg = prefix + BW::string( ": " ) + buffer;

			BW::string assetLogPath = configSection->readString( "debug/assetLogPath", "asset.log" );
			FILE* assetLogFile = bw_fopen( assetLogPath.c_str(), "a" );

			if (assetLogFile != NULL)
			{
				fprintf( assetLogFile, "%s", fullMsg.c_str() );
				fclose( assetLogFile );
			}
			else
			{
				ERROR_MSG( "Asset message handler: "
					"Could not open '%s'\n",
					assetLogPath.c_str() );
			}
		}

	}
	//Always return false since we don't want to break the handling sequence.
	return false;
}

bool App::savePreferences()
{
	BW_GUARD;
	return DeviceApp::instance.savePreferences();
}

// -----------------------------------------------------------------------------
// Section: Window Events
// -----------------------------------------------------------------------------


/**
 *	This method is called when the window has resized. It is called in response
 *	to the Windows event.
 */
void App::resizeWindow( void )
{
	BW_GUARD;
	if( Moo::rc().windowed() == true )
	{
		Moo::rc().resetDevice();
	}
}


void App::resizeWindow(int width, int height)
{
	BW_GUARD;

	if (Moo::rc().device() == NULL || Moo::rc().windowed())
	{
		RECT rect;

		GetWindowRect( hWnd_, &rect );

		rect.right = rect.left + width;
		rect.bottom = rect.top + height;

		AdjustWindowRectEx( &rect, GetWindowLong( hWnd_, GWL_STYLE ),
			FALSE, GetWindowLong( hWnd_, GWL_EXSTYLE ) );

		if (rect.left < 0)
		{
			rect.right -= rect.left;
			rect.left = 0;
		}

		if (rect.top < 0)
		{
			rect.bottom -= rect.top;
			rect.top = 0;
		}

		MoveWindow( hWnd_, rect.left, rect.top, rect.right - rect.left,
			rect.bottom - rect.top, true );
	}
}

const POINT App::windowSize() const
{
	BW_GUARD;
	POINT result;
	result.x = LONG(Moo::rc().screenWidth());
	result.y = LONG(Moo::rc().screenHeight());
	return result;
}

/**
 *	This method is called when the window has moved. It is called in response to
 *	the Windows event and is important for DirectX.
 */
void App::moveWindow( int16 x, int16 y )
{
	BW_GUARD;
	//Moo::rc().device()->moveFrame( x, y );
}


/**
 *	This method is called when the application gets the focus.
 */
/*static*/ void App::handleSetFocus( bool state )
{
	BW_GUARD;
	DEBUG_MSG("App::handleSetFocus: %d\n", state);
	InputDevices::setFocus( state, DeviceApp::instance.pInputHandler_ );

	SimpleGUI* gui = SimpleGUI::pInstance();
	if ( gui )
	{
		gui->appFocus( state );
	}

	if (pInstance_ != NULL && pInstance_->activeCursor_ != NULL)
	{
		pInstance_->activeCursor_->focus( state );
	}

	MouseCursor::staticSetFocus( state );

	//Check that either there is no SimpleGUI yet
	if (SimpleGUI::pInstance() == NULL)
	{
		Moo::rc().restoreCursor(!state);
	}

	if (Moo::rc().device() && !Moo::rc().windowed() && !desktopLocked())
	{
		Moo::rc().changeMode( Moo::rc().modeIndex(), true );
	}
}


/**
 *	This function sets (or clears, if the input string is empty)
 *	the title note identified by 'pos'
 */
void App::setWindowTitleNote( int pos, const BW::string & note )
{
	BW_GUARD;
	if (note.empty())
	{
		TitleNotes::iterator it = titleNotes_.find( pos );
		if (it != titleNotes_.end()) titleNotes_.erase( it );
	}
	else
	{
		titleNotes_[ pos ] = note;
	}

	BW::string newTitle;
	bw_wtoutf8( APP_TITLE, newTitle );
	if (!titleNotes_.empty())
	{
		int n = 0;
		for (TitleNotes::iterator it = titleNotes_.begin();
			it != titleNotes_.end();
			it++, n++)
		{
			newTitle += n ? ", " : " [";
			newTitle += it->second;
		}
		newTitle += "]";
	}

	BW::wstring wnewTitle;
	bw_utf8tow( newTitle, wnewTitle );
	SetWindowText( hWnd_, wnewTitle.c_str() );
}


// -----------------------------------------------------------------------------
// Section: Statistics and Debugging
// -----------------------------------------------------------------------------

/**
 *	This method calculates the time between this frame and last frame.
 */
void App::calculateFrameTime()
{
	BW_GUARD;
	uint64 thisTime = frameTimerValue();

	// Render time is wall-clock time.
	FRAME_TIMER_TYPE deltaRenderTime =
		FRAME_TIMER_TYPE( thisTime - frameStartRenderTime_ );
	dRenderTime_ = float( deltaRenderTime / frameTimerFreq() );
	frameStartRenderTime_ = thisTime;

	ConnectionControl & connectionControl = ConnectionControl::instance();

	BWServerConnection * pServerConn = connectionControl.pServerConnection();
	BWNullConnection * pNullConn = connectionControl.pNullConnection();
	BWReplayConnection * pReplayConn = connectionControl.pReplayConnection();

	dGameTime_ = dRenderTime_;

	if (pServerConn != NULL)
	{
		// If we are connected to a network server, we do not control dGameTime_
		// so cannot apply local adjustments.
		return;
	}

	if (pReplayConn != NULL)
	{
		ReplayControllerPtr pReplayController =
			pReplayConn->pReplayController();
		if (pReplayController == NULL)
		{
			// Replay was terminated already. We're in post-replay explore
			// mode, waiting for the script engine to clear the entities.
			return;
		}

		if (pReplayController->isPlaying())
		{
			// If playing a replay, we need to apply the speed scale
			dGameTime_ /= pReplayConn->speedScale();
		}
		else if (!pReplayController->isSeekingToTick())
		{
			// We're paused. Try not to advance time very much...
			// dGameTime_ should not be zero, since we need a non-zero
			// dGameTime_ to calculate entity animation and movement when
			// entities are reloaded on pause.
			dGameTime_ = NONZERO_PAUSE_TIME_DIFFERENCE;
		}
	}
	else
	{
		// We must be in offline mode or not connected at all.
		MF_ASSERT( pNullConn != NULL ||
			connectionControl.pConnection() == NULL )

		// Apply DebugApp's slowTime game time speed scale
		if (DebugApp::instance.slowTime_ > 0.000001f)
		{
			dGameTime_ /= DebugApp::instance.slowTime_;
		}
	}

	// Apply any accumulated time-skip from seeking, after speed scales are
	// applied.
	// This applies to replays in any state and offline mode.
	dGameTime_ += float(accumulatedTimeSkip_);
	accumulatedTimeSkip_ = 0.0;
}


void App::skipGameTimeForward( double dGameTime )
{
	// This increases the dGameTime_ of the next offline, non-paused frame.
	accumulatedTimeSkip_ += dGameTime;
}

/**
 *	This method gets the name of the active console
 */
BW::string App::activeConsole() const
{
#if ENABLE_CONSOLES
	XConsole* con = ConsoleManager::instance().pActiveConsole();
	if (!con)
	{
		return "";
	}


	return ConsoleManager::instance().consoleName( con );
#else
	return "";
#endif // ENABLE_CONSOLES
}

/**
 *	This method sets the active console by name
 */
void App::activeConsole(BW::string v)
{
#if ENABLE_CONSOLES
	if ( !v.empty() )
	{
		ConsoleManager::instance().activate( v.c_str() );		
	}
	else
	{
		ConsoleManager::instance().deactivate();
	}
#endif // ENABLE_CONSOLES
}


// -----------------------------------------------------------------------------
// Section: Miscellaneous App methods
// -----------------------------------------------------------------------------


/**
 *	This method returns the current render time, which is
 *	the wall-clock time since App was instantiated. This is mainly intended for
 *	log time-stamping, anything else should use getRenderTimeFrameStart().
 *	It is unaffected by any slow-down effects from either replays or DebugApp.
 */
double App::getRenderTimeNow() const
{
	FRAME_TIMER_TYPE deltaRenderTime =
		FRAME_TIMER_TYPE( frameTimerValue() - appStartRenderTime_ );
	return double( deltaRenderTime / frameTimerFreq() );
}


/**
 *	This method returns the render time of the current frame's render, which is
 *	the wall-clock time between when the App was instantiated and the current
 *	frame was started. Most things not related to the simulation should be
 *	referring to this.
 *	It is unaffected by any slow-down effects from either replays or DebugApp.
 */
double App::getRenderTimeFrameStart() const
{
	FRAME_TIMER_TYPE deltaRenderTime =
		FRAME_TIMER_TYPE( frameStartRenderTime_ - appStartRenderTime_ );
	return double( deltaRenderTime / frameTimerFreq() );
}


/**
 *	This method returns the current game time, which is a monotonic time
 *	against which events in the game world are scheduled.
 *	Reply playback speed scaling and DebugApp speed scaling affect the speed
 *	at which this increases.
 */
double App::getGameTimeFrameStart() const
{
	// SmartServerConnection provides this monotonic clock already.
	return ConnectionControl::instance().gameTime();
}


/// Make sure the Python object ring hasn't been corrupted
void App::checkPython()
{
	BW_GUARD;	
#ifdef Py_DEBUG
	PyObject* head = PyInt_FromLong(1000000);
	PyObject* p = head;

	INFO_MSG("App::checkPython: checking python...\n");

	while(p && p->_ob_next != head)
	{
		if((p->_ob_prev->_ob_next != p) || (p->_ob_next->_ob_prev != p))
		{
			CRITICAL_MSG("App::checkPython: Python object %p is screwed\n", p);
		}
		p = p->_ob_next;
	}

	Py_DECREF(head);
	INFO_MSG("App::checkPython: done..\n");
#endif
}

/*
	Check if the client is quitting now
*/
bool App::isQuiting()
{
	return quiting_;
}

void App::setQuiting( bool quiting )
{
	quiting_ = quiting;
}



/**
 *	Returns whether or not the camera is outside
 */
bool isCameraOutside()
{
	BW_GUARD;
	Chunk * pCC = ChunkManager::instance().cameraChunk();
	return pCC == NULL || pCC->isOutsideChunk();
}

/**
 *	Returns whether or not the player is outside
 */
bool isPlayerOutside()
{
	BW_GUARD;
	Entity * pPlayer;
	if ((pPlayer = ScriptPlayer::entity()) == NULL) return true;

	return pPlayer->pPrimaryEmbodiment() == NULL ||
		pPlayer->pPrimaryEmbodiment()->isOutside();
}

/*~ function BigWorld.quit
 *  Ask the application to quit.
 */
/**
 *	Ask the app to quit
 */
static void quit()
{
	BW_GUARD;
	App::instance().quit();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, quit, END, BigWorld )

/*~ function BigWorld.playMovie
 *	@components{ client }
 *
 *	Placeholder for deprecated functionality.
 */
static void playMovie()
{
	//not on PC
}
PY_AUTO_MODULE_FUNCTION( RETVOID, playMovie, END, BigWorld )




/*~ function BigWorld timeOfDay
 *  Gets and sets the time of day in 24 hour time, as used by the environment
 *  system. If the camera is not currently in a space, then this function will
 *	not do anything, and will return an empty string.
 *
 *  This can also be changed manually via the following key combinations:
 *
 *  DEBUG + "[":			Rewind time of day by 10 minutes
 *
 *  DEBUG + Shift + "[":	Rewind time of day by 1 hour
 *
 *  DEBUG + "]":			Advance time of day by 10 minutes
 *
 *  DEBUG + Shift + "]":	Advance time of day by 1 hour
 *
 *  @param time Optional string. If provided, the time of day is set to this.
 *	This can be in the format "hour:minute" (eg "01:00", "1:00", "1:0", "1:"
 *  ), or as a string representation of a float (eg "1", "1.0"). Note that an
 *  incorrectly formatted string will not throw an exception, nor will it
 *  change the time of day.
 *  @return A string representing the time of day at the end of the function
 *  call in the form "hh:mm" (eg "01:00" ). Returns an empty string if the
 *	camera is not currently in a space.
 */
/**
 *	Function to let scripts set the time of day
 */
const BW::string & timeOfDay( const BW::string & tod )
{
	BW_GUARD;
	ClientSpacePtr cameraSpace = DeprecatedSpaceHelpers::cameraSpace();
	if (!cameraSpace.exists())
	{
		static const BW::string empty;
		return empty;
	}

	EnviroMinder & enviro = cameraSpace->enviro();
	if (tod.size())
	{
		enviro.timeOfDay()->setTimeOfDayAsString(tod);
	}
	return enviro.timeOfDay()->getTimeOfDayAsString();
}
PY_AUTO_MODULE_FUNCTION(
	RETDATA, timeOfDay, OPTARG( BW::string, "", END ), BigWorld )

/*~ function BigWorld.spaceTimeOfDay
 *  Gets and sets the time of day in 24 hour time, as used by the environment
 *  system.
 *	@param spaceID The spaceID of the space to set/get the time.
 *  @param time Optional string. If provided, the time of day is set to this.
 *	This can be in the format "hour:minute" (eg "01:00", "1:00", "1:0", "1:"
 *  ), or as a string representation of a float (eg "1", "1.0"). Note that an
 *  incorrectly formatted string will not throw an exception, nor will it
 *  change the time of day.
 *  @return A string representing the time of day at the end of the function
 *  call in the form "hh:mm" (eg "01:00" ).
 */
/**
 *	Function to let scripts set the time of day for a given space.
 */
const BW::string & spaceTimeOfDay( SpaceID spaceID, const BW::string & tod )
{
	BW_GUARD;
	ClientSpacePtr pSpace = SpaceManager::instance().space( spaceID );

	if ( pSpace )
	{
		EnviroMinder & enviro = pSpace->enviro();
		if (tod.size())
		{
			enviro.timeOfDay()->setTimeOfDayAsString(tod);
		}

		return enviro.timeOfDay()->getTimeOfDayAsString();
	}

	static BW::string s_nullSpaceTime = "00:00";
	return s_nullSpaceTime;
}
PY_AUTO_MODULE_FUNCTION(
	RETDATA, spaceTimeOfDay, ARG( SpaceID, OPTARG( BW::string, "", END )), BigWorld )



/*~	attribute BigWorld.platform
 *
 *	This is a string that represents which platform the client is running on,
 *	for example, "windows".
 */
/// Add the platform name attribute
PY_MODULE_ATTRIBUTE( BigWorld, platform, Script::getData( "windows" ) )

BW_END_NAMESPACE

// app.cpp

