#include "pch.hpp"
#include "device_app.hpp"
#include "app.hpp"
#include "app_config.hpp"
#include "bw_winmain.hpp"
#include "camera_app.hpp"
#include "canvas_app.hpp"
#include "client_camera.hpp"
#include "connection_control.hpp"
#include "critical_handler.hpp"
#include "entity_manager.hpp"
#include "settings_overrides.hpp"
#include "asset_pipeline/asset_client.hpp"
#include "camera/annal.hpp"
#include "camera/direction_cursor.hpp"
#include "chunk/chunk_manager.hpp"
#include "connection/smart_server_connection.hpp"
#include "fmodsound/sound_manager.hpp"
#include "math/colour.hpp"
#include "moo/texture_manager.hpp"
#include "moo/texture_streaming_manager.hpp"
#include "moo/vertex_format_cache.hpp"
#include "moo/renderer.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/xml_section.hpp"
#include "romp/font_manager.hpp"
#include "romp/lens_effect_manager.hpp"
#include "romp/progress.hpp"
#include "romp/texture_feeds.hpp"
#include "moo/texture_renderer.hpp"
#include "romp/water.hpp"
#include "post_processing/manager.hpp"
#include "scaleform/config.hpp"

#if SCALEFORM_SUPPORT
    #include "scaleform/manager.hpp"
#if SCALEFORM_IME
    #include "scaleform/ime.hpp"
#endif
#endif


BW_BEGIN_NAMESPACE

DeviceApp DeviceApp::instance;

int DeviceApp_token = 1;

static DogWatch g_watchInput("Input");

namespace {

#if SCALEFORM_IME
const AutoConfigString s_scaleformIMEMovie( 
    "scaleform/imeMovie", 
    "IME.swf" );
#endif

}


DeviceApp::DeviceApp() :
        dRenderTime_( 0.f ),
        soundEnabled_( true ),
        pInputHandler_( NULL ),
        pAssetClient_( NULL ),
        pConnectionControl_( NULL )
{
    BW_GUARD;
    MainLoopTasks::root().add( this, "Device/App", NULL );
}


DeviceApp::~DeviceApp()
{
    BW_GUARD;
    /*MainLoopTasks::root().del( this, "Device/App" );*/
}


bool DeviceApp::init()
{
    BW_GUARD;
    if (App::instance().isQuiting())
    {
        return false;
    }
#if ENABLE_WATCHERS
    DEBUG_MSG( "DeviceApp::init: Initially using %d(~%d)KB\n",
        memUsed(), memoryAccountedFor() );
#endif

    BgTaskManager::init();
    FileIOTaskManager::init();

    // Open the preferences
    DataSectionPtr configSection = AppConfig::instance().pRoot();

    // preferences
    preferencesFilename_.init( 
                configSection->openSection( "preferences" ),
                "preferences.xml", 
                PathedFilename::BASE_EXE_PATH );

    // Set up the processor affinity
    DataSectionPtr preferences;
    DataResource dataRes;
    if (dataRes.load( preferencesFilename_.resolveName() ) == DataHandle::DHE_NoError)
    {
        preferences = dataRes.getRootSection();
    }

    if (preferences)
    {
        DataSectionPtr appPreferences = 
            preferences->openSection("appPreferences");
        if (appPreferences.exists())
        {
        }
    }

    // Init the timestamp, this can take up to 1 second
    double sps = stampsPerSecondD();

    // 1. Input

    // init input devices
    if (!InputDevices::instance().init( s_hInstance_, s_hWnd_ ))
    {
        ERROR_MSG( "DeviceApp::init: Init inputDevices FAILED\n" );
        return false;
    }

    // 2. Network
    initNetwork();

    pConnectionControl_ = new ConnectionControl();

    // 3. Graphics
    // Search suitable video mode
    const Moo::DeviceInfo& di = Moo::rc().deviceInfo( 0 );

    int maxWindowWidth = 0;
    int maxWindowHeight = 0;
    int maxNumberPixels = 0;
    for ( BW::vector< D3DDISPLAYMODE >::const_iterator it = di.displayModes_.begin() ;
            it != di.displayModes_.end() ; ++it )
    {
        const D3DDISPLAYMODE & dm = *it;

        int numberPixels = dm.Width * dm.Height;
        if ( numberPixels > maxNumberPixels )
        {
            maxNumberPixels = numberPixels;
            maxWindowWidth = dm.Width;
            maxWindowHeight = dm.Height;
        }
    }

    bool windowed     = true;
    bool waitVSync    = true;
    bool tripleBuffering = true;
    float aspectRatio = 4.0f/3.0f;
    int windowWidth   = 1024;
    int windowHeight  = 768;
    int fullscreenWidth = 1024;
    int fullscreenHeight = 768;

    // load graphics settings
    if (preferences)
    {
        DataSectionPtr graphicsPreferences =
            preferences->openSection("graphicsPreferences");

        Moo::GraphicsSetting::init(graphicsPreferences);

        DataSectionPtr devicePreferences =
            preferences->openSection("devicePreferences");

        if (devicePreferences.exists())
        {
            windowed     = devicePreferences->readBool("windowed", windowed);
            waitVSync    = devicePreferences->readBool("waitVSync", waitVSync);
            tripleBuffering = devicePreferences->readBool("tripleBuffering", tripleBuffering);
            aspectRatio  = devicePreferences->readFloat("aspectRatio", aspectRatio);
            windowWidth  = devicePreferences->readInt("windowedWidth", windowWidth);
            windowHeight = devicePreferences->readInt("windowedHeight", windowHeight);
            fullscreenWidth = devicePreferences->readInt("fullscreenWidth", fullscreenWidth);
            fullscreenHeight = devicePreferences->readInt("fullscreenHeight", fullscreenHeight);

            // limit width and height
            windowWidth  = Math::clamp(512, windowWidth, maxWindowWidth);
            windowHeight = Math::clamp(384, windowHeight, maxWindowHeight);
        }

        // console history
        DataSectionPtr consoleSect = 
            preferences->openSection("consoleHistory");

        if (consoleSect.exists())
        {
            CanvasApp::StringVector history;
            consoleSect->readWideStrings("line", history);

            // unfortunately, the python console hasn't been 
            // created yet at this point. Delegate to the canvas 
            // app to restore the python console history
            CanvasApp::instance.setPythonConsoleHistory(history);
        }

        s_scriptsPreferences =
            preferences->openSection("scriptsPreferences", true);
    }
    else
    {
        // set blank xml data section
        s_scriptsPreferences = new XMLSection("root");
    }

    // apply command line overrides for screen settings
    ScreenSettingsOverrides sso = readCommandLineScreenSettingsOverrides();
    if (sso.screenModeForced_)
    {
        windowed = !sso.fullscreen_;
    }
    if (sso.resolutionForced_)
    {
        if (windowed)
        {
            windowWidth = sso.resolutionX_;
            windowHeight = sso.resolutionY_;
        }
        else
        {
            fullscreenWidth = sso.resolutionX_;
            fullscreenHeight = sso.resolutionY_;
        }

        MF_ASSERT( sso.resolutionY_ > 0 );
        aspectRatio = sso.resolutionX_/(float)sso.resolutionY_;
    }

    bgColour_ = Vector3( 160, 180, 250 ) * 0.9f;
    uint32 deviceIndex = 0;

    // search for a suitable video mode.
    int modeIndex = 0;
    const int numDisplay = static_cast<int>(di.displayModes_.size());

    int i = 0;
    for ( ; i < numDisplay; i++ )
    {
        if ( ( di.displayModes_[i].Width == fullscreenWidth ) &&
             ( di.displayModes_[i].Height == fullscreenHeight ) && 
             ( ( di.displayModes_[i].Format == D3DFMT_X8R8G8B8 ) ||
               ( di.displayModes_[i].Format == D3DFMT_A8B8G8R8 ) ) )
        {
            modeIndex = i;
            break;
        }

    }
    if ( i >= numDisplay )
    {
        // This will be used as a fallback.
        const int numTraits = 6;
        const int32 modeTraits[numTraits][4] = {
            { 1024, 768, D3DFMT_X8R8G8B8, D3DFMT_A8B8G8R8 },
            { 800,  600, D3DFMT_X8R8G8B8, D3DFMT_A8B8G8R8 },
            { 640,  480, D3DFMT_X8R8G8B8, D3DFMT_A8B8G8R8 },
            { 1024, 768, D3DFMT_R5G6B5,   D3DFMT_X1R5G5B5 },
            { 800,  600, D3DFMT_R5G6B5,   D3DFMT_X1R5G5B5 },
            { 640,  480, D3DFMT_R5G6B5,   D3DFMT_X1R5G5B5 },
        };

        int searchIndex = 0;
        while(searchIndex < numDisplay * numTraits)
        {
            const int mIndex = searchIndex % numDisplay;
            const int tIndex = (searchIndex / numDisplay) % numTraits;
            if( di.displayModes_[mIndex].Width  == modeTraits[tIndex][0]  &&
                di.displayModes_[mIndex].Height == modeTraits[tIndex][1] && (
                di.displayModes_[mIndex].Format == modeTraits[tIndex][2] ||
                di.displayModes_[mIndex].Format == modeTraits[tIndex][3]))
            {
                if (sso.screenModeForced_ && sso.fullscreen_ && sso.resolutionForced_)
                {
                    // user-forced resolution failed, so print message
                    ERROR_MSG( 
                        "DeviceApp: Command line supplied fullscreen resolution of '%dx%d' "
                        "is not a supported mode. Falling back to '%dx%d'.\n",
                        sso.resolutionX_, sso.resolutionY_, 
                        di.displayModes_[mIndex].Width, di.displayModes_[mIndex].Height );
                }

                modeIndex = mIndex;
                break;
            }
            ++searchIndex;
        }
    }

    // Enable NVIDIA PerfKit hooks for all builds except consumer builds. This
    // means we can debug an issue without needing a rebuild of the application.
#if ENABLE_NVIDIA_PERFHUD
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
#endif

    App::instance().resizeWindow(windowWidth, windowHeight);
    Moo::rc().fullScreenAspectRatio(aspectRatio);
    Moo::rc().waitForVBL(waitVSync);
    Moo::rc().tripleBuffering(tripleBuffering);

    //-- we need find some place before creating render device but at 
    //-- moment when we've already knew all needed info about users machine spec 
    //-- to select right renderer pipeline.
    renderer_.reset(new Renderer());
    bool isAssetProcessor = false;
    bool isSupportedDS = (Moo::rc().deviceInfo(deviceIndex).compatibilityFlags_ 
        & Moo::COMPATIBILITYFLAG_DEFERRED_SHADING) != 0;
    if (!renderer_->init(isAssetProcessor, isSupportedDS))
    {
        ERROR_MSG("DeviceApp::init()  Could not initialize renderer's pipeline.\n");
        return false;
    }

    int maxModeIndex = static_cast<int>(
        Moo::rc().deviceInfo(deviceIndex).displayModes_.size() - 1);

#if ENABLE_ASSET_PIPE
    if (pAssetClient_ != NULL)
    {
        pAssetClient_->waitForConnection();
    }
#endif

    modeIndex = std::min(modeIndex, maxModeIndex);
    Vector2 size = Vector2(float(windowWidth), float(windowHeight));
    if (!Moo::rc().createDevice( s_hWnd_, deviceIndex, modeIndex, windowed, true, 
        size, true, false
#if ENABLE_ASSET_PIPE
        , pAssetClient_
#endif
        ))

    {
        ERROR_MSG( "DeviceApp::init()  Could not create Direct3D device\n" );
        return false;
    }

#if SCALEFORM_SUPPORT
    pScaleFormBWManager_ = ScaleFormBWManagerPtr( new ScaleformBW::Manager );
#if SCALEFORM_IME
    ScaleformBW::IME::init( s_scaleformIMEMovie.value() );
#endif
#endif

    bool ret = true;

    DataSectionPtr ptr = BWResource::instance().openSection("shaders/formats");
    if (ptr)
    {
        DataSectionIterator it = ptr->begin();
        DataSectionIterator end = ptr->end();
        while (it != end)
        {
            BW::string format = (*it)->sectionName();
            size_t off = format.find_first_of( "." );
            format = format.substr( 0, off );

            const Moo::VertexFormat* vertexFormat = 
                Moo::VertexFormatCache::get( format );
            if (vertexFormat && 
                Moo::VertexDeclaration::isFormatSupportedOnDevice( *vertexFormat ))
            {
                Moo::VertexDeclaration::get( format );
            }
            ++it;
        }
    }

    // wait for windows to
    // send us a paint message
    uint64  tnow = timestamp();
    while ( (timestamp() - tnow < stampsPerSecond()/2) && ret)
    {
        ret = BWProcessOutstandingMessages();
    }

    // init the texture feed instance, this
    // registers material section processors.
    setupTextureFeedPropertyProcessors();

    // 4. Sound
#if FMOD_SUPPORT
    soundEnabled_ = configSection->readBool( "soundMgr/enabled", soundEnabled_ );
    if (soundEnabled_)
    {
        DataSectionPtr dsp = configSection->openSection( "soundMgr" );

        if (dsp)
        {
            if (!SoundManager::instance().initialise( dsp ))
            {
                ERROR_MSG( "DeviceApp::init: Failed to initialise sound\n" );
            }
        }
        else
        {
            ERROR_MSG( "DeviceApp::init: "
                "No <soundMgr> config section found, sound support is "
                "disabled\n" );
        }
    }
    else
    {
        SoundManager::instance().errorLevel( SoundManager::ERROR_LEVEL_SILENT );
    }
#endif // FMOD_SUPPORT

    pTextureFeeds_ = TextureFeedsPtr( new TextureFeeds() );

    pPostProcessingManager_ = 
        PostProcessingManagerPtr( new PostProcessing::Manager() );

    pFontManager_ = FontManagerPtr( new FontManager() );

    pLensEffectManager_ = LensEffectManagerPtr( new LensEffectManager() );

    return ret;
}


/**
 *  This method finalises the application.
 *  DeviceApp finalisation involves the following steps:
 *  a. Stop/cancel all background tasks.
 *  b. Finalise in the reverse order of DeviceApp::init().
 */
void DeviceApp::fini()
{
    BW_GUARD;

    pLensEffectManager_.reset();

    //
    // a. Stop/cancel background tasks

    // Stop background tasks before shutting down anything else, as they may
    // be using one of those things
    FileIOTaskManager::fini();
    BgTaskManager::fini();
    Moo::GraphicsSetting::finiSettingsData();

    //
    // b. Finalise in the reverse order of DeviceApp::init().

    // 5. Misc
    bw_safe_delete( DeviceApp::s_pStartupProgTask_ );
    bw_safe_delete( DeviceApp::s_pProgress_ );

    pFontManager_.reset();
    pPostProcessingManager_.reset();
    pTextureFeeds_.reset();

    // 4. Sound
#if FMOD_SUPPORT
    SoundManager::instance().fini();
#endif // FMOD_SUPPORT

    // 3. Graphics

#if SCALEFORM_SUPPORT
    pScaleFormBWManager_.reset();
#endif

    // release the render context
    // has to be done here and not in device, as this may free up
    // some pythonised stuff
    Moo::rc().releaseDevice();

    //-- destroy renderer right after device releasing.
    renderer_.reset();

    s_scriptsPreferences = NULL;

    // 2. Network
    bw_safe_delete( pConnectionControl_ );
    finiNetwork();

    // 1. Input
    // InputDevices is deleted in ~App
}


void DeviceApp::tick( float dGameTime, float dRenderTime )
{
    BW_GUARD_PROFILER( AppTick_Device );

    if (s_pProgress_ != NULL)
    {
        bw_safe_delete(s_pStartupProgTask_);
        bw_safe_delete(s_pProgress_);
    }

    dRenderTime_ = dRenderTime;
    g_watchInput.start();
    MF_ASSERT( pInputHandler_ != NULL );
    InputDevices::processEvents( *pInputHandler_ );
    //  make sure we are still connected.
    ConnectionControl::instance().tick( dGameTime );
    g_watchInput.stop();
    // get the direction cursor to process its input immediately here too
    DirectionCursor::instance().tick( dGameTime );

    Moo::TextureManager::instance()->streamingManager()->tick();

    Moo::rc().nextFrame();
}


void DeviceApp::inactiveTick( float dGameTime, float dRenderTime )
{
    BW_GUARD;
    dRenderTime_ = dRenderTime;
    //  make sure we are still connected.
    ConnectionControl::instance().tick( dGameTime );

    Moo::rc().disableResourcePreloads();
}


void DeviceApp::draw()
{
    BW_GUARD_PROFILER( AppDraw_Device );
    GPU_PROFILER_SCOPE(AppDraw_Device);
    Moo::rc().beginScene();

    Renderer::instance().pipeline()->begin();

    // begin rendering
    if (Moo::rc().mixedVertexProcessing())
        Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );

    DX::Viewport viewport;
    viewport.Width = (DWORD)Moo::rc().screenWidth();
    viewport.Height = (DWORD)Moo::rc().screenHeight();
    viewport.MinZ = 0.f;
    viewport.MaxZ = 1.f;
    viewport.X = 0;
    viewport.Y = 0;
    Moo::rc().setViewport( &viewport );

    uint32 clearFlags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
    if ( Moo::rc().stencilAvailable() )
        clearFlags |= D3DCLEAR_STENCIL;
    Moo::rc().device()->Clear( 0, NULL, clearFlags,
        Colour::getUint32( bgColour_ ), 1, 0 );

    Moo::rc().setWriteMask( 1, D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_ALPHA );

    //HACK_MSG( "Heap size %d\n", heapSize() );
    // update any dynamic textures
    Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA |
        D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
    TextureRenderer::updateDynamics( dRenderTime_ );
    Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
        D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );

    //TODO: under water effect..
    Waters::instance().checkVolumes();

#if FMOD_SUPPORT
    // ### FMOD
    Vector3 cameraPosition = CameraApp::instance().clientCamera().camera()->invView().applyToOrigin();
    Vector3 cameraDirection = CameraApp::instance().clientCamera().camera()->invView().applyToUnitAxisVector( 2 );
    Vector3 cameraUp = CameraApp::instance().clientCamera().camera()->invView().applyToUnitAxisVector( 1 );
    SoundManager::instance().setListenerPosition(
        cameraPosition, cameraDirection, cameraUp, dRenderTime_ );
#endif // FMOD_SUPPORT

}


void DeviceApp::deleteGUI()
{
    BW_GUARD;
    bw_safe_delete(DeviceApp::s_pProgress_);
}


bool DeviceApp::savePreferences()
{
    BW_GUARD;
    bool result = false;
    BW::string preferencesFilename = preferencesFilename_.resolveName();
    if (!preferencesFilename.empty())
    {
        BWResource::ensureAbsolutePathExists( preferencesFilename );

        DataResource dataRes(preferencesFilename, RESOURCE_TYPE_XML, true);
        DataSectionPtr root = dataRes.getRootSection();

        // graphics preferences
        DataSectionPtr grPref = root->openSection("graphicsPreferences", true);
        Moo::GraphicsSetting::write(grPref);

        // device preferences
        DataSectionPtr devPref = root->openSection("devicePreferences", true);
        devPref->delChildren();
        devPref->writeBool( "windowed",    Moo::rc().windowed());
        devPref->writeBool( "waitVSync",   Moo::rc().waitForVBL());
        devPref->writeBool( "tripleBuffering",   Moo::rc().tripleBuffering());
        devPref->writeFloat("aspectRatio", Moo::rc().fullScreenAspectRatio());

        Vector2 windowSize = Moo::rc().windowedModeSize();
        devPref->writeInt("windowedWidth",  int(windowSize.x));
        devPref->writeInt("windowedHeight", int(windowSize.y));

        const Moo::DeviceInfo& di = Moo::rc().deviceInfo( 0 );

        if (di.displayModes_.size() > Moo::rc().modeIndex())
        {
            const D3DDISPLAYMODE& mode = di.displayModes_[Moo::rc().modeIndex()];
            devPref->writeInt( "fullscreenWidth", mode.Width );
            devPref->writeInt( "fullscreenHeight", mode.Height );
        }

        // script preferences
        DataSectionPtr scrptPref = root->openSection("scriptsPreferences", true);
        scrptPref->delChildren();
        scrptPref->copy(s_scriptsPreferences);

        CanvasApp::StringVector history = CanvasApp::instance.pythonConsoleHistory();
        DataSectionPtr consoleSect = root->openSection("consoleHistory", true);
        consoleSect->delChildren();
        consoleSect->writeWideStrings("line", history);

        // save it
        if (dataRes.save() == DataHandle::DHE_NoError)
        {
            result = true;
        }
        else
        {
            ERROR_MSG("Could not save preferences file.\n");
        }
    }
    return result;
}

void DeviceApp::assetClient( AssetClient* pAssetClient )
{
    pAssetClient_ = pAssetClient;
}

BW_END_NAMESPACE

// device_app.cpp
