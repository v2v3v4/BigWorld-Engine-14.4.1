#include "pch.hpp"
#include "worldeditor/framework/initialisation.hpp"
#ifndef CODE_INLINE
#include "worldeditor/framework/initialisation.ipp"
#endif
#include "worldeditor/scripting/world_editor_script.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "appmgr/module_manager.hpp"
#include "appmgr/options.hpp"
#include "asset_pipeline/asset_client.hpp"
#include "input/input.hpp"
#include "moo/effect_manager.hpp"
#include "moo/init.hpp"
#include "moo/render_context.hpp"
#include "moo/graphics_settings.hpp"
#include "physics2/material_kinds.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "romp/console_manager.hpp"
#include "romp/console.hpp"
#include "romp/engine_statistics.hpp"
#include "romp/font_manager.hpp"
#include "romp/resource_statistics.hpp"
#include "romp/sky_light_map.hpp"
#include "romp/frame_logger.hpp"
#include "romp/lens_effect_manager.hpp"
#include "post_processing/manager.hpp"
#include "ashes/simple_gui.hpp"
#include "fmodsound/sound_manager.hpp"
#ifndef BIGWORLD_CLIENT_ONLY
#include "network/endpoint.hpp"
#endif

#include "entitydef/data_description.hpp"

#include "moo/shadow_manager.hpp"
#include "moo/renderer.hpp"


DECLARE_DEBUG_COMPONENT2( "App", 0 )

BW_BEGIN_NAMESPACE

class TextureFeeds;

static AutoConfigString s_engineConfigXML("system/engineConfigXML");
static std::auto_ptr< AssetClient > s_pAssetClient_( NULL );

typedef std::auto_ptr< FontManager > FontManagerPtr;
static FontManagerPtr s_pFontManager;

typedef std::auto_ptr< TextureFeeds > TextureFeedsPtr;
static TextureFeedsPtr s_pTextureFeeds;

typedef std::auto_ptr< PostProcessing::Manager > PostProcessingManagerPtr;
static PostProcessingManagerPtr s_pPostProcessingManager;

typedef std::auto_ptr< LensEffectManager > LensEffectManagerPtr;
static LensEffectManagerPtr s_pLensEffectManager;

HINSTANCE Initialisation::s_hInstance = NULL;
HWND Initialisation::s_hWndApp = NULL;
HWND Initialisation::s_hWndGraphics = NULL;

std::auto_ptr<Renderer> Initialisation::renderer_;

bool Initialisation::inited_ = false;

/**
 *  This method intialises global resources for the application.
 *
 *  @param hInstance    The HINSTANCE variable for the application.
 *  @param hWndApp      The HWND variable for the application's main window.
 *  @param hWndGraphics The HWND variable the 3D device will use.
 *
 *  @return     True if initialisation succeeded, otherwise false.
 */
bool
Initialisation::initApp( HINSTANCE hInstance, HWND hWndApp, HWND hWndGraphics )
{
    BW_GUARD;

    inited_ = false;
    s_hInstance = hInstance;
    s_hWndApp = hWndApp;
    s_hWndGraphics = hWndGraphics;
    s_pAssetClient_.reset( new AssetClient() );
    s_pAssetClient_->waitForConnection();

    initErrorHandling();

    // Initialise the timestamps
    initTiming();

    // Create input devices
    InputDevices * pInputDevices = new InputDevices();

    if (!InputDevices::instance().init( hInstance, hWndGraphics ))
    {
        ERROR_MSG( "Initialisation::initApp: Init inputDevices FAILED\n" );
        return false;
    }

    if (!Initialisation::initGraphics(hInstance, hWndGraphics))
    {
        return false;
    }

#ifndef BIGWORLD_CLIENT_ONLY
    // Init winsock
    initNetwork();
#endif

    if (!Initialisation::initScripts())
    {
        return false;
    }

    // Need to initialise after scripts because it creates PyTextureProviders
    s_pLensEffectManager = LensEffectManagerPtr( new LensEffectManager() );

    if (!MaterialKinds::init())
    {
        ERROR_MSG( "Initialisation::initApp: failed to initialise MaterialKinds\n" );
        return false;
    }

    if (!Initialisation::initConsoles())
    {
        return false;
    }

    // Background task manager:
    BgTaskManager::init();
    BgTaskManager::instance().startThreads( "Init App Thread", 1 );

    FileIOTaskManager::init();
    FileIOTaskManager::instance().startThreads("File IO Thread", 1);

    Initialisation::initSound();

    if ( !WorldManager::instance().init( s_hInstance, s_hWndApp, s_hWndGraphics ) )
    {
        return false;
    }

    inited_ = true;

    return true;
}


/**
 *  This method finalises the application. All stuff done in initApp is undone.
 *  @todo make a better sentence
 */
void Initialisation::finiApp()
{
    BW_GUARD;

    if (!inited_)
    {
        return;
    }

    BgTaskManager::fini();

    WorldManager::instance().fini();

    MaterialKinds::fini();

    Initialisation::finaliseGraphics();

    Initialisation::finaliseScripts();

    delete InputDevices::pInstance();

    DogWatchManager::fini();

#ifndef BIGWORLD_CLIENT_ONLY
    finiNetwork();
#endif

    DataType::fini();

    s_pAssetClient_.reset( NULL );

    DebugFilter::instance().deleteMessageCallback(
        WorldManager::instance().getDebugMessageCallback() );
    DebugFilter::instance().deleteCriticalCallback(
        WorldManager::instance().getCriticalMessageCallback() );
}


/**
 *  This method sets up error handling, by routing error msgs
 *  to our own static function
 *
 *  @see messageHandler
 */
bool Initialisation::initErrorHandling()
{
    BW_GUARD;

    MF_WATCH( "debug/filterThreshold", DebugFilter::instance(),
                MF_ACCESSORS( DebugMessagePriority, DebugFilter,
                    filterThreshold ) );

    DebugFilter::instance().addMessageCallback(
        WorldManager::instance().getDebugMessageCallback() );
    DebugFilter::instance().addCriticalCallback(
        WorldManager::instance().getCriticalMessageCallback() );
    return true;
}

/**
 *  This method set up the high frequency timing used by bigworld
 */
bool Initialisation::initTiming()
{
    BW_GUARD;
    
    // this initialises the internal timers for stamps per second, this can take up to a second
    double d = stampsPerSecondD();

    return true;
}


// -----------------------------------------------------------------------------
// Section: Graphics
// -----------------------------------------------------------------------------

/**
 *  This method initialises the graphics subsystem.
 *
 *  @param hInstance    The HINSTANCE variable for the application.
 *  @param hWnd         The HWND variable for the application's main window.
 *
 * @return true if initialisation went well
 */
bool Initialisation::initGraphics( HINSTANCE hInstance, HWND hWnd )
{
    BW_GUARD;

    // Initialise Moo library.
    if ( !Moo::init() )
        return false;

    // Read render surface options.
    bool fullScreen = false;
    uint32 modeIndex = 0;

    // Read shadow options.
    bool useShadows = Options::getOptionBool( "graphics/shadows", false );

    // initialise graphics settings
    DataSectionPtr graphicsPrefDS = BWResource::openSection(
        Options::getOptionString(
            "graphics/graphicsPreferencesXML",
            "resources/graphics_preferences.xml" ) );
    if ( graphicsPrefDS == NULL || !Moo::GraphicsSetting::init( graphicsPrefDS ) )
    {
        ERROR_MSG("Moo::GraphicsSetting::init()  Could not initialise the graphics settings.\n");
        return false;
    }

    // Look for Nvidia's PerfHUD device, and use that if available.
    uint32 deviceIndex = 0;

    // Uncomment this to enable

    for (uint32 i = 0; i < Moo::rc().nDevices(); i++)
    {
        BW::string description 
            = Moo::rc().deviceInfo(i).identifier_.Description;
        
        if ( description.find("PerfHUD") != BW::string::npos )
        {
            deviceIndex     = i;
            break;
        }
    }

    //-- we need find some place before creating render device but at moment when we've already
    //--     knew all needed info about users machine spec to select right renderer pipeline.
    renderer_.reset(new Renderer());

    bool isAssetProcessor = false;
    bool isSupportedDS    = (Moo::rc().deviceInfo(deviceIndex).compatibilityFlags_ & Moo::COMPATIBILITYFLAG_DEFERRED_SHADING) != 0;
    if (!renderer_->init(isAssetProcessor, isSupportedDS))
    {
        ERROR_MSG("DeviceApp::init()  Could not initialize renderer's pipeline.\n");
        return false;
    }

    // Initialise the directx device with the settings from the options file.
    if (!Moo::rc().createDevice( hWnd, deviceIndex, modeIndex, !fullScreen, 
        useShadows, Vector2(0, 0), false, false
#if ENABLE_ASSET_PIPE
        , s_pAssetClient_.get()
#endif
        ))
    {
        CRITICAL_MSG( "Initialisation:initApp: Moo::rc().createDevice() FAILED\n" );
        return false;
    }


    //We don't need no fog man!
    Moo::FogParams fog = Moo::FogHelper::pInstance()->fogParams();
    fog.m_start = 5000.f;
    fog.m_end = 10000.f;
    Moo::FogHelper::pInstance()->fogParams(fog);

    //ASHES
    SimpleGUI::instance().hwnd( hWnd );
    s_pTextureFeeds = TextureFeedsPtr( new TextureFeeds() );

    s_pPostProcessingManager = 
        PostProcessingManagerPtr( new PostProcessing::Manager() );

    // Hide the 3D window to avoid it turning black from the clear device in
    // the following method
    ::ShowWindow( hWnd, SW_HIDE );

    s_pFontManager = FontManagerPtr( new FontManager() );

    ::ShowWindow( hWnd, SW_SHOW );

    return true;
}


/**
 *  This method finalises the graphics sub system.
 */
void Initialisation::finaliseGraphics()
{
    BW_GUARD;

    s_pLensEffectManager.reset();
    s_pFontManager.reset();
    s_pPostProcessingManager.reset();
    s_pTextureFeeds.reset();

    Moo::VertexDeclaration::fini();

    Moo::rc().releaseDevice();  

    //-- destroy renderer right after device releasing.
    renderer_.reset();

    // save graphics settings
    DataSectionPtr graphicsPrefDS = BWResource::openSection(
        Options::getOptionString(
            "graphics/graphicsPreferencesXML",
            "resources/graphics_preferences.xml" ), true );
    if ( graphicsPrefDS != NULL )
    {
        Moo::GraphicsSetting::write( graphicsPrefDS );
        graphicsPrefDS->save();
    }

    Moo::GraphicsSetting::finiSettingsData();
    // Finalise the Moo library
    Moo::fini();
}



// -----------------------------------------------------------------------------
// Section: Scripts
// -----------------------------------------------------------------------------

/**
 * This method initialises the scripting environment
 *
 * @param hInstance the HINSTANCE variable for the application
 * @param hWnd the HWND variable for the application's main window
 *
 * @return true if initialisation went well
 */
bool Initialisation::initScripts()
{
    BW_GUARD;

    // as a test, nuke the PYTHONPATH
    _putenv( "PYTHONPATH=" );

    if (!WorldEditorScript::init( BWResource::instance().rootSection() ))
    {
        CRITICAL_MSG( "Initialisation::initScripts: WorldEditorScript::init() failed\n" );
        return false;
    }

    return true;
}


/**
 * This method finalises the scripting environment
 */
void Initialisation::finaliseScripts()
{
    BW_GUARD;

    WorldEditorScript::fini();
}


// -----------------------------------------------------------------------------
// Section: Consoles
// -----------------------------------------------------------------------------

/**
 *  This method initialises the consoles.
 */
bool Initialisation::initConsoles()
{
    BW_GUARD;

    // Initialise the consoles
    ConsoleManager & mgr = ConsoleManager::instance();

    XConsole * pStatusConsole = new XConsole();
    XConsole * pStatisticsConsole = new StatisticsConsole( &EngineStatistics::instance() );
    XConsole * pResourceConsole = new ResourceUsageConsole( &ResourceStatistics::instance() );

    mgr.add( new HistogramConsole(),    "Histogram",    KeyCode::KEY_F11 );
    mgr.add( pStatisticsConsole,        "Default",      KeyCode::KEY_F5, MODIFIER_CTRL );
    mgr.add( pResourceConsole,          "Resource",     KeyCode::KEY_F5, MODIFIER_CTRL | MODIFIER_SHIFT );

#if ENABLE_WATCHERS
    mgr.add( new DebugConsole(),        "Debug",        KeyCode::KEY_F7, MODIFIER_CTRL );
#endif//ENABLE_WATCHERS

    mgr.add( new PythonConsole(),       "Python",       KeyCode::KEY_P, MODIFIER_CTRL );
    mgr.add( pStatusConsole,            "Status" );

    pStatusConsole->setConsoleColour( 0xFF26D1C7 );
    pStatusConsole->setScrolling( true );
    pStatusConsole->setCursor( 0, 20 );

    FrameLogger::init();

    return true;
}


// -----------------------------------------------------------------------------
// Section: Miscellaneous
// -----------------------------------------------------------------------------

void Initialisation::initSound()
{
#if FMOD_SUPPORT
    BW_GUARD;

    //Load the "engine_config.xml" file...
    DataSectionPtr configRoot = BWResource::instance().openSection( s_engineConfigXML.value() );
    
    if (!configRoot)
    {
        ERROR_MSG( "Initialisation::initSound: Couldn't open \"%s\"\n", s_engineConfigXML.value().c_str() );
        return;
    }

    if (! configRoot->readBool( "soundMgr/enabled", true ) )
    {
        WARNING_MSG( "Sounds are disabled for the client (set in \"engine_config.xml/soundMrg/enabled\").\n" );
    }

    DataSectionPtr dsp = configRoot->openSection( "soundMgr" );

    if (dsp)
    {
        if (!SoundManager::instance().initialise( dsp ))
        {
            ERROR_MSG( "PeShell::initSound: Failed to initialise sound\n" );
        }
    }
    else
    {
        ERROR_MSG( "PeShell::initSound: "
            "No <soundMgr> config section found, sound support is "
            "disabled\n" );
    }
#endif // FMOD_SUPPORT
}

/**
 *  Output streaming operator for Initialisation.
 */
std::ostream& operator<<(std::ostream& o, const Initialisation& t)
{
    BW_GUARD;

    o << "Initialisation\n";
    return o;
}

BW_END_NAMESPACE
// initialisation.cpp
