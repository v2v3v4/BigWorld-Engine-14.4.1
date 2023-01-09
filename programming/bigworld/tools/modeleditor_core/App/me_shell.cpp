
#include "pch.hpp"

#include "me_shell.hpp"
#include "me_scripter.hpp"

#include "appmgr/application_input.hpp"
#include "appmgr/module_manager.hpp"
#include "appmgr/options.hpp"
#include "asset_pipeline/asset_client.hpp"
#include "resmgr/auto_config.hpp"
#include "ashes/simple_gui.hpp"
#include "chunk/chunk_item_amortise_delete.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "cstdmf/debug_filter.hpp"
#include "tools/common/tools_camera.hpp"
#include "fmodsound/sound_manager.hpp"
#include "gizmo/tool.hpp"
#include "gizmo/general_editor.hpp"
#include "input/input.hpp"
#include "moo/effect_manager.hpp"
#include "moo/init.hpp"
#include "moo/render_context.hpp"
#include "moo/renderer.hpp"
#include "moo/draw_context.hpp"
#include "terrain/manager.hpp"
#include "terrain/terrain_settings.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/resource_cache.hpp"
#include "romp/console_manager.hpp"
#include "romp/console.hpp"
#include "romp/engine_statistics.hpp"
#include "romp/resource_statistics.hpp"
#include "romp/font_manager.hpp"
#include "romp/flora.hpp"
#include "romp/lens_effect_manager.hpp"
#include "common/dxenum.hpp"
#include "common/material_properties.hpp"
#include "post_processing/manager.hpp"
#include "space/deprecated_space_helpers.hpp"
#include "space/space_manager.hpp"
#include "space/client_space.hpp"
#include "chunk_scene_adapter/client_chunk_space_adapter.hpp"

#include "guimanager/gui_input_handler.hpp"
#include "tools/common/resource_loader.hpp"
#include "splash_dialog.hpp"
#include "material_preview.hpp"

#include "pyscript/pyobject_plus.hpp"

DECLARE_DEBUG_COMPONENT2( "Shell", 0 )

BW_BEGIN_NAMESPACE

static AutoConfigString s_engineConfigXML("system/engineConfigXML");
static AutoConfigString s_floraXML( "environment/floraXML" );
static AutoConfigString s_defaultSpace( "environment/defaultEditorSpace" );
static AutoConfigString s_dxEnumPath( "system/dxenum" );

MeShell * MeShell::s_instance_ = NULL;

// need to define some things for the libs we link to..
// for ChunkManager
BW::string g_specialConsoleString = "";
// for network
const char * compileTimeString = __TIME__ " " __DATE__;

MeShell::MeShell()
    : hInstance_(NULL)
    , hWndApp_(NULL)
    , hWndGraphics_(NULL)
    , renderer_(NULL)
    , romp_(NULL)
    , inited_(false)
{
    BW_GUARD;

    ASSERT(s_instance_ == NULL);
    s_instance_ = this;
    REGISTER_SINGLETON( MeShell )
}

MeShell::~MeShell()
{
    BW_GUARD;

    ASSERT(s_instance_);
    DebugFilter::instance().deleteMessageCallback( &debugMessageCallback_ );
    s_instance_ = NULL;
}


bool MeShell::initApp( HINSTANCE hInstance, HWND hWndApp, HWND hWndGraphics )
{
    BW_GUARD;

    return MeShell::instance().init( hInstance, hWndApp, hWndGraphics );
}


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
MeShell::init( HINSTANCE hInstance, HWND hWndApp, HWND hWndGraphics )
{
    BW_GUARD;

    hInstance_ = hInstance;
    hWndApp_ = hWndApp;
    hWndGraphics_ = hWndGraphics;
    pAssetClient_.reset( new AssetClient() );
    pAssetClient_->waitForConnection();

    initErrorHandling();

    // Create Direct Input devices
    InputDevices * pInputDevices = new InputDevices();

    if (!InputDevices::instance().init( hInstance, hWndGraphics ))
    {
        CRITICAL_MSG( "MeShell::initApp - Init inputDevices FAILED\n" );
        return false;
    }

    if (!initTiming())
    {
        return false;
    }

    if (!initGraphics())
    {
        return false;
    }

    if (!initScripts())
    {
        return false;
    }

    // Need to initialise after scripts because it creates PyTextureProviders
    pLensEffectManager_ = LensEffectManagerPtr( new LensEffectManager() );

    if (!initConsoles())
    {
        return false;
    }

    initSound(); // No need to exit if this fails

    // Init the terrain
    pTerrainManager_ = TerrainManagerPtr( new Terrain::Manager() );

    // Init the MaterialPreview singleton. No need to store the pointer in a
    // member variable because we'll use the singleton's pInstance to delete
    // it.
    MaterialPreview* pMaterialPreview = new MaterialPreview();

    // romp needs chunky
    {
        new DXEnum( s_dxEnumPath );

        new AmortiseChunkItemDelete();
        ClientChunkSpaceAdapter::init();
        ChunkManager::instance().init();

        ResourceCache::instance().init();

        // Precompile effects?
        if ( Options::getOptionInt( "precompileEffects", 1 ) )
        {
            BW::vector<ISplashVisibilityControl*> SVCs;
            if (CSplashDlg::getSVC())
                SVCs.push_back(CSplashDlg::getSVC());
            ResourceLoader::instance().precompileEffects( SVCs );
        }

        ClientSpacePtr clientSpace = SpaceManager::instance().findOrCreateSpace( 1 );
        ChunkSpacePtr space = ClientChunkSpaceAdapter::getChunkSpace( clientSpace );

        BW::string spacePath = Options::getOptionString( "space", s_defaultSpace );
        Matrix& nonConstIdentity = const_cast<Matrix&>( Matrix::identity );
        GeometryMapping* mapping = space->addMapping( SpaceEntryID(), nonConstIdentity, spacePath );
        if (!mapping)
        {
            ERROR_MSG( "Couldn't map path \"%s\" as a space.\n", spacePath.c_str() );
            // To let it work without mapping for s_defaultSpace, used in "Terrain" mode.
        }

        ChunkManager::instance().camera( Matrix::identity, space );
        DeprecatedSpaceHelpers::cameraSpace( clientSpace );

        // Call tick to give it a chance to load the outdoor seed chunk, before
        // we ask it to load the dirty chunks
        ChunkManager::instance().tick( 0.f );
    }

    if (!initRomp())
    {
        return false;
    }

    ApplicationInput::disableModeSwitch();

    this->inited_ = true;

    return true;
}

bool MeShell::initSound()
{
    BW_GUARD;

#if FMOD_SUPPORT
    //Load the "engine_config.xml" file...
    DataSectionPtr configRoot = BWResource::instance().openSection( s_engineConfigXML.value() );
    
    if (!configRoot)
    {
        ERROR_MSG( "MeShell::initSound: Couldn't open \"%s\"\n", s_engineConfigXML.value().c_str() );
        return false;
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
            ERROR_MSG( "MeShell::initSound: Failed to initialise sound\n" );
            return false;
        }
    }
    else
    {
        ERROR_MSG( "MeShell::initSound: No <soundMgr> config section found, sound support is disabled\n" );
        return false;
    }
#endif // FMOD_SUPPORT
    
    return true;
}

bool MeShell::inited()
{
    return inited_;
}

bool MeShell::initRomp()
{
    BW_GUARD;

    if ( !romp_ )
    {
        romp_ = new RompHarness;

        if ( !romp_->init() )
        {
            ERROR_MSG( "MeShell::initRomp: init romp FAILED\n" );
            Py_DECREF( romp_ );
            romp_ = NULL;
            return false;
        }

        // set it into the ModelEditor module
        PyObject * pMod = PyImport_AddModule( "ModelEditor" );  // borrowed
        PyObject_SetAttrString( pMod, "romp", romp_ );

        romp_->enviroMinder().activate();
    }
    return true;
}


/**
 *  This method finalises the application. All stuff done in initApp is undone.
 *  @todo make a better sentence
 */
void MeShell::fini()
{
    BW_GUARD;

    if ( !this->inited() )
    {
        return;
    }
    if ( romp_ )
        romp_->enviroMinder().deactivate();

    ResourceLoader::fini();

    DeprecatedSpaceHelpers::cameraSpace( nullptr );

    ChunkManager::instance().processPendingChunks();
    ChunkManager::instance().checkLoadingChunks();
    ChunkManager::instance().fini();

    ResourceCache::instance().fini();

    if ( romp_ )
    {
        PyObject * pMod = PyImport_AddModule( "ModelEditor" );
        PyObject_DelAttrString( pMod, "romp" );

        Py_DECREF( romp_ );
        romp_ = NULL;
    }

    delete AmortiseChunkItemDelete::pInstance();

    delete MaterialPreview::pInstance();

    pTerrainManager_.reset();

    MeShell::finiGraphics();

    MeShell::finiScripts();

    delete InputDevices::pInstance();

    BWResource::instance().purgeAll();

    // Kill winsock
    //WSACleanup();

    LogMsg::fini();

    PropManager::fini();

    ModuleManager::fini();

    delete DXEnum::pInstance();

    pAssetClient_.reset( NULL );
}


/*
 *  Override from DebugMessageCallback.
 */
bool MeShellDebugMessageCallback::handleMessage(
    DebugMessagePriority messagePriority, const char * /*pCategory*/,
    DebugMessageSource /*messageSource*/, const LogMetaData & /*metaData*/,
    const char * pFormat, va_list argPtr )
{
    BW_GUARD;

    return MeShell::instance().messageHandler(  messagePriority,
                                                pFormat,
                                                argPtr );
}


/**
 *  This method sets up error handling, by routing error msgs
 *  to our own static function
 *
 *  @see messageHandler
 */
bool MeShell::initErrorHandling()
{
    BW_GUARD;

    MF_WATCH( "debug/filterThreshold", DebugFilter::instance(),
                MF_ACCESSORS( DebugMessagePriority, DebugFilter,
                    filterThreshold ) );

    DebugFilter::instance().addMessageCallback( &debugMessageCallback_ );
    return true;
}


// -----------------------------------------------------------------------------
// Section: Graphics
// -----------------------------------------------------------------------------

/**
 *  This method initialises the graphics subsystem.
 *
 *  @param hInstance    The HINSTANCE variable for the application.
 *  @param hWnd         The HWND variable for the application's 3D view window.
 *
 * @return true if initialisation went well
 */
bool MeShell::initGraphics()
{
    BW_GUARD;

    if ( !Moo::init() )
        return false;

    // Read render surface options.
    uint32 width = Options::getOptionInt( "graphics/width", 1024 );
    uint32 height = Options::getOptionInt( "graphics/height", 768 );
    uint32 bpp = Options::getOptionInt( "graphics/bpp", 32 );
    bool fullScreen = Options::getOptionBool( "graphics/fullScreen", false );

    int iDeviceToUse = 0;
    int iDevice = 0;
    for( ; iDevice < (int)Moo::rc().nDevices(); iDevice++ )
    {
        if ( strstr( Moo::rc().deviceInfo( iDevice ).identifier_.Description, "PerfHUD" ) )
        {
            iDeviceToUse = iDevice;
            break;
        }
    }


    const Moo::DeviceInfo& di = Moo::rc().deviceInfo( iDeviceToUse );

    uint32 modeIndex = 0;
    bool foundMode = false;

    // Go through available modes and try to match a mode from the options file.
    while( foundMode != true &&
        modeIndex != di.displayModes_.size() )
    {
        if( di.displayModes_[ modeIndex ].Width == width &&
            di.displayModes_[ modeIndex ].Height == height )
        {
            if( bpp == 32 &&
                ( di.displayModes_[ modeIndex ].Format == D3DFMT_R8G8B8 ||
                di.displayModes_[ modeIndex ].Format == D3DFMT_X8R8G8B8 ) )
            {
                foundMode = true;
            }
            else if( bpp == 16 &&
                ( di.displayModes_[ modeIndex ].Format == D3DFMT_R5G6B5 ||
                di.displayModes_[ modeIndex ].Format == D3DFMT_X1R5G5B5 ) )
            {
                foundMode = true;
            }
        }
        if (!foundMode)
            modeIndex++;
    }

    // If the mode could not be found. Set windowed and mode 0.
    if (!foundMode)
    {
        modeIndex = 0;
        fullScreen = false;
    }

    // Read shadow options.
    bool useShadows = Options::getOptionBool( "graphics/shadows", false );

    DataSectionPtr graphicsPrefDS = BWResource::openSection( Options::getOptionString(
        "graphics/graphicsPreferencesXML",
        "resources/graphics_preferences.xml" ) );

    int overrideAutoDetectGraphicsSettings = Options::getOptionInt(
        "graphics/overrideAutoDetectGraphicsSettings", 
        Moo::GraphicsSettingsAutoDetector::NO_OVERRIDE );
    //DataSectionPtr ds = XMLSection::createFromBinary("graphicsPreferences", bp);
    if (!Moo::GraphicsSetting::init(graphicsPrefDS, overrideAutoDetectGraphicsSettings))
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

    renderer_.reset(new Renderer());

    bool isAssetProcessor = false;
    bool isSupportedDS    = (Moo::rc().deviceInfo(deviceIndex).compatibilityFlags_ & Moo::COMPATIBILITYFLAG_DEFERRED_SHADING) != 0;
    if (!renderer_->init( isAssetProcessor, isSupportedDS ))
    {
        ERROR_MSG("DeviceApp::init()  Could not initialize renderer's pipeline.\n");
        return false;
    }


    if (!Moo::rc().createDevice( hWndGraphics_, 0, modeIndex, !fullScreen, 
        useShadows, Vector2(0, 0), false, false
#if ENABLE_ASSET_PIPE
        , pAssetClient_.get() 
#endif
        ))
    {
        CRITICAL_MSG( "MeShell:initApp: Moo::rc().createDevice() FAILED\n" );
        return false;
    }

    if ( Moo::rc().device() )
    {
        ::ShowCursor( true );
    }
    else
    {
        CRITICAL_MSG( "MeShell:initApp: Moo::rc().device() FAILED\n" );
        return false;
    }

    //Use no fogging...
    Moo::FogParams params = Moo::FogHelper::pInstance()->fogParams();
    params.m_start = 5000.f;
    params.m_end   = 10000.f;
    Moo::FogHelper::pInstance()->fogParams(params);

    //Load Ashes
    SimpleGUI::instance().hwnd( hWndGraphics_ );

    pTextureFeeds_ = TextureFeedsPtr( new TextureFeeds() );

    pPostProcessingManager_ = 
        PostProcessingManagerPtr( new PostProcessing::Manager() );

    //We need to call this so that we can set material properties
    MaterialProperties::runtimeInitMaterialProperties();

    // Hide the 3D window to avoid it turning black from the clear device in
    // the following method
    ::ShowWindow( hWndGraphics_, SW_HIDE );

    pFontManager_ = FontManagerPtr( new FontManager() );

    ::ShowWindow( hWndGraphics_, SW_SHOW );

    return true;
}


/**
 *  This method finalises the graphics sub system.
 */
void MeShell::finiGraphics()
{
    BW_GUARD;

    pLensEffectManager_.reset();
    pFontManager_.reset();
    pPostProcessingManager_.reset();
    pTextureFeeds_.reset();

    Moo::VertexDeclaration::fini();

    Moo::rc().releaseDevice();

    renderer_.reset();

    DataSectionPtr graphicsPrefDS = BWResource::openSection(
        Options::getOptionString( "graphics/graphicsPreferencesXML", "resources/graphics_preferences.xml" ), true );
    if ( graphicsPrefDS != NULL )
    {
        Moo::GraphicsSetting::write( graphicsPrefDS );
        graphicsPrefDS->save();
    }

    Moo::GraphicsSetting::finiSettingsData();

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
bool MeShell::initScripts()
{
    BW_GUARD;

    if (!Scripter::init( BWResource::instance().rootSection() ))
    {
        CRITICAL_MSG( "MeShell::initScripts: failed\n" );
        return false;
    }

    return true;
}


/**
 * This method finalises the scripting environment
 */
void MeShell::finiScripts()
{
    BW_GUARD;

    Scripter::fini();
}

POINT MeShell::currentCursorPosition() const
{
    BW_GUARD;

    POINT pt;
    ::GetCursorPos( &pt );
    ::ScreenToClient( hWndGraphics_, &pt );

    return pt;
}


// -----------------------------------------------------------------------------
// Section: Consoles
// -----------------------------------------------------------------------------

/**
 *  This method initialises the consoles.
 */
bool MeShell::initConsoles()
{
    BW_GUARD;

    // Initialise the consoles
    ConsoleManager & mgr = ConsoleManager::instance();

    XConsole * pStatusConsole = new XConsole();
    XConsole * pStatisticsConsole =
        new StatisticsConsole( &EngineStatistics::instance() );

    mgr.add( pStatisticsConsole,                                            "Default",  KeyCode::KEY_F5, MODIFIER_CTRL );
    mgr.add( new ResourceUsageConsole( &ResourceStatistics::instance() ),   "Resource", KeyCode::KEY_F5, MODIFIER_CTRL | MODIFIER_SHIFT );
    mgr.add( new DebugConsole(),                                            "Debug",    KeyCode::KEY_F7, MODIFIER_CTRL );
    mgr.add( new PythonConsole(),                                           "Python",   KeyCode::KEY_P, MODIFIER_CTRL );
    mgr.add( pStatusConsole,                                                "Status" );

    pStatusConsole->setConsoleColour( 0xFF26D1C7 );
    pStatusConsole->setScrolling( true );
    pStatusConsole->setCursor( 0, 20 );

    return true;
}

// -----------------------------------------------------------------------------
// Section: Timing
// -----------------------------------------------------------------------------

/**
 *  This method inits the timing code, it also sets up the
 *  
 */
bool MeShell::initTiming()
{
    BW_GUARD;
    
    // this initialises the internal timers for stamps per second, this can take up to a second
    double d = stampsPerSecondD();

    return true;
}


// -----------------------------------------------------------------------------
// Section: Miscellaneous
// -----------------------------------------------------------------------------


/**
 *  This static function implements the callback that will be called for each
 *  *_MSG.
 */
bool MeShell::messageHandler( DebugMessagePriority /*messagePriority*/,
        const char * /*pFormat*/, va_list /*argPtr*/ )
{
    // This is commented out to stop ModelEditor from spitting out
    // an error box for every message, it still appears in 3D view
    // and messages panel.
    //if (messagePriority == MESSAGE_PRIORITY_ERROR)
    //{
    //  bool fullScreen = !Moo::rc().windowed();

    //  //don't display message boxes in full screen mode.
    //  char buf[2*BUFSIZ];
    //  bw_vsnprintf( buf, sizeof(buf), pFormat, argPtr );
    //  buf[sizeof(buf)-1] = '\0';

    //  if ( LogMsg::showErrorDialogs() && !fullScreen )
    //  {
    //      ::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
    //          buf, "Error", MB_ICONERROR | MB_OK );
    //  }
    //}
    return false;
}

/**
 *  Output streaming operator for MeShell.
 */
std::ostream& operator<<(std::ostream& o, const MeShell& t)
{
    BW_GUARD;

    o << "MeShell\n";
    return o;
}
BW_END_NAMESPACE

