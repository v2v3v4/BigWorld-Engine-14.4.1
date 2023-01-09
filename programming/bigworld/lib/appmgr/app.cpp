/**
 *
 * The main application class
 */

#include "pch.hpp"
#include "app.hpp"

#include "ashes/simple_gui.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/timestamp.hpp"
#include "cstdmf/profiler.hpp"
#include "cstdmf/watcher.hpp"
#include "cstdmf/string_utils.hpp"

#include "moo/render_context.hpp"
#include "moo/animating_texture.hpp"
#include "moo/effect_visual_context.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/xml_section.hpp"

#include "romp/console_manager.hpp"
#include "romp/engine_statistics.hpp"
#include "moo/geometrics.hpp"

#include "moo/effect_visual_context.hpp"
#include "moo/draw_context.hpp"

#include "romp/flora.hpp"
#include "romp/enviro_minder.hpp"

#include "dev_menu.hpp"
#include "module_manager.hpp"
#include "options.hpp"

DECLARE_DEBUG_COMPONENT2( "App", 0 );

BW_BEGIN_NAMESPACE

extern void setupTextureFeedPropertyProcessors();

static AutoConfigString s_blackTexture( "system/blackBmp" );
static AutoConfigString s_notFoundTexture( "system/notFoundBmp" );
static AutoConfigString s_notFoundModel( "system/notFoundModel" );	
static AutoConfigString s_graphicsSettingsXML("system/graphicsSettingsXML");
static AutoConfigString s_engineConfigXML("system/engineConfigXML");

static DogWatch g_watchDrawConsole( "Draw Console" );
static DogWatch g_watchPreDraw( "Predraw" );

static DogWatch g_watchEndScene( "EndScene" );
static DogWatch g_watchPresent( "Present" );

// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	The constructor for the App class. App::init needs to be called to fully
 *	initialise the application.
 *
 *	@see App::init
 */
App::App() :
	hWndApp_( NULL ),
	lastTime_(0),
    paused_(false),
	maxTimeDelta_( 0.5f ),
	timeScale_( 1.f ),
	bInputFocusLastFrame_( false )
{
#if ENABLE_PROFILER
	g_profiler.init(12*1024*1024);
#endif // ENABLE_PROFILER
}


/**
 *	Destructor.
 */
App::~App()
{
#if ENABLE_PROFILER
	g_profiler.fini();
#endif
}

/**
 *	This method initialises the application.
 *
 *	@param hInstance	The HINSTANCE associated with this application.
 *	@param hWndApp		The main window associated with this application.
 *	@param hWndGraphics	The window associated with the 3D device
 *
 *	@return		True if the initialisation succeeded, false otherwise.
 */
bool App::init( HINSTANCE hInstance, HWND hWndApp, HWND hWndGraphics,
			   IMainFrame * mainFrame,
			   bool ( *userInit )( HINSTANCE, HWND, HWND ) )
{
#if ENABLE_FILE_CASE_CHECKING
	// Optionally enable filename case checking:
	bool checkFileCase = Options::getOptionBool( "checkFileCase", false );
	BWResource::checkCaseOfPaths( checkFileCase );
#endif//ENABLE_FILE_CASE_CHECKING

	CStdMf::checkUnattended();

	BgTaskManager::init();
	FileIOTaskManager::init();
	ConsoleManager::createInstance();

	hWndApp_ = hWndApp;
	maxTimeDelta_ = Options::getOptionFloat( "app/maxTimeDelta", 
		maxTimeDelta_ );
	timeScale_ = Options::getOptionFloat( "app/timeScale", timeScale_ );
	
	bool darkBackground = Options::getOptionBool( "consoles/darkenBackground", false );
	ConsoleManager::instance().setDarkenBackground(darkBackground);

	MF_WATCH( "App/maxTimeDelta", maxTimeDelta_, Watcher::WT_READ_WRITE, "Limits the delta time passed to application tick methods." );
	MF_WATCH( "App/timeScale", timeScale_, Watcher::WT_READ_WRITE, "Multiplies the delta time passed to application tick methods." );


	// Resource glue initialisation
	if (!AutoConfig::configureAllFrom("resources.xml"))
	{
		CRITICAL_MSG( "Unable to find the required file resources.xml at the root of your resource tree.\n"
			"Check whether either the \"BW_RES_PATH\" environmental variable or the \"paths.xml\" files are incorrect." );
		return false;
	}

	//init the texture feed instance, this registers material section
	//processors.
	setupTextureFeedPropertyProcessors();

	//Initialise the GUI
	SimpleGUI::init( NULL );

	//userInit must be called before loading any configs, 
	//otherwise the saved configs may be overwritten.
	//Bug: BWWOT-156
	if (userInit)
	{
		if ( !userInit( hInstance, hWndApp, hWndGraphics ) )
			return false;
	}

	//Load the "engine_config.xml" file...
	DataSectionPtr configRoot = BWResource::instance().openSection( s_engineConfigXML.value() );
	if (configRoot)
	{
		XMLSection::shouldReadXMLAttributes( configRoot->readBool( 
			"shouldReadXMLAttributes", XMLSection::shouldReadXMLAttributes() ));
		XMLSection::shouldWriteXMLAttributes( configRoot->readBool( 
			"shouldWriteXMLAttributes", XMLSection::shouldWriteXMLAttributes() ));
	}
	else
	{
		INFO_MSG( "App::init: Couldn't open \"%s\"\n", s_engineConfigXML.value().c_str() );
	}

	DataSectionPtr pSection = BWResource::instance().openSection( s_graphicsSettingsXML );

	// setup far plane options
	DataSectionPtr farPlaneOptionsSec = pSection->openSection("farPlane");
	EnviroMinderSettings::instance().init( farPlaneOptionsSec );

	// setup far flora options
	DataSectionPtr floraOptionsSec = pSection->openSection("flora");
	FloraSettings::instance().init( floraOptionsSec );

	if (!ModuleManager::instance().init(
		BWResource::openSection( "resources/data/modules.xml" ) ))
	{
		return false;
	}

	//Check for startup modules
	DataSectionPtr spSection =
		BWResource::instance().openSection( "resources/data/modules.xml/startup");

	BW::vector< BW::string > startupModules;

	if ( spSection )
	{
		spSection->readStrings( "module", startupModules );
	}

	//Now, create all startup modules.
	BW::vector<BW::string>::iterator it = startupModules.begin();
	BW::vector<BW::string>::iterator end = startupModules.end();

	while (it != end)
	{
		if (!ModuleManager::instance().push( (*it) ))
		{
			ERROR_MSG( "Unable to create module: %s",(*it).c_str() );
		}
		else
		{
			ModuleManager::instance().currentModule()->setApp( this );
			ModuleManager::instance().currentModule()->setMainFrame( mainFrame );
		}
		++it;
	}


	// enable file monitoring
	BWResource::instance().enableModificationMonitor( true );

#if ENABLE_GPU_PROFILER
	Moo::GPUProfiler::instance().init();
#endif

	//BUG 4252 FIX: Make sure we start up with focus to allow navigation of view.
	handleSetFocus( true );

	return true;
}


/**
 *	This method finalises the application.
 */
void App::fini( void (*userFini)() )
{
#if ENABLE_GPU_PROFILER
	Moo::GPUProfiler::instance().fini();
#endif

	ModuleManager::instance().popAll();

	SimpleGUI::fini();	

	if (userFini)
	{
		userFini();
	}

	DogWatchManager::fini();

	ConsoleManager::deleteInstance();
	BgTaskManager::fini();
	FileIOTaskManager::fini();
#if ENABLE_WATCHERS
	Watcher::fini();
#endif//ENABLE_WATCHERS
}

// -----------------------------------------------------------------------------
// Section: Framework
// -----------------------------------------------------------------------------

/**
 *	This method calculates the time between this frame and last frame.
 */
float App::calculateFrameTime()
{
	uint64 thisTime = timestamp();

	float dTime = (float)(((int64)(thisTime - lastTime_)) / stampsPerSecondD());

	if (dTime > maxTimeDelta_)
	{
		dTime = maxTimeDelta_;
	}

	dTime *= timeScale_;
	lastTime_ = thisTime;

    if (paused_)
        dTime = 0.0f;

	return dTime;
}

/**
 *  This method allows pausing.  The time between pausing and unpausing is,
 *  as far as rendering is concerned, zero.
 */
void App::pause(bool paused)
{
	static uint64 s_myDtime = 0;
    // If we are unpausing, update the current time so that there isn't a 
    // 'jump'.
    if ((paused != paused_) && !paused)
    {
        lastTime_ = timestamp() - s_myDtime;
    }
	else if ((paused != paused_) && paused)
	{
		uint64 thisTime = timestamp();
		s_myDtime = thisTime - lastTime_;
	}
    paused_ = paused;
}

/**
 *  Are we paused?
 */
bool App::isPaused() const
{
    return paused_;
}

/**
 *	This method is called once per frame to do all the application processing
 *	for that frame.
 */
void App::updateFrame( bool tick )
{
#if ENABLE_PROFILER
	g_profiler.setScreenWidth(Moo::rc().screenWidth());
	g_profiler.tick();
#endif // ENABLE_PROFILER

	g_watchPreDraw.start();

	BWResource::instance().flushModificationMonitor();
	
	float dTime = this->calculateFrameTime();
	if (!tick) dTime = 0.f;

	if ( InputDevices::hasFocus() )
	{
		if ( bInputFocusLastFrame_ )
		{
			InputDevices::processEvents( this->inputHandler_ );
		}
		else
		{
			InputDevices::consumeInput();
			bInputFocusLastFrame_ = true;
		}
	}
	else bInputFocusLastFrame_ = false;

	EngineStatistics::instance().tick( dTime );
	Moo::AnimatingTexture::tick( dTime );
	Moo::Material::tick( dTime );
	Moo::rc().effectVisualContext().tick( dTime );
	
	ModulePtr pModule = ModuleManager::instance().currentModule();

	g_watchPreDraw.stop();
	
	if (pModule)
	{
		g_watchPreDraw.start();
        // If the device cannot be used don't bother rendering
	    if (!Moo::rc().checkDevice())
	    {
		    Sleep(100);
			g_watchPreDraw.stop();
		    return;
	    }

		pModule->updateFrame( dTime );

		HRESULT hr = Moo::rc().beginScene();
        if (SUCCEEDED(hr))
        {
		    if (Moo::rc().mixedVertexProcessing())
			    Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );
		    Moo::rc().nextFrame();

			g_watchPreDraw.stop();

			// The frameTimestamp is already incremented above
			pModule->updateAnimations();

		    pModule->render( dTime );

		    g_watchDrawConsole.start();
		    if (ConsoleManager::instance().pActiveConsole() != NULL)
		    {
			    ConsoleManager::instance().draw( dTime );
		    }
		    g_watchDrawConsole.stop();
    		
		    // End rendering
		    g_watchEndScene.start();
		    Moo::rc().endScene();
		    g_watchEndScene.stop();
        }
		else
		{
			g_watchPreDraw.stop();
		}

		g_watchPresent.start();
		presenting( true );

		if (Moo::rc().present() != D3D_OK)
		{
			DEBUG_MSG( "An error occured while presenting the new frame.\n" );
		}

		presenting( false );
		g_watchPresent.stop();
	}
	else
	{
		//TEMP! WindowsMain::shutDown();
	}
}

/**
 *	flush input queue
 */

void App::consumeInput()
{
	InputDevices::consumeInput();
}

/**
 *	This method is called when the window has resized. It is called in response
 *	to the Windows event.
 */
void App::resizeWindow()
{
	if (Moo::rc().windowed())
	{
		Moo::rc().changeMode( Moo::rc().modeIndex(), Moo::rc().windowed() );
	}
}


/**
 *	This method is called when the application gets the focus.
 */
void App::handleSetFocus( bool state )
{
	//DEBUG_MSG( "App::handleSetFocus: %d\n", state );

	bool isMouseDown = (GetAsyncKeyState( VK_LBUTTON ) < 0 ||
						GetAsyncKeyState( VK_MBUTTON ) < 0 ||
						GetAsyncKeyState( VK_RBUTTON ) < 0);
	if (state || !isMouseDown)
	{
		// Set the input focus IF we are getting the focus, OR if we are losing
		// the focus but all mouse buttons are up (not dragging anything).
		InputDevices::setFocus( state, &this->inputHandler_ );
	}
}


bool directoryExists( const char* dir )
{
	// check that the directory actually exists
	DWORD dwAttrib = ::GetFileAttributesA(dir);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

BW_END_NAMESPACE
