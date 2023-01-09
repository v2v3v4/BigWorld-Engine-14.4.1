#include "pch.hpp"
#include "appmgr/closed_captions.hpp"
#include "appmgr/module_manager.hpp"
#include "appmgr/options.hpp"
#include "appmgr/app.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "ashes/simple_gui.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_item_amortise_delete.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "common/floor.hpp"
#include "common/romp_harness.hpp"
#include "common/tools_camera.hpp"
#include "fmodsound/sound_manager.hpp"
#include "particle/py_particle_system.hpp"
#include "particle/meta_particle_system.hpp"
#include "particle/actions/particle_system_action.hpp"
#include "particle/actions/source_psa.hpp"
#include "particle/actions/vector_generator.hpp"
#include "particle/renderers/particle_system_renderer.hpp"
#include "gizmo/gizmo_manager.hpp"
#include "gizmo/tool_manager.hpp"
#include "main_frame.hpp"
#include "math/planeeq.hpp"
#include "terrain/base_terrain_renderer.hpp"
#include "moo/effect_visual_context.hpp"
#include "terrain/terrain2/terrain_lod_controller.hpp"
#include "moo/texture_manager.hpp"
#include "moo/complex_effect_material.hpp"
#include "moo/draw_context.hpp"
#include "particle_editor.hpp"
#include "pe_module.hpp"
#include "pe_shell.hpp"
#include "pyscript/py_callback.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/custom_mesh.hpp"
#include "romp/flora.hpp"
#include "moo/geometrics.hpp"
#include "speedtree/speedtree_renderer.hpp"
#include "space/deprecated_space_helpers.hpp"
#include "space/client_space.hpp"

DECLARE_DEBUG_COMPONENT2( "Shell", 0 )

BW_BEGIN_NAMESPACE

static AutoConfigString s_defaultSpace( "environment/defaultEditorSpace" );

extern int ChunkTree_token;
extern int ChunkModel_token;
extern int ChunkLight_token;
extern int ChunkTerrain_token;
extern int ChunkFlare_token;
extern int ChunkWater_token;
extern int ChunkParticles_token;
static int s_chunkTokenSet = ChunkModel_token | ChunkLight_token |
	ChunkTerrain_token | ChunkFlare_token | ChunkWater_token |
	ChunkParticles_token 
#ifdef SPEEDTREE_SUPPORT
        | ChunkTree_token
#endif
    ;

extern int Math_token;
extern int PyScript_token;
extern int GUI_token;
extern int ResMgr_token;
static int s_moduleTokens =
	Math_token |
	PyScript_token |
	GUI_token |
	ResMgr_token;

extern int PyGraphicsSetting_token;
static int pyTokenSet = PyGraphicsSetting_token;

namespace PostProcessing
{
	extern int tokenSet;
	static int ppTokenSet = tokenSet;
}

//These are used for Script::tick and the BigWorld.callback fn.
static double g_totalTime = 0.0;

void incrementTotalTime( float dTime )
{
	g_totalTime += (double)dTime;
}

double getTotalTime()
{		
	return g_totalTime;
}

typedef ModuleManager ModuleFactory;

IMPLEMENT_CREATOR(PeModule, Module);

static bool s_enableScripting = true;
static ParticleSystem * myParticleSystem = NULL;

PeModule* PeModule::s_instance_ = NULL;

PeModule::PeModule()
	: selectionStart_( Vector2::zero() )
	, currentSelection_( GridRect::zero() )
	, localToWorld_( GridCoord::zero() )
	, particleSystem_(NULL)
	, averageFps_(0.0f)
	, lastTimeStep_(0.0f)
	, scriptDict_(NULL)
	, drawHelperModel_(false)
{
	BW_GUARD;

	ASSERT(s_instance_ == NULL);
	s_instance_ = this;

	lastCursorPosition_.x = 0;
	lastCursorPosition_.y = 0;
}

PeModule::~PeModule()
{
	BW_GUARD;

	ASSERT(s_instance_);
	s_instance_ = NULL;
}

bool PeModule::init( DataSectionPtr pSection )
{
	return true;
}

void PeModule::onStart()
{
	BW_GUARD;

	// needed, otherwise the mouse cursor is hidden when we start?!
	::ShowCursor(1);

	cc_ = new ClosedCaptions();
	Commentary::instance().addView( &*cc_ );
	cc_->visible( true );

	// Get the handdrawn map for the current space
	BW::string space = Options::getOptionString( "space", s_defaultSpace );

	DataSectionPtr	pDS = BWResource::openSection( space + "/space.settings" );	
	if (pDS)
	{
		// work out grid size
		int minX = pDS->readInt( "bounds/minX", 1 );
		int minY = pDS->readInt( "bounds/minY", 1 );
		int maxX = pDS->readInt( "bounds/maxX", -1 );
		int maxY = pDS->readInt( "bounds/maxY", -1 );

		gridWidth_ = maxX - minX + 1;
		gridHeight_ = maxY - minY + 1;

		localToWorld_ = GridCoord( minX, minY );
	}


	viewPosition_ = Vector3( gridWidth_ / 2.f, gridHeight_ / 2.f, -1.f );

	// set the zoom to the extents of the grid
	float angle =  Moo::rc().camera().fov() / 2.f;
	float yopp = gridHeight_ / 2.f;
	float xopp = gridWidth_ / 2.f;

	// Get the distance away we have to be to see the x points and the y points
	float yheight = yopp / tanf( angle );
	float xheight = xopp / tanf( angle * Moo::rc().camera().aspectRatio() );
	
	// Go back the furthest amount between the two of them
	viewPosition_.z = min( -xheight, -yheight );

	// Turn off HDR for PE
	const bool enableHDR = false;

	ClientSpacePtr clientSpace = DeprecatedSpaceHelpers::cameraSpace();
	if (clientSpace)
	{
		EnviroMinder & enviroMinder = clientSpace->enviro();
		HDRSettings hdrCfg = enviroMinder.hdrSettings();
		hdrCfg.m_enabled = enableHDR;
		enviroMinder.hdrSettings( hdrCfg );
	}
	
	Script::setTotalGameTimeFn( getTotalTime );

	if (s_enableScripting)
		initPyScript();
}


bool PeModule::initPyScript()
{
	BW_GUARD;

	// make a python dictionary here
	PyObject * pScript = PyImport_ImportModule("ParticleEditorDirector");

	scriptDict_ = PyModule_GetDict(pScript);

	PyObject * pInit = PyDict_GetItemString( scriptDict_, "init" );
	if (pInit != NULL)
	{
		PyObject * pResult = PyObject_CallFunction( pInit, "" );

		if (pResult != NULL)
		{
			Py_DECREF( pResult );
		}
		else
		{
			PyErr_Print();
			return false;
		}
	}
	else
	{
		PyErr_Print();
		return false;
	}

	return true;
}


bool PeModule::finiPyScript()
{
	BW_GUARD;

	// make a python dictionary here
	PyObject * pScript = PyImport_ImportModule("ParticleEditorDirector");

	scriptDict_ = PyModule_GetDict(pScript);

	PyObject * pFini = PyDict_GetItemString( scriptDict_, "fini" );
	if (pFini != NULL)
	{
		PyObject * pResult = PyObject_CallFunction( pFini, "" );

		if (pResult != NULL)
		{
			Py_DECREF( pResult );
		}
		else
		{
			PyErr_Print();
			return false;
		}
	}
	else
	{
		PyErr_Print();
		return false;
	}

	return true;
}




int PeModule::onStop()
{
	BW_GUARD;

	Py_DecRef(cc_.getObject());
	cc_ = NULL;

	finiPyScript();
	::ShowCursor( 0 );

	return 0;
}


bool PeModule::updateState( float dTime )
{
	BW_GUARD;

	cc_->update( dTime );

	SimpleGUI::instance().update( dTime );

	// Update the camera.  This interprets the view direction from the 
    // mouse input.
    // We use our saved time here because dTime will be zero when the system 
    // is in pause mode, but we still want the user to be able to walk
    // around.
    uint64 thisTime = timestamp();
    static uint64 lastTime = thisTime - 1;// so that the delta time is > 0 initially

    float myDTime = dTime;
    if (myDTime == 0.0f)
    {        
        myDTime = (float)(((int64)(thisTime - lastTime))/stampsPerSecondD());        
    }
    lastTime = thisTime;
	PeShell::instance().camera().update(myDTime, true);

    // tick time and update the other components, such as romp
	PeShell::instance().romp().update( dTime, false );

	// gizmo manager
	POINT   cursorPos = MainFrame::instance()->CurrentCursorPosition();
	Vector3 worldRay  = MainFrame::instance()->GetWorldRay(cursorPos.x, cursorPos.y);
	ToolPtr spTool    = ToolManager::instance().tool();
	if (spTool)
	{
		spTool->calculatePosition( worldRay );
		spTool->update( dTime );
	}
	else if (GizmoManager::instance().update(worldRay))
		GizmoManager::instance().rollOver();

	// set input focus as appropriate
	bool acceptInput = (MainFrame::instance()->CursorOverGraphicsWnd() != FALSE);
	ParticleEditorApp::instance().mfApp()->handleSetFocus( acceptInput );

	// calc frame rate
	{
		lastTimeStep_ = myDTime;
		float newFps = 1.0f / myDTime;
		// add some damping so display doesn't flicker
		const float damping = 9.0f;
		const float invDamping = 1.0f / (1.0f + damping);
		averageFps_ = (damping * averageFps_ + newFps) * invDamping;
	}

    size_t numParticles = 0;
    size_t sizeBytes    = 0;
	BW::wstring fpsString;
    if (MainFrame::instance()->IsMetaParticleSystem())
    {
        numParticles = 0;
        sizeBytes    = 0;

        if (MainFrame::instance()->numberAppendPS() == 0)
        {
            numParticles += MainFrame::instance()->GetMetaParticleSystem()->size();
            sizeBytes    += MainFrame::instance()->GetMetaParticleSystem()->sizeInBytes();
        }
        else
        {
            // The non-appended particle system doesn't contribute, because it
            // hasn't generated any particles.
            for (size_t i = 0; i < MainFrame::instance()->numberAppendPS(); ++i)
            {
                numParticles += MainFrame::instance()->getAppendedPS(i).size();
                sizeBytes    += MainFrame::instance()->getAppendedPS(i).sizeInBytes();
            }
        }

		fpsString = Localise(L"PARTICLEEDITOR/SHELL/PE_MODULE/FPS_PARTICLES",
			Formatter((int)averageFps_, L"%3d"),
			numParticles,
            Formatter(sizeBytes/1024.0f, L"%.3f"));
    }
    else
    {
		fpsString = Localise(L"PARTICLEEDITOR/SHELL/PE_MODULE/FPS_NO_PARTICLES",
			Formatter((int)averageFps_, L"%3d"));
    }
	MainFrame::instance()->SetPerformancePaneText(fpsString.c_str());

    if (MainFrame::instance()->IsMetaParticleSystem())
    {
	    // update the particle system
	    MainFrame::instance()->GetMetaParticleSystem()->tick(dTime);

        // update any spawnd particle systems
        for (size_t i = 0; i < MainFrame::instance()->numberAppendPS(); ++i)
        {
            MainFrame::instance()->getAppendedPS(i).tick(dTime);
        }
	    if (s_enableScripting)
	    {
		    if (myParticleSystem)
		    {
			    static int asd = 10;
			    if (asd-- < 0)
			    {
				    asd = 10;
				    static_cast<SourcePSA *>(&*myParticleSystem->pAction(PSA_SOURCE_TYPE_ID))->force(1);
			    }
			    myParticleSystem->tick(dTime);
		    }
	    }

		// Delete any without any particles:
		MainFrame::instance()->cleanupAppendPS();
    }

	// update bg color selection if req
	MainFrame::instance()->UpdateBackgroundColor();

	ChunkManager::instance().tick(dTime);
	BgTaskManager::instance().tick();
	FileIOTaskManager::instance().tick();
	ProviderStore::tick( dTime );
	incrementTotalTime( dTime );
	Script::tick( getTotalTime() );
	AmortiseChunkItemDelete::instance().tick();

#if SPEEDTREE_SUPPORT
	speedtree::SpeedTreeRenderer::tick( dTime );
#endif

#if FMOD_SUPPORT
	//Tick FMod by setting the camera position
	Matrix view = PeShell::instance().camera().view();
	view.invert();
	Vector3 cameraPosition = view.applyToOrigin();
	Vector3 cameraDirection = view.applyToUnitAxisVector( 2 );
	Vector3 cameraUp = view.applyToUnitAxisVector( 1 );
	SoundManager::instance().setListenerPosition(
		cameraPosition, cameraDirection, cameraUp, dTime );
#endif // FMOD_SUPPORT

	//Disable flora in ParticleEditor,
	//this needs to be done here since any new spaces (including water reflection scenes)
	//will cause the flora to be set to the highest detail level and thus enable it again.
	Flora::enabled( false );

	IRendererPipeline* rendererPipeline = Renderer::instance().pipeline();
	bool isDeferred = Renderer::instance().pipelineType() == 
		IRendererPipeline::TYPE_DEFERRED_SHADING;

	rendererPipeline->tick(dTime);

	return true;
}

void PeModule::beginRender()
{
	BW_GUARD;

	if (Moo::rc().mixedVertexProcessing())
	{
		Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );
	}


	Moo::rc().reset();
	Moo::rc().updateProjectionMatrix();
	Moo::rc().updateViewTransforms();
}


/**
 *	Update the LOD and animations of chunks and environment in this module.
 */
void PeModule::updateAnimations()
{
	BW_GUARD_PROFILER( PeModule_updateAnimations );

	static DogWatch updateAnimations( "Scene update" );
	ScopedDogWatch sdw( updateAnimations );

	ChunkManager& chunkManager = ChunkManager::instance();
	chunkManager.updateAnimations();

	ChunkSpacePtr pSpace = chunkManager.cameraSpace();
	if (pSpace.exists())
	{
		EnviroMinder & enviro = pSpace->enviro();
		enviro.updateAnimations( );
	}

	ParticleSystemAction::flushLateUpdates();
}

void PeModule::render( float dTime )
{
	BW_GUARD;

	IRendererPipeline* rendererPipeline = Renderer::instance().pipeline();
	bool isDeferred = Renderer::instance().pipelineType() == 
		IRendererPipeline::TYPE_DEFERRED_SHADING;

	rendererPipeline->begin();

	PeShell::instance().camera().render(dTime);
	beginRender();

	BW::string bkgMode = Options::getOptionString( "defaults/bkgMode", "Terrain" );
	if (bkgMode == "Terrain")
	{
		Options::setOptionInt("render/environment/drawSunAndMoon", 1);
		Options::setOptionInt("render/environment/drawSky"       , 1);
		Options::setOptionInt("render/environment/drawClouds"    , 1);
	}
	else
	{
		Options::setOptionInt("render/environment/drawSunAndMoon", 0);
		Options::setOptionInt("render/environment/drawSky"       , 0);
		Options::setOptionInt("render/environment/drawClouds"    , 0);
	}

	BW::RompHarness * romp = bkgMode == "Terrain" ? 
		&PeShell::instance().romp() : NULL;
	if (romp)
	{
		romp->drawPreSceneStuff();
	}

	//-- constants

	Moo::SunLight sun;
	Moo::DirectionalLightPtr dir = ChunkManager::instance().cameraSpace()->sunLight();
	sun.m_dir = dir->direction();
	sun.m_color = dir->colour();
	sun.m_ambient = ChunkManager::instance().cameraSpace()->ambientLight();
	Moo::rc().effectVisualContext().sunLight(sun);
	Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_ALL);

	//-- TODO(a_cherkes): reconsider
	//rendererPipeline->beginCastShadows();
	//rendererPipeline->endCastShadows();

	//-- opaque (terrain or floor only)

#if SPEEDTREE_SUPPORT
	speedtree::SpeedTreeRenderer::beginFrame(
		&ChunkManager::instance().cameraSpace()->enviro(),
		Moo::RENDERING_PASS_COLOR,
		Moo::rc().invView());
#endif

	Moo::DrawContext drawContext( Moo::RENDERING_PASS_COLOR );
	drawContext.begin( Moo::DrawContext::ALL_CHANNELS_MASK );
	rendererPipeline->beginOpaqueDraw();

	//-- render opaques (terrain)
	if (bkgMode == "Terrain")
	{
		Moo::rc().effectVisualContext().initConstants();
        this->renderChunks( drawContext );
		this->renderTerrain(dTime);
	}

	BW::string hlpModelName;
	

	if (MainFrame::instance()->IsMetaParticleSystem())
	{
		hlpModelName = MainFrame::instance()->GetMetaParticleSystem()->helperModelName();
	}
	else
	{
		hlpModelName = Options::getOptionString("helperModel/name", "");
	}

	if (!hlpModelName.empty())
		loadHelperModel( hlpModelName );

	// draw the helper model
	if (drawHelperModel_ && helperModel_ != NULL)
	{
		Moo::rc().push();

		uint helperModelCenterOnHp = helperModelCenterOnHardPoint();
		if (helperModelCenterOnHp < helperModelHardPointTransforms_.size())
			Moo::rc().postMultiply(helperModelHardPointTransforms_[ helperModelCenterOnHp ]);

		Moo::rc().effectVisualContext().initConstants();

		helperModel_->draw( drawContext, true );

		Moo::rc().pop();
	}

	//-- render opaques (floor)
	if (bkgMode == "Floor")
	{
		PeShell::instance().floor().render();
	}

	//-- draw flora
	PeShell::instance().romp().drawSceneStuff( bkgMode == "Terrain", bkgMode == "Terrain" );

	//-- particles
	this->renderParticles( drawContext );

	drawContext.end( Moo::DrawContext::ALL_CHANNELS_MASK );
	drawContext.flush( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
	rendererPipeline->endOpaqueDraw();
	rendererPipeline->applyLighting();

#if SPEEDTREE_SUPPORT
	speedtree::SpeedTreeRenderer::endFrame();
#endif

	//-- other stuff

	this->renderScale(); //-- coordinat system basis

	if (romp)
	{
		romp->drawDelayedSceneStuff();
	}
	else
	{
		// Setup full screen rect
		static Vector2 topLeft( 0.0f, 0.0f );
		Vector2 bottomRight( Moo::rc().screenWidth(), Moo::rc().screenHeight() );
		static const float rectDepth = 1.0f;

		// Draw rect, respecting depth.
		Moo::rc().pushRenderState( D3DRS_ZENABLE );
		Moo::rc().setRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
		{
			Moo::rc().pushRenderState( D3DRS_ZFUNC );
			Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
			{
				Geometrics::drawRect( topLeft, bottomRight, 
					MainFrame::instance()->BgColour(), rectDepth );
			}
			Moo::rc().popRenderState();
		}
		Moo::rc().popRenderState();
	}
	rendererPipeline->beginSemitransparentDraw();
	if (romp)
	{
		drawContext.begin( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK );
		romp->drawPostSceneStuff( drawContext );
		drawContext.end( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK );
	}
	rendererPipeline->endSemitransparentDraw();
	if (romp)
	{
		romp->drawPostProcessStuff();
	}

	Geometrics::flushDrawItems();

	drawContext.begin( Moo::DrawContext::ALL_CHANNELS_MASK );
	this->renderGizmo( drawContext );
	this->renderAndUpdateBound();
	drawContext.end( Moo::DrawContext::ALL_CHANNELS_MASK );
	drawContext.flush( Moo::DrawContext::ALL_CHANNELS_MASK );
		
	SimpleGUI::instance().draw();
	Chunks_drawCullingHUD(); 

	rendererPipeline->drawDebugStuff();
	endRender();
	rendererPipeline->end();
}


void PeModule::renderChunks( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	// draw chunks
    {
		Moo::rc().setRenderState( D3DRS_FILLMODE,
			Options::getOptionInt( "render/scenery/wireFrame", 0 ) ?
				D3DFILL_WIREFRAME : D3DFILL_SOLID );

		ChunkManager::instance().camera( Moo::rc().invView(), *&(ChunkManager::instance().cameraSpace()) );
	    ChunkManager::instance().draw( drawContext );

		Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    }
}

void PeModule::renderTerrain( float dTime )
{
	BW_GUARD;

	if (Options::getOptionInt( "render/terrain", 1 ) != 0 )
	{
		// draw terrain
		bool canSeeTerrain = Terrain::BaseTerrainRenderer::instance()->canSeeTerrain();

		// Update terrain lods
		Terrain::BasicTerrainLodController::instance().setCameraPosition( 
			Moo::rc().invView().applyToOrigin(), 1.f );

		Moo::rc().setRenderState( D3DRS_FILLMODE,
			Options::getOptionInt( "render/terrain/wireFrame", 0 ) ?
				D3DFILL_WIREFRAME : D3DFILL_SOLID );

		Terrain::BaseTerrainRenderer::instance()->drawAll( 
			Moo::RENDERING_PASS_COLOR );

		Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
	}
	else
	{
		Terrain::BaseTerrainRenderer::instance()->clearBlocks();
	}
}


void PeModule::renderScale()
{
	BW_GUARD;

    if (!!Options::getOptionInt( "render/showGrid", 0 ) )
    {
        // Draw a 10x10m coloured grid.
        for (int x = -5; x <= 5; ++x)
        {
            Geometrics::drawLine
            (
                Vector3((float)x,  0.0f, 0.0f),
                Vector3((float)x, 10.0f, 0.0f),
                D3DCOLOR_RGBA(37, 37, 37, 255)
            );
        }
        for (int y = 0; y <= 10; ++y)
        {
            Geometrics::drawLine
            (
                Vector3(-5.0, (float)y, 0.0f),
                Vector3(+5.0, (float)y, 0.0f),
                D3DCOLOR_RGBA(37, 37, 37, 255)
            );
        }
    }
    else
    {
	    static Vector3 N(0.0, 0.0, 0.0);
	    static Vector3 X(1.0, 0.0, 0.0); 
	    static Vector3 Y(0.0, 1.0, 0.0); 
	    static Vector3 Z(0.0, 0.0, 1.0); 
	    static Vector3 bottomRight(0.0, 2.0, 0.0);
	    static Moo::PackedColour colourRed   D3DCOLOR_RGBA( 128,   0,   0, 255 );
	    static Moo::PackedColour colourGreen D3DCOLOR_RGBA(   0, 128,   0, 255 );
	    static Moo::PackedColour colourBlue  D3DCOLOR_RGBA(   0,   0, 128, 255 );

	    // draw axes
	    Geometrics::drawLine(N, X, colourGreen);
	    Geometrics::drawLine(N, Y, colourBlue);
	    Geometrics::drawLine(N, Z, colourRed);
    }
}

void PeModule::renderParticles( Moo::DrawContext& drawContext )
{
	MainFrame *mainFrame = MainFrame::instance();

	// We can use this since the particle system is always at the origin:
	float distance = (Moo::rc().view().applyToOrigin()).length();

	// Render the particle system(s):
	if (mainFrame->IsMetaParticleSystem())
	{
		mainFrame->GetMetaParticleSystem()->draw( drawContext, Matrix::identity, distance );
		for (size_t i = 0; i < mainFrame->numberAppendPS(); ++i)
		{
			mainFrame->getAppendedPS(i).draw( drawContext, Matrix::identity, distance );
		}
	}

	if (s_enableScripting)
	{
		if (myParticleSystem)
			myParticleSystem->draw( drawContext, Matrix::identity, distance );
	}
}

void PeModule::renderGizmo( Moo::DrawContext& drawContext )
{
	ToolPtr spTool = ToolManager::instance().tool();
	if (spTool)
	{
		spTool->render( drawContext );
	}
	GizmoManager::instance().draw( drawContext );
}

void PeModule::renderAndUpdateBound()
{
	MainFrame *mainFrame = MainFrame::instance();

	//Update the particle system bounding box
	BoundingBox frameBB( BoundingBox::s_insideOut_ );
	BoundingBox modelBB( BoundingBox::s_insideOut_ );

	if (mainFrame->IsMetaParticleSystem())
	{
		MetaParticleSystemPtr metaParticleSystem = MainFrame::instance()->GetMetaParticleSystem();

		// Update the bounding box for the particle system
		metaParticleSystem->localBoundingBox( frameBB );
		metaParticleSystem->localVisibilityBoundingBox( modelBB );

		// Now add the bounding boxes for any appended particle systems (i.e. spawned)
		for (size_t i=0; i<mainFrame->numberAppendPS(); i++)
		{
			MetaParticleSystem& appendedPS = mainFrame->getAppendedPS( i );
			BoundingBox newFrameBB( BoundingBox::s_insideOut_ );
			appendedPS.localBoundingBox( newFrameBB );
			BoundingBox newModelBB( BoundingBox::s_insideOut_ );
			appendedPS.localVisibilityBoundingBox( newModelBB );

			if (newFrameBB != BoundingBox::s_insideOut_)
			{
				frameBB.addBounds( newFrameBB );
			}
			if (newModelBB != BoundingBox::s_insideOut_)
			{
				modelBB.addBounds( newModelBB );
			}
		}

		// Render the bounding boxes is needed
		if (modelBB != BoundingBox::s_insideOut_)
		{
			if ( !!Options::getOptionInt( "render/showBB", 0 ) )
			{
				if (frameBB != BoundingBox::s_insideOut_)
					Geometrics::wireBox( frameBB, 0x00ffff00 );
				Geometrics::wireBox( modelBB, 0x000000ff );
			}

			// Ensure the camera box will be above the ground
			modelBB.setBounds(	Vector3(	modelBB.minBounds().x,
				0.f,
				modelBB.minBounds().z ),
				modelBB.maxBounds() );
		}
	}

	// Set a sensible view if not bounding box is found:
	if (modelBB == BoundingBox::s_insideOut_)
	{
		modelBB.setBounds(	Vector3( -1.f, 0.f, -1.f ),
			Vector3(  1.f, 2.f,  1.f ));
	}

	// Set the camera bounding box:
	PeShell::instance().camera().boundingBox( modelBB, false );
}

void PeModule::endRender()
{
}


bool PeModule::handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;

	// usually called through py script
	bool handled = PeShell::instance().camera().handleKeyEvent(event);

	// TODO: Cursor hiding when moving around
	// this should be already done by the camera, but somehow is not reliable..
	// code below not reliable either...

	// Hide the cursor when the right mouse is held down
	if (event.key() == KeyCode::KEY_RIGHTMOUSE)
	{
		handled = true;

		if (event.isKeyDown())
		{
			::ShowCursor(0);
			::GetCursorPos( &lastCursorPosition_ );
		}
		else
		{
			::ShowCursor(1);
			::SetCursorPos( lastCursorPosition_.x, lastCursorPosition_.y );
		}
	}

	if (event.key() == KeyCode::KEY_LEFTMOUSE)
	{
		if (event.isKeyDown())
		{
			handled = true;

			if (GizmoManager::instance().click())
			{
				MainFrame::instance()->PotentiallyDirty
                (
                    true,
                    UndoRedoOp::AK_PARAMETER,
                    LocaliseUTF8(L"PARTICLEEDITOR/SHELL/PE_MODULE/GIZMO_INTERACTION"),
                    true
                );
			}
		}
		else
		{
			MainFrame::instance()->OnBatchedUndoOperationEnd();
		}
	}

	return handled;
}

bool PeModule::handleMouseEvent( const MouseEvent & event )
{
	BW_GUARD;

	// Its ugly, but for the mean time...it needs to be done in CPP
	if ( InputDevices::isKeyDown( KeyCode::KEY_SPACE ) && (event.dz() != 0 ) )
	{
		// Scrolling mouse wheel while holding space: change camera speed
		BW::string currentSpeed = Options::getOptionString( "camera/speed", "Slow" );
		if ( event.dz() > 0 )
		{
			if ( currentSpeed == "Slow" )
				Options::setOptionString( "camera/speed", "Medium" );
			else if ( currentSpeed == "Medium" )
				Options::setOptionString( "camera/speed", "Fast" );
			else if ( currentSpeed == "Fast" )
				Options::setOptionString( "camera/speed", "SuperFast" );
		}
		if ( event.dz() < 0 )
		{
			if ( currentSpeed == "Medium" )
				Options::setOptionString( "camera/speed", "Slow" );
			else if ( currentSpeed == "Fast" )
				Options::setOptionString( "camera/speed", "Medium" );
			else if ( currentSpeed == "SuperFast" )
				Options::setOptionString( "camera/speed", "Fast" );
		}
		BW::string newSpeed = Options::getOptionString( "camera/speed", currentSpeed );
		float speed = newSpeed == "Medium" ? 8.f : newSpeed == "Fast" ? 24.f : newSpeed == "SuperFast" ? 48.f : 1.f;
		PeShell::instance().camera().speed( Options::getOptionFloat( "camera/speed/"+newSpeed, speed ) );
		PeShell::instance().camera().turboSpeed( Options::getOptionFloat( "camera/speed/"+newSpeed+"/turbo", 2*speed ) );
		GUI::Manager::instance().update();
		return true;
	}
	else
		return PeShell::instance().camera().handleMouseEvent(event);
}

Vector2 PeModule::currentGridPos()
{
	BW_GUARD;

	POINT pt = PeShell::instance().currentCursorPosition();
	Vector3 cursorPos = Moo::rc().camera().nearPlanePoint(
			(float(pt.x) / Moo::rc().screenWidth()) * 2.f - 1.f,
			1.f - (float(pt.y) / Moo::rc().screenHeight()) * 2.f );

	Matrix view;
	view.setTranslate( viewPosition_ );

	Vector3 worldRay = view.applyVector( cursorPos );
	worldRay.normalise();

	PlaneEq gridPlane( Vector3(0.f, 0.f, 1.f), .0001f );

	Vector3 gridPos = gridPlane.intersectRay( viewPosition_, worldRay );

	return Vector2( gridPos.x, gridPos.y );
}

Vector3 PeModule::gridPosToWorldPos( Vector2 gridPos )
{
	BW_GUARD;

	Vector2 w = (gridPos + Vector2( float(localToWorld_.x), float(localToWorld_.y) )) *
		ChunkManager::instance().cameraSpace()->gridSize();

	return Vector3( w.x, 0, w.y);
}

const BW::string & PeModule::helperModelName() const
{
	if (MainFrame::instance() == NULL ||
		MainFrame::instance()->GetMetaParticleSystem() == NULL)
	{
		static const BW::string s_EmptyString( "" );
		return s_EmptyString;
	}
	return MainFrame::instance()->GetMetaParticleSystem()->helperModelName();
}

bool PeModule::helperModelName( const BW::string &name )
{
	return loadHelperModel( name );
}

bool PeModule::loadHelperModel(const BW::string & name )
{
	BW_GUARD;

	if( name.empty() &&
		helperModel_.get() != NULL )
	{
		MainFrame::instance()->GetMetaParticleSystem()->helperModelName( name );
		helperModelName_ = name;
		helperModel_ = NULL;
		helperModelHardPointNames_.clear();
		helperModelHardPointTransforms_.clear();
		helperModelCenterOnHardPoint( (uint)(-1) );
		return true;
	}

	if ( name == helperModelName_ )
	{
		return true;
	}

	MainFrame::instance()->GetMetaParticleSystem()->helperModelName( name );
	helperModelName_ = name;
	helperModelHardPointNames_.clear();
	helperModelHardPointTransforms_.clear();
	helperModelCenterOnHardPoint( (uint)(-1) );

	helperModel_ = Model::get( name );
	if ( helperModel_ == NULL )
	{
		return false;
	}

	MetaParticleSystem::getHardPointTransforms( helperModel_, helperModelHardPointNames_, helperModelHardPointTransforms_ );

	return true;
}

void PeModule::helperModelCenterOnHardPoint(uint idx)
{
	idx = std::min<uint>((uint)helperModelHardPointNames_.size() - 1, idx);
	MainFrame::instance()->GetMetaParticleSystem()->helperModelCenterOnHardPoint( idx );
}

uint PeModule::helperModelCenterOnHardPoint() const
{
	return MainFrame::instance()->GetMetaParticleSystem()->helperModelCenterOnHardPoint();
}

void PeModule::drawHelperModel(bool draw)
{
	drawHelperModel_ = draw;

	Options::setOptionBool("helperModel/draw", drawHelperModel_);
}

BW_END_NAMESPACE

