#include "pch.hpp"
#include "pe_shell.hpp"

#include "tools/common/floor.hpp"
#include "pe_scripter.hpp"
#include "appmgr/application_input.hpp"
#include "appmgr/module_manager.hpp"
#include "appmgr/options.hpp"
#include "asset_pipeline/asset_client.hpp"
#include "ashes/simple_gui.hpp"
#include "chunk/chunk_item_amortise_delete.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "common/tools_camera.hpp"
#include "common/romp_harness.hpp"
#include "cstdmf/debug_filter.hpp"
#include "fmodsound/sound_manager.hpp"
#include "gizmo/tool.hpp"
#include "input/input.hpp"
#include "moo/effect_manager.hpp"
#include "moo/init.hpp"
#include "moo/render_context.hpp"
#include "terrain/manager.hpp"
#include "terrain/terrain_settings.hpp"
#include "particle/particle_system_manager.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/resource_cache.hpp"
#include "romp/console_manager.hpp"
#include "romp/console.hpp"
#include "romp/engine_statistics.hpp"
#include "romp/font_manager.hpp"
#include "romp/resource_statistics.hpp"
#include "romp/weather.hpp"
#include "romp/texture_feeds.hpp"
#include "romp/lens_effect_manager.hpp"
#include "post_processing/manager.hpp"
#include "guimanager/gui_input_handler.hpp"
#include "tools/common/resource_loader.hpp"
#include "gui/dialogs/splash_dialog.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "moo/fog_helper.hpp"
#include "space/deprecated_space_helpers.hpp"
#include "space/space_manager.hpp"
#include "space/client_space.hpp"
#include "chunk_scene_adapter/client_chunk_space_adapter.hpp"

DECLARE_DEBUG_COMPONENT2( "Shell", 0 )

BW_BEGIN_NAMESPACE

static AutoConfigString s_engineConfigXML("system/engineConfigXML");
static AutoConfigString s_ShellDefaultSpace( "environment/defaultEditorSpace" );
static AutoConfigString s_defaultFloor( "system/defaultFloorTexture" );

PeShell * PeShell::s_instance_ = NULL;

// need to define some things for the libs we link to..
// for ChunkManager
BW::string g_specialConsoleString = "";
// for network
const char * compileTimeString = __TIME__ " " __DATE__;

PeShell::PeShell():
	inited_( false ),
	hInstance_(NULL),
	hWndApp_(NULL),
	hWndGraphics_(NULL),
	romp_(NULL),
	camera_(NULL),
	floor_(NULL),
	space_(NULL),
	m_renderer(NULL)
{
    ASSERT(s_instance_ == NULL);
    s_instance_ = this;
}

PeShell::~PeShell()
{
	BW_GUARD;

    ASSERT(s_instance_);
    DebugFilter::instance().deleteMessageCallback( &debugMessageCallback_ );
    s_instance_ = NULL;
}

/*static*/ PeShell &PeShell::instance()
{
	BW_GUARD;

    ASSERT(s_instance_);
    return *s_instance_;
}

/*static*/ bool PeShell::hasInstance()
{
    return s_instance_ != NULL;
}

bool PeShell::initApp( HINSTANCE hInstance, HWND hWndApp, HWND hWndGraphics )
{
	BW_GUARD;

    return PeShell::instance().init( hInstance, hWndApp, hWndGraphics );
}


/**
 *  This method intialises global resources for the application.
 *
 *  @param hInstance    The HINSTANCE variable for the application.
 *  @param hWnd         The HWND variable for the application's main window.
 *  @param hWndGraphics The HWND variable the 3D device will use.
 *
 *  @return     True if initialisation succeeded, otherwise false.
 */
bool
PeShell::init( HINSTANCE hInstance, HWND hWndApp, HWND hWndGraphics )
{
	BW_GUARD;

	MF_ASSERT( !inited_ );

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
        ERROR_MSG( "Error initialising input devices.\n" );
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

	// Init the particles
	MF_VERIFY( ParticleSystemManager::init() );

    // romp needs chunky
    {
		new AmortiseChunkItemDelete();

		SpaceManager::instance().init();
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
		space_ = ClientChunkSpaceAdapter::getChunkSpace( clientSpace );

        BW::string spacePath = Options::getOptionString( "space", s_ShellDefaultSpace );
		Matrix& nonConstIdentity = const_cast<Matrix&>( Matrix::identity );
        GeometryMapping * mapping = space_->addMapping( SpaceEntryID(), (float*)nonConstIdentity, spacePath );
        if (!mapping)
        {
            ERROR_MSG( "Couldn't map path %s as a space.\n", spacePath.c_str() );
            return false;
        }

        ChunkManager::instance().camera( Matrix::identity, space_ );
		DeprecatedSpaceHelpers::cameraSpace( clientSpace );

        // Call tick to give it a chance to load the outdoor seed chunk, before
        // we ask it to load the dirty chunks
        ChunkManager::instance().tick( 0.f );
    }

    if (!initRomp())
    {
        return false;
    }

    if (!initCamera())
    {
        return false;
    }

	floor_ = new Floor(Options::getOptionString("settings/floorTexture", s_defaultFloor ));

	ApplicationInput::disableModeSwitch();

	inited_ = true;

    return true;
}

bool PeShell::initSound()
{
#if FMOD_SUPPORT
	BW_GUARD;

	//Load the "engine_config.xml" file...
	DataSectionPtr configRoot = BWResource::instance().openSection( s_engineConfigXML.value() );

	if (!configRoot)
	{
		ERROR_MSG( "PeShell::initSound: Couldn't open \"%s\"\n", s_engineConfigXML.value().c_str() );
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
			ERROR_MSG( "PeShell::initSound: Failed to initialise sound\n" );
			return false;
		}
	}
	else
	{
		ERROR_MSG( "PeShell::initSound: No <soundMgr> config section found, sound support is disabled\n" );
		return false;
	}
#endif // FMOD_SUPPORT

	return true;
}

bool PeShell::initCamera()
{
	BW_GUARD;

    camera_ = ToolsCameraPtr(new ToolsCamera(), true);
    camera_->windowHandle( hWndGraphics_ );
    camera_->maxZoomOut(200.0f);
    BW::string speedName = Options::getOptionString( "camera/speed", "Slow" );
    camera_->speed( Options::getOptionFloat( "camera/speed/" + speedName, 1.f ) );
    camera_->turboSpeed( Options::getOptionFloat( "camera/speed/" + speedName + "/turbo", 2.f ) );
    camera_->mode( Options::getOptionInt( "camera/mode", 0 ) );
    camera_->invert( !!Options::getOptionInt( "camera/invert", 0 ) );
    camera_->rotDir( Options::getOptionInt( "camera/rotDir", -1 ) );
    camera_->view( Options::getOptionMatrix34( "startup/lastView", camera_->view() ) );

    return true;
}

bool PeShell::initRomp()
{
	BW_GUARD;

    if ( !romp_ )
    {
        romp_ = new RompHarness;

        // set it into the ParticleEditor module
        PyObject * pMod = PyImport_AddModule( "ParticleEditor" );  // borrowed
        PyObject_SetAttrString( pMod, "romp", romp_ );

        if ( !romp_->init() )
        {
            CRITICAL_MSG( "PeShell::initRomp: init romp FAILED\n" );
            return false;
        }

		romp_->enviroMinder().activate();

		// no more hardcoded wind! instead, edit game\bigworld\res\helpers\spaces\pe\space.settings,
		// add <windSpeed> x z </windSpeed> and <windGustiness> g </windGustiness> tags under root tag if you want some wind
		//// Add some wind so that particle systems that are affected by wind can
		//// be tested.
		//romp_->enviroMinder().weather()->windAverage(1.0f, 0.5f);
    }
    return true;
}


/**
 *  This method finalises the application. All stuff done in initApp is undone.
 *  @todo make a better sentence
 */
void PeShell::fini()
{
	BW_GUARD;

	if (!inited_)
	{
		return;
	}

	inited_ = false;

	if ( romp_ )
		romp_->enviroMinder().deactivate();

	delete floor_; floor_ = NULL;

	ResourceLoader::fini();

	DeprecatedSpaceHelpers::cameraSpace( NULL );
	space_ = NULL;

	BgTaskManager::instance().stopAll();
	ChunkManager::instance().fini();
	SpaceManager::instance().fini();

	ResourceCache::instance().fini();

	if ( romp_ )
	{
		PyObject * pMod = PyImport_AddModule( "ParticleEditor" );
		PyObject_DelAttrString( pMod, "romp" );

		Py_DECREF( romp_ );
		romp_ = NULL;
	}

	delete AmortiseChunkItemDelete::pInstance();

	pTerrainManager_.reset();

	PeShell::finiScripts();

	PeShell::finiGraphics();

	delete InputDevices::pInstance();

    // Kill winsock
    //WSACleanup();

	ModuleManager::fini();

    pAssetClient_.reset( NULL );
}

HINSTANCE &PeShell::hInstance()
{
    return hInstance_;
}

HWND &PeShell::hWndApp()
{
    return hWndApp_;
}

HWND &PeShell::hWndGraphics()
{
    return hWndGraphics_;
}

RompHarness & PeShell::romp()
{
    return *romp_;
}

ToolsCamera &PeShell::camera()
{
    return *camera_;
}

Floor& PeShell::floor()
{
	return *floor_;
}


/*
 *	Override from DebugMessageCallback.
 */
bool PeShellDebugMessageCallback::handleMessage(
	DebugMessagePriority messagePriority, const char * /*pCategory*/,
	DebugMessageSource /*messageSource*/, const LogMetaData & /*metaData*/,
	const char * pFormat, va_list argPtr )
{
	BW_GUARD;

    return PeShell::instance().messageHandler(	messagePriority,
                                                pFormat,
                                                argPtr );
}


/**
 *  This method sets up error handling, by routing error msgs
 *  to our own static function
 *
 *  @see messageHandler
 */
bool PeShell::initErrorHandling()
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
bool PeShell::initGraphics()
{
	BW_GUARD;

	// Initialise Moo
	if ( !Moo::init() )
		return false;

    // Read render surface options.
    int width = Options::getOptionInt( "graphics/width", 1024 );
    int height = Options::getOptionInt( "graphics/height", 768 );
    int bpp = Options::getOptionInt( "graphics/bpp", 32 );
    bool fullScreen = Options::getOptionBool( "graphics/fullScreen", false );

	//-- Init renderer pipeline
	const bool wantDeferred = true;
	bool isDeferred = Options::getOptionBool( "graphics/isDeferred", wantDeferred );

	const char* deferredEnabled = 
		"		<root>										 "
		"			<entry>									 "
		"				<label>	RENDER_PIPELINE	</label>	 "
		"				<activeOption>	0	</activeOption>	 "
		"			</entry>								 "
		"		</root>										 "
		;

	const char* deferredDisabled = 
		"		<root>										 "
		"			<entry>									 "
		"				<label>	RENDER_PIPELINE	</label>	 "
		"				<activeOption>	1	</activeOption>	 "
		"			</entry>								 "
		"		</root>										 "
		;

	const char* data = isDeferred ? deferredEnabled : deferredDisabled;
	BinaryPtr bp(new BinaryBlock(data, strlen(data) + 1, ""));
	DataSectionPtr ds = XMLSection::createFromBinary("graphicsPreferences", bp);
	if (!Moo::GraphicsSetting::init(ds))
	{
		ERROR_MSG("Moo::GraphicsSetting::init()  Could not initialise the graphics settings.\n");
		return false;
	}

	m_renderer.reset(new Renderer());
	if (!m_renderer->init(false, isDeferred))
	{
		ERROR_MSG("DeviceApp::init()  Could not initialize renderer's pipeline.\n");
		return false;
	}

	//-- Create device

    const Moo::DeviceInfo& di = Moo::rc().deviceInfo( 0 );

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

    //for (uint32 i = 0; i < Moo::rc().nDevices(); i++)
	//{
	//    if (BW::string(Moo::rc().deviceInfo(i).identifier_.Description) == BW::string("NVIDIA NVPerfHUD"))
	//    {
	//         modeIndex = i;
	//	  }
	//}

    // Initialise the directx device with the settings from the options file.

    if (!Moo::rc().createDevice( hWndGraphics_, 0, modeIndex, !fullScreen, 
		useShadows, Vector2(0, 0), false, false
#if ENABLE_ASSET_PIPE
		, pAssetClient_.get() 
#endif
		))
    {
        CRITICAL_MSG( "Creation of DirectX device failed\n" );
        return false;
    }

    if ( Moo::rc().device() )
    {
        ::ShowCursor( true );
    }
    else
    {
        CRITICAL_MSG( "Unable to use DirectX device\n" );
        return false;
    }

	// Fog not required - this ensures that if a shader has enabled fog state it will be ignored.
	Moo::FogHelper::pInstance()->fogEnable(false);

	//ASHES
    SimpleGUI::instance().hwnd( hWndGraphics_ );

	pTextureFeeds_ = TextureFeedsPtr( new TextureFeeds() );

	pPostProcessingManager_ = 
		PostProcessingManagerPtr( new PostProcessing::Manager() );


    // camera

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
void PeShell::finiGraphics()
{
	BW_GUARD;

	pLensEffectManager_.reset();
	pFontManager_.reset();
	pPostProcessingManager_.reset();
	pTextureFeeds_.reset();

	Moo::VertexDeclaration::fini();

    Moo::rc().releaseDevice();

	m_renderer.reset();

	Moo::GraphicsSetting::finiSettingsData();
	// Finalise Moo
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
bool PeShell::initScripts()
{
	BW_GUARD;

    if (!Scripter::init( BWResource::instance().rootSection() ))
    {
        ERROR_MSG( "Scripting failed to initialise\n" );
        return false;
    }

    return true;
}


/**
 * This method finalises the scripting environment
 */
void PeShell::finiScripts()
{
	BW_GUARD;

    Scripter::fini();
}

POINT PeShell::currentCursorPosition() const
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
bool PeShell::initConsoles()
{
	BW_GUARD;

    // Initialise the consoles
    ConsoleManager & mgr = ConsoleManager::instance();

    XConsole * pStatusConsole = new XConsole();
    XConsole * pStatisticsConsole =
        new StatisticsConsole( &EngineStatistics::instance() );

    mgr.add( pStatisticsConsole,											"Default",  KeyCode::KEY_F5, MODIFIER_CTRL );
	mgr.add( new ResourceUsageConsole( &ResourceStatistics::instance() ),	"Resource",	KeyCode::KEY_F5, MODIFIER_CTRL | MODIFIER_SHIFT );
    mgr.add( new DebugConsole(),        									"Debug",    KeyCode::KEY_F7, MODIFIER_CTRL );
    mgr.add( new PythonConsole(),       									"Python",   KeyCode::KEY_P, MODIFIER_CTRL );
    mgr.add( pStatusConsole,            									"Status" );

    pStatusConsole->setConsoleColour( 0xFF26D1C7 );
    pStatusConsole->setScrolling( true );
    pStatusConsole->setCursor( 0, 20 );

    return true;
}


// -----------------------------------------------------------------------------
// Section: Miscellaneous
// -----------------------------------------------------------------------------


/**
 *  This static function implements the callback that will be called for each
 *  *_MSG.
 */
bool PeShell::messageHandler( DebugMessagePriority /*messagePriority*/,
        const char * /*pFormat*/, va_list /*argPtr*/ )
{
	BW_GUARD;

	// Nothing to do here at the moment.  Messages are now handled by the
	// messages page.
    return false;
}

/**
 *  Output streaming operator for PeShell.
 */
std::ostream& operator<<(std::ostream& o, const PeShell& t)
{
	BW_GUARD;

    o << "PeShell\n";
    return o;
}
BW_END_NAMESPACE

