#include "pch.hpp"
#include "canvas_app.hpp"

#include "cstdmf/config.hpp"
#include "cstdmf/debug.hpp"
#include "chunk/chunk_manager.hpp"
#include "particle/particle_system_manager.hpp"
#include "math/colour.hpp"
#include "moo/animating_texture.hpp"
#include "moo/effect_visual_context.hpp"
#include "resmgr/datasection.hpp"
#if ENABLE_CONSOLES
#include "romp/console.hpp"
#include "romp/console_manager.hpp"
#endif // ENABLE_CONSOLES
#include "romp/histogram_provider.hpp"
#include "romp/log_console.hpp"
#include "romp/progress.hpp"
#include "romp/distortion.hpp"
#include "romp/water.hpp"
#include "post_processing/manager.hpp"

#include "action_matcher.hpp"
#include "app.hpp"
#include "app_config.hpp"
#include "device_app.hpp"
#include "physics.hpp"
#include "script_bigworld.hpp"
#include "script_player.hpp"

#include "space/deprecated_space_helpers.hpp"
#include "space/client_space.hpp"


BW_BEGIN_NAMESPACE

CanvasApp CanvasApp::instance;

int CanvasApp_token = 1;

#if ENABLE_MSG_LOGGING
MEMTRACKER_REFERENCE( LogConsole );
#endif // ENABLE_MSG_LOGGING

CanvasApp::CanvasApp() :
	gammaCorrectionOutside_( 1.f ),
	gammaCorrectionInside_( 1.f ),
	gammaCorrectionSpeed_( 0.2f ),
	dGameTime_( 0.f ),
	distortion_( NULL ),	
	drawSkyCtrl_( EnviroMinder::DRAW_ALL )
{
	BW_GUARD;
	MainLoopTasks::root().add( this, "Canvas/App", NULL );
}


CanvasApp::~CanvasApp()
{
	BW_GUARD;
	/*MainLoopTasks::root().del( this, "Canvas/App" );*/
}


bool CanvasApp_getSkyBoxesToggle()
{
	BW_GUARD;
	return
		(CanvasApp::instance.drawSkyCtrl_
		& EnviroMinder::DRAW_SKY_BOXES) != 0;
}


void CanvasApp_setSkyBoxesToggle(bool on)
{
	BW_GUARD;
	CanvasApp::instance.drawSkyCtrl_ = on
		? CanvasApp::instance.drawSkyCtrl_ | EnviroMinder::DRAW_SKY_BOXES
		: CanvasApp::instance.drawSkyCtrl_ & ~EnviroMinder::DRAW_SKY_BOXES;
}


bool CanvasApp::init()
{
	BW_GUARD_PROFILER( AppTick_Canvas );
#if ENABLE_WATCHERS
	DEBUG_MSG( "CanvasApp::init: Initially using %d(~%d)KB\n",
		memUsed(), memoryAccountedFor() );
#endif

	DataSectionPtr configSection = AppConfig::instance().pRoot();

	EnviroMinder::init();

#if ENABLE_CONSOLES
	// Initialise the consoles
	ConsoleManager & mgr = ConsoleManager::instance();

	XConsole * pPythonConsole = new PythonConsole();
	XConsole * pStatusConsole = new XConsole();

	mgr.add( pPythonConsole,		"Python" );
	mgr.add( pStatusConsole,		"Status" );

#if ENABLE_MSG_LOGGING
	MEMTRACKER_BEGIN( LogConsole );
	XConsole * pLogConsole =
		new LogConsole( App::instance().getRenderTimeNow() );
	MEMTRACKER_END();

	mgr.add( pLogConsole,			"Log" );
#endif // ENABLE_MSG_LOGGING

	this->setPythonConsoleHistoryNow( history_ );

	Vector3 colour = configSection->readVector3( "ui/loadingText", Vector3(255,255,255) );
	pStatusConsole->setConsoleColour( Colour::getUint32( colour, 255 ) );
	pStatusConsole->setScrolling( true );
	pStatusConsole->setCursor( 0, pStatusConsole->visibleHeight()-2 );
#endif // ENABLE_CONSOLES
	// print some status information
	loadingText( BW::string( "Resource path:   " ) +
		BWResource::getDefaultPath() );

	loadingText(BW::string( "App config file: " ) + s_configFileName);

	// Initialise the adaptive lod controller
	lodController_.minimumFPS( 10.f );
	lodController_.addController( "clod", &CLODPower, 10.f, 15.f, 50.f );

	MF_WATCH( "Client Settings/LOD/FPS",
		lodController_,
		&AdaptiveLODController::effectiveFPS,
		"Effective fps as seen by the adaptive Level-of-detail controller." );
	MF_WATCH( "Client Settings/LOD/Minimum fps",
		lodController_,
		MF_ACCESSORS( float, AdaptiveLODController, minimumFPS ),
		"Minimum fps setting for the adaptive level-of-detail controller.  FPS "
		"below this setting will cause adaptive lodding to take place." );
	MF_WATCH(
		"Client Settings/Sky Dome2/Render sky boxes",
		&CanvasApp_getSkyBoxesToggle, &CanvasApp_setSkyBoxesToggle,
		"Toggles rendering of the sky boxes" );

	for ( int i = 0; i < lodController_.numControllers(); i++ )
	{
		AdaptiveLODController::LODController & controller = lodController_.controller( i );
		BW::string watchPath = "Client Settings/LOD/" + controller.name_;
		BW::string watchName = watchPath + "/current";
		MF_WATCH( watchName.c_str(), controller.current_, Watcher::WT_READ_ONLY );
		watchName = "Client Settings/LOD/";
		watchName = watchName + controller.name_;
		watchName = watchName + " curr";
		MF_WATCH( watchName.c_str(), controller.current_, Watcher::WT_READ_ONLY );
		watchName = watchPath + "/default";
		MF_WATCH( watchName.c_str(), controller, MF_ACCESSORS( float, AdaptiveLODController::LODController, defaultValue ));
		watchName = watchPath + "/worst";
		MF_WATCH( watchName.c_str(), controller, MF_ACCESSORS( float, AdaptiveLODController::LODController, worst ));
		watchName = watchPath + "/speed";
		MF_WATCH( watchName.c_str(), controller, MF_ACCESSORS( float, AdaptiveLODController::LODController, speed ));
		watchName = watchPath + "/importance";
		MF_WATCH( watchName.c_str(), controller.relativeImportance_ );
	}

	// and some fog stuff
	Moo::FogParams params = Moo::FogHelper::pInstance()->fogParams();
	params.m_start = 0;
	params.m_end = 500;
	params.m_color = 0x102030;
	Moo::FogHelper::pInstance()->fogParams( params );

	// Renderer settings
	MF_WATCH( "Render/waitForVBL", Moo::rc(),
		MF_ACCESSORS( bool, Moo::RenderContext, waitForVBL ),
		"Enable locking of frame presentation to the vertical blank signal" );
	MF_WATCH( "Render/tripleBuffering", Moo::rc(),
		MF_ACCESSORS( bool, Moo::RenderContext, tripleBuffering ),
		"Enable triple-buffering, including the front-buffer and 2 back buffers" );

	gammaCorrectionOutside_ = configSection->readFloat(
		"renderer/gammaCorrectionOutside", configSection->readFloat(
		"renderer/gammaCorrection", gammaCorrectionOutside_ ) );
	gammaCorrectionInside_ = configSection->readFloat(
		"renderer/gammaCorrectionInside", configSection->readFloat(
		"renderer/gammaCorrection", gammaCorrectionInside_ ) );
	gammaCorrectionSpeed_ = configSection->readFloat(
		"renderer/gammaCorrectionSpeed", gammaCorrectionSpeed_ );

	MF_WATCH( "Render/Gamma Correction Outside",
		gammaCorrectionOutside_,
		Watcher::WT_READ_WRITE,
		"Gama correction factor when the camera is in outside chunks" );
	MF_WATCH( "Render/Gamma Correction Inside",
		gammaCorrectionInside_,
		Watcher::WT_READ_WRITE,
		"Gamma correction factor when the camera is in indoor chunks" );
	MF_WATCH( "Render/Gamma Correction Now", Moo::rc(),
		MF_ACCESSORS( float, Moo::RenderContext, gammaCorrection),
		"Current gama correction factor" );

	Moo::rc().gammaCorrection( gammaCorrectionOutside_ );

	MF_WATCH( "Render/Enviro draw",
		(int&)drawSkyCtrl_,
		Watcher::WT_READ_WRITE,
		"Enable / Disable various environment features such as sky, "
		"sun, moon and clouds." );

	// misc stuff

	ActionMatcher::globalEntityCollision_ =
		configSection->readBool( "entities/entityCollision", false );

	ParticleSystemManager::instance().active( configSection->readBool(
		"entities/particlesActive",
		ParticleSystemManager::instance().active() ) );

	Physics::setMovementThreshold( configSection->readFloat(
		"entities/movementThreshold", 0.25 ) );

	bool ret = DeviceApp::s_pStartupProgTask_->step(APP_PROGRESS_STEP);

	if (distortion_ == NULL)
	{
		if (Distortion::isSupported())
		{
			// initialised at first use
			distortion_ = new Distortion;
		}
		else
		{
			INFO_MSG( "Distortion is not supported on this hardware\n" );
		}
	}

	if (App::instance().isQuiting())
	{
		return false;
	}
	return ret;
}


void CanvasApp::fini()
{
	BW_GUARD;

	bw_safe_delete(distortion_);
	
	EnviroMinder::fini();
}


void CanvasApp::tick( float dGameTime, float dRenderTime )
{
	BW_GUARD;
	dGameTime_ = dGameTime;

	// Update the animating textures
	Moo::AnimatingTexture::tick( dGameTime );
	Moo::Material::tick( dGameTime );
	Moo::rc().effectVisualContext().tick( dGameTime );

	//Adaptive degradation section
	lodController_.fpsTick( 1.f / dRenderTime );

	// set the values
	int controllerIdx = 0;

	/*float recommendedDistance = lodController_.controller( controllerIdx++ ).current_;
	if ( this->getFarPlane() > recommendedDistance )
	{
		this->setFarPlane( recommendedDistance );
	}*/
	Moo::rc().lodPower( lodController_.controller( controllerIdx++ ).current_ );

	if (distortion_)
		distortion_->tick( dGameTime );
}


void CanvasApp::draw()
{
	BW_GUARD_PROFILER( AppDraw_Canvas );
	GPU_PROFILER_SCOPE( AppDraw_Canvas );
	if(!gWorldDrawEnabled)
		return;

	// set the gamma level
	float desiredGamma = isCameraOutside() ?
		gammaCorrectionOutside_ : gammaCorrectionInside_;
	float currentGamma = Moo::rc().gammaCorrection();
	if (currentGamma != desiredGamma)
	{
		currentGamma += Math::clamp( gammaCorrectionSpeed_ * dGameTime_,
			desiredGamma - currentGamma );
		Moo::rc().gammaCorrection( currentGamma );
	}

	//TODO : this is specific to heat shimmer, which may or may not be included
	//via the post processing chain.  Make generic.
	Moo::rc().setRenderState( 
		D3DRS_COLORWRITEENABLE, 
			D3DCOLORWRITEENABLE_RED | 
			D3DCOLORWRITEENABLE_GREEN | 
			D3DCOLORWRITEENABLE_BLUE );

	Moo::Material::shimmerMaterials = true;
	//end TODO

	PostProcessing::Manager::instance().tick( dGameTime_ );

	// render the backdrop
	ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
	if (pSpace)
	{
		pSpace->enviro().drawHind( dGameTime_, drawSkyCtrl_ );
	}
}


void CanvasApp::updateDistortionBuffer()
{
	BW_GUARD;

	if (distortion() && distortion()->begin() )
	{
		// if player isnt visible in main buffer, draw it to the copy...
		if (ScriptPlayer::entity() != NULL &&
			ScriptPlayer::entity()->pPrimaryModel() != NULL &&
			!ScriptPlayer::entity()->pPrimaryModel()->visible() )
		{
			ScriptPlayer::entity()->pPrimaryModel()->visible(true);
			DX::Surface* oldDepth = NULL;
			Moo::rc().device()->GetDepthStencilSurface( &oldDepth );
			Moo::rc().device()->SetDepthStencilSurface( NULL );	
			Moo::rc().device()->SetDepthStencilSurface( oldDepth );
			oldDepth->Release();
			ScriptPlayer::entity()->pPrimaryModel()->visible(false);
		}
		ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
		if (pSpace)
		{
			pSpace->enviro().drawFore( dGameTime_, true, false, false, true, false );
		}
		distortion()->end( );
	}
	else
	{
		Waters::instance().drawDrawList( dGameTime_ );
	}
}


const CanvasApp::StringVector CanvasApp::pythonConsoleHistory() const
{
	BW_GUARD;
#if ENABLE_CONSOLES
	PythonConsole * console = static_cast<PythonConsole *>(
		ConsoleManager::instance().find("Python"));

	return console != NULL
		? console->history()
		: this->history_;
#else
	return this->history_;
#endif // ENABLE_CONSOLES
}


void CanvasApp::setPythonConsoleHistory(const StringVector & history)
{
	BW_GUARD;
#if ENABLE_CONSOLES
	if (!this->setPythonConsoleHistoryNow(history))
	{
		this->history_ = history;
	}
#endif // ENABLE_CONSOLES
}


bool CanvasApp::setPythonConsoleHistoryNow(const StringVector & history)
{
	BW_GUARD;
#if ENABLE_CONSOLES
	PythonConsole * console = static_cast<PythonConsole *>(
		ConsoleManager::instance().find("Python"));

	bool result = console != NULL;
	if (result)
	{
		console->setHistory(history);
	}
	return result;
#else
	return false;
#endif // ENABLE_CONSOLES
}


void CanvasApp::finishFilters()
{
	BW_GUARD;
	if(!gWorldDrawEnabled)
		return;

	//TODO : this is specific to shimmer channel. remove
	App::instance().drawContext( App::COLOUR_DRAW_CONTEXT ).flush( Moo::DrawContext::SHIMMER_CHANNEL_MASK );
	Moo::Material::shimmerMaterials = false;
	//end TODO

	PostProcessing::Manager::instance().draw();	

	HistogramProvider::instance().update();
}

BW_END_NAMESPACE

// canvas_app.
