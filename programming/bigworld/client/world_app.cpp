#include "pch.hpp"
#include "world_app.hpp"

#include "cstdmf/watcher.hpp"
#include "app.hpp"
#include "app_config.hpp"
#include "canvas_app.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk_scene_adapter/client_chunk_space_adapter.hpp"
#include "cstdmf/main_loop_task.hpp"
#include "device_app.hpp"
#include "entity_udo_factory.hpp"
#include "duplo/foot_trigger.hpp"
#include "duplo/py_attachment.hpp"
#include "entity_manager.hpp"
#include "terrain/manager.hpp"
#include "terrain/base_terrain_renderer.hpp"
#include "terrain/terrain2/terrain_lod_controller.hpp"
#include "moo/fog_helper.hpp"
#include "resmgr/datasection.hpp"
#include "romp/progress.hpp"
#include "moo/texture_renderer.hpp"
#include "romp/water.hpp"
#include "script_player.hpp"

#include "moo/forward_decal.hpp"
#include "moo/renderer.hpp"

#include "model/model.hpp"
#include "moo/draw_context.hpp"
#include "moo/deferred_lights_manager.hpp"

#include "particle/actions/particle_system_action.hpp"

#include "space/space_manager.hpp"
#include "space/client_space.hpp"
#include "space/deprecated_space_helpers.hpp"
#include "space/global_embodiments.hpp"
#include "space/light_embodiment.hpp"

#include "scene/scene.hpp"
#include "scene/scene_object.hpp"
#include "scene/intersect_scene_view.hpp"
#include "scene/intersection_set.hpp"
#include "scene/draw_operation.hpp"
#include "scene/scene_draw_context.hpp"
#include "scene/scene_intersect_context.hpp"
#include "scene/draw_helpers.hpp"

#include "compiled_space/terrain2_scene_provider.hpp"
#include "compiled_space/py_attachment_entity_embodiment.hpp"
#include "compiled_space/entity_udo_factory.hpp"
#include "compiled_space/compiled_space.hpp"
#include "terrain/terrain_hole_map.hpp"

#if UMBRA_ENABLE
#include "chunk/chunk_umbra.hpp"
#include "math/colour.hpp"
#endif

#include "math/frustum_hull.hpp"
#include "math/convex_hull.hpp"

BW_BEGIN_NAMESPACE

EntityUDOFactory s_entityUDOFactory;

WorldApp WorldApp::instance;

int WorldApp_token = 1;

WorldApp::WorldApp() :
    canSeeTerrain_( false ),
    wireFrameStatus_( 0 ),
    debugSortedTriangles_( 0 ),
    pAssetClient_( NULL )
{
    BW_GUARD;
    MainLoopTasks::root().add( this, "World/App", NULL );
}


WorldApp::~WorldApp()
{
    BW_GUARD;
    /*MainLoopTasks::root().del( this, "World/App" );*/
}


bool WorldApp::init()
{
    BW_GUARD;
#if ENABLE_WATCHERS
    DEBUG_MSG( "WorldApp::init: Initially using %d(~%d)KB\n",
        memUsed(), memoryAccountedFor() );
#endif

    DEBUG_MSG( "Terrain: Initialising Terrain Manager...\n" );
    terrainManager_ = TerrainManagerPtr( new Terrain::Manager() );
    MF_ASSERT( terrainManager_->isValid() );

    DataSectionPtr configSection = AppConfig::instance().pRoot();

    DEBUG_MSG( "Task: Initialising BgTaskManager...\n" );
    BgTaskManager::instance().initWatchers( "WorldApp" );
    BgTaskManager::instance().startThreads(
        "WorldApp", 1);

    DEBUG_MSG( "Task: Initialising FileIOTaskManager...\n" );
    FileIOTaskManager::instance().initWatchers( "FileIO" );
    FileIOTaskManager::instance().startThreads(
        "FileIO", 1);

    DEBUG_MSG( "Chunk: Initialising ChunkManager...\n" );
    SpaceManager::instance().init();
    CompiledSpace::CompiledSpace::init( configSection,
        &s_entityUDOFactory, pAssetClient_ );
    ClientChunkSpaceAdapter::init();
    ChunkManager::instance().init( configSection );

    MF_WATCH( "Render/Wireframe Mode",
        WorldApp::instance.wireFrameStatus_,
        Watcher::WT_READ_WRITE,
        "Toggle wire frame mode between none, scenery, terrain and all." );

    MF_WATCH( "Render/Debug Sorted Tris",
        WorldApp::instance.debugSortedTriangles_,
        Watcher::WT_READ_WRITE,
        "Toggle sorted triangle debug mode.  Values of 1 or 2 will display "
        "sorted triangles so you can see how many are being drawn and where." );

//  MF_WATCH( "Draw Wireframe", g_drawWireframe );

    MF_WATCH( "Render/Draw Portals",
        ChunkBoundary::Portal::drawPortals_,
        Watcher::WT_READ_WRITE,
        "Draw portals on-screen as red polygons." );

    MF_WATCH( "Render/Draw Skeletons",
        PyModel::drawSkeletons_,
        Watcher::WT_READ_WRITE,
        "Draw the skeletons of PyModels." );

    MF_WATCH( "Render/Skeleton Draw Shows Local Axes",
        PyModel::drawSkeletonWithCoordAxes_,
        Watcher::WT_READ_WRITE,
        "If true, when PyModel skeletons are drawn their axes are shown. " );

    MF_WATCH( "Render/Draw Node Labels",
        PyModel::drawNodeLabels_,
        Watcher::WT_READ_WRITE,
        "Draw model's node labels?" );

    MF_WATCH( "Render/Plot Foottriggers",
        FootTrigger::plotEnabled_,
        Watcher::WT_READ_WRITE,
        "Plot foot triggers height?" );

    MF_WATCH( "Render/Terrain/draw",
        g_drawTerrain,
        Watcher::WT_READ_WRITE,
        "Enable drawing of the terrain." );

    DEBUG_MSG( "Initialising ForwardDecalsManager...\n" );
    ForwardDecalsManager::init();

    bool ret = false;
    if (!App::instance().isQuiting() &&
        DeviceApp::s_pProgress_ && DeviceApp::s_pStartupProgTask_)
    {
        ret = DeviceApp::s_pStartupProgTask_->step(APP_PROGRESS_STEP) &&
            DeviceApp::s_pProgress_->draw( true );  // for if new loading screen
    }

    return ret;
}


void WorldApp::fini()
{
    BW_GUARD;

    ForwardDecalsManager::fini();
    GlobalEmbodiments::fini();
    EntityManager::instance().fini();
    BgTaskManager::instance().stopAll();
    ChunkManager::instance().fini();
    SpaceManager::instance().fini();
    terrainManager_.reset();
}


void WorldApp::tick( float dGameTime, float dRenderTime )
{
    BW_GUARD_PROFILER( AppTick_World );

    static DogWatch sceneTick( "Scene tick" );
    {
        ScopedDogWatch sdw( sceneTick );
        PROFILER_SCOPED( WorldApp_sceneTick );

        dGameTime_ = dGameTime;

        Renderer::instance().tick( dRenderTime );

        // do the background task manager tick
        BgTaskManager::instance().tick();

        FileIOTaskManager::instance().tick();

        // do the chunk tick now
        ChunkManager::instance().tick( dGameTime );
        SpaceManager::instance().tick( dGameTime );

        // Update terrain lod controller with new info.
        Terrain::BasicTerrainLodController::instance().setCameraPosition( 
            Moo::rc().lodInvView().applyToOrigin(), Moo::rc().lodZoomFactor() );

        // update global models as late as possible, we don't know
        // what kind of matrix providers they'll be using.
        GlobalEmbodiments::tick( dGameTime );
    }
}


void WorldApp::inactiveTick( float dGameTime, float dRenderTime )
{
    BW_GUARD;
    this->tick( dGameTime, dRenderTime );
    ParticleSystemAction::clearLateUpdates();
}


/**
 *  Update the animations of models within all of the world's chunks and the
 *  environment.
 */
void WorldApp::updateAnimations( float dGameTime ) const
{
    BW_GUARD_PROFILER( WorldApp_updateAnimations );

    static DogWatch updateAnimations( "Scene update" );
    ScopedDogWatch sdw( updateAnimations );

    SpaceManager::instance().updateAnimations( dGameTime );

    // Update "tracking" objects after updating all nodes' positions
    LightEmbodimentManager::instance().tick();
    ParticleSystemAction::flushLateUpdates();
}

static DogWatch g_watchCanvasAppDraw("CanvasApp");

void WorldApp::draw()
{
    BW_GUARD_PROFILER( AppDraw_World );

    Moo::DrawContext& colourDrawContext = App::instance().drawContext( App::COLOUR_DRAW_CONTEXT );
    Moo::DrawContext& shadowDrawContext = App::instance().drawContext( App::SHADOW_DRAW_CONTEXT );

    static DogWatch sceneWatch( "Scene" );

    //--
    IRendererPipeline* rp = Renderer::instance().pipeline();

    // Cast Shadows
    rp->beginCastShadows( shadowDrawContext );

    rp->endCastShadows();

    colourDrawContext.resetStatistics();
    colourDrawContext.begin( Moo::DrawContext::ALL_CHANNELS_MASK );
    //-- update all shared constants.
    Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_ALL);
    
    rp->beginOpaqueDraw();

    // draw the design-time scene
    sceneWatch.start();
    Moo::rc().setRenderState( D3DRS_FILLMODE,
        (wireFrameStatus_ & 2) ? D3DFILL_WIREFRAME : D3DFILL_SOLID );

    //-- set up sun lighting.
    ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
    if (pSpace && pSpace->sunLight())
    {
        Moo::SunLight sun;
        sun.m_dir     = pSpace->sunLight()->worldDirection();
        sun.m_color   = pSpace->sunLight()->colour();
        sun.m_ambient = pSpace->ambientLight();

        Moo::rc().effectVisualContext().sunLight(sun);
    }

    {
    PROFILER_SCOPED( MainScene_Draw );
    GPU_PROFILER_SCOPE( MainScene_Draw );
    
#if UMBRA_ENABLE
    if (ChunkManager::instance().umbra()->umbraEnabled())
    {
        Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
        ChunkManager::instance().umbraDraw( colourDrawContext );

        if (wireFrameStatus_ & (1 | 2))
        {
            Moo::rc().device()->EndScene();

            Vector3 bgColour = Vector3( 0, 0, 0 );
            uint32 clearFlags = D3DCLEAR_ZBUFFER;
            if ( Moo::rc().stencilAvailable() )
                clearFlags |= D3DCLEAR_STENCIL;
            Moo::rc().device()->Clear( 0, NULL,
                    clearFlags,
                    Colour::getUint32( bgColour ), 1, 0 );

            Moo::rc().device()->BeginScene();

            Moo::rc().setRenderState( D3DRS_FILLMODE,
                (wireFrameStatus_ & 2) ? D3DFILL_WIREFRAME : D3DFILL_SOLID );

            if (Terrain::BaseTerrainRenderer::instance()->version() == 
                Terrain::TerrainSettings::TERRAIN2_VERSION)
            {
                Terrain::TerrainRenderer2::instance()->zBufferIsClear( true );
            }

            ChunkManager::instance().umbra()->wireFrameTerrain( wireFrameStatus_ & 1 );
            ChunkManager::instance().umbraRepeat( colourDrawContext );
            ChunkManager::instance().umbra()->wireFrameTerrain( 0 );

            if (Terrain::BaseTerrainRenderer::instance()->version() ==
                Terrain::TerrainSettings::TERRAIN2_VERSION)
            {
                Terrain::TerrainRenderer2::instance()->zBufferIsClear( false );
            }
        }
    }
    else
    {
        ChunkManager::instance().draw( colourDrawContext );
    }
#else
    ChunkManager::instance().draw( colourDrawContext );
#endif

    sceneWatch.stop();

    // Looks like we are very close to disabling the ChunkManager::draw 
    // function, not going to disable it yet, because currently working on
    // shadows and don't want to introduce too many changes at once!
    // The following check stops the chunk system from rendering objects
    // twice, now that scene culling is supported.
    ChunkSpacePtr pChunkSpace = ClientChunkSpaceAdapter::getChunkSpace( pSpace );
    bool drawScene = pChunkSpace == NULL;
    if (pSpace && drawScene)
    {
        static DogWatch sceneWatch( "LibScene" );
        ScopedDogWatch sdw( sceneWatch );

        Scene & scene = pSpace->scene();
        SceneDrawContext sceneDrawContext( scene, colourDrawContext );
        IntersectionSet visibleObjects;

        Matrix viewProjectionMat = Moo::rc().viewProjection();
        MF_FREEZE_DATA_WATCHER( Matrix, viewProjectionMat,
            "Render/CompiledSpace/Freeze_CullingViewProjection",
            "Temporarily freeze the frustum used for culling");

        // Only add lights to the LightsManager if we are using the 
        // deferred pipeline
        if(Renderer::instance().pipelineType() == 
            IRendererPipeline::TYPE_DEFERRED_SHADING)
        {
            Moo::LightContainerPtr pLights( new Moo::LightContainer() );
            FrustumHull frustum( viewProjectionMat );
            DrawHelpers::addIntersectingSceneLightsToContainer( scene, frustum.hull(), *pLights );
            LightsManager::instance().gatherVisibleLights( pLights );
        }

        SceneIntersectContext cullingContext( scene );
        DrawHelpers::cull( cullingContext, scene, viewProjectionMat, visibleObjects );
        DrawHelpers::drawSceneVisibilitySet( sceneDrawContext, 
            scene, visibleObjects );
    }

    colourDrawContext.end( Moo::DrawContext::ALL_CHANNELS_MASK );
    {
        GPU_PROFILER_SCOPE( ColourDrawContext_opaqueFlush );
        colourDrawContext.flush( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
    }

    }
    
    {
    PROFILER_SCOPED( Terrain_Draw );
    GPU_PROFILER_SCOPE( Terrain_Draw );

    static DogWatch terrainWatch( "Terrain" );
    terrainWatch.start();
    Moo::rc().setRenderState( D3DRS_FILLMODE, (wireFrameStatus_ & 1) ? D3DFILL_WIREFRAME : D3DFILL_SOLID );

    canSeeTerrain_ = Terrain::BaseTerrainRenderer::instance()->canSeeTerrain();

    Terrain::BaseTerrainRenderer::instance()->drawAll( 
        Moo::RENDERING_PASS_COLOR );

    Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    terrainWatch.stop();
    }

    if (pSpace)
    {
        pSpace->enviro().drawFore( dGameTime_, false, true, false, false, true );
    }

    rp->endOpaqueDraw();

    ForwardDecalsManager::getInstance()->draw();

    //-- do lighting pass.
    rp->applyLighting();


    // Draw the delayed background of our environment.
    // We do this here in order to save fillrate, as we can
    // test the delayed background against the opaque scene that
    // is in the z-buffer.
    if (pSpace)
    {
        static DogWatch enviroWatch( "Enviro" );
        ScopedDogWatch sdw( enviroWatch );
        PROFILER_SCOPED( WorldApp_Enviro );
        // Draw the delayed background of our environment.
        // We do this here in order to save fillrate, as we can
        // test the delayed background against the opaque scene that
        // is in the z-buffer.
        if (pSpace)
        {
            EnviroMinder & enviro = pSpace->enviro();
            // a_radchenko: ideally we want to use another draw context to draw delayed stuff
            enviro.drawHindDelayed( dGameTime_, CanvasApp::instance.drawSkyCtrl_ );

        }
    }

    Entity * pPlayer = ScriptPlayer::entity();
    bool bWasVisible;
    if (pPlayer && pPlayer->pPrimaryModel())
    {
        bWasVisible = pPlayer->pPrimaryModel()->visible( );
        pPlayer->pPrimaryModel()->visible( true );
    }

    Waters::instance().tick( dGameTime_ );

    static DogWatch dTexWatch( "Dynamic Textures" );
    dTexWatch.start();
    TextureRenderer::updateCachableDynamics( dGameTime_ );
    dTexWatch.stop();

    if (pPlayer && pPlayer->pPrimaryModel())
        pPlayer->pPrimaryModel()->visible( bWasVisible );
}


void WorldApp::assetClient( AssetClient* pAssetClient )
{
    pAssetClient_ = pAssetClient;
}

BW_END_NAMESPACE

// world_app.cpp
