#include "pch.hpp"
#include "water_scene_renderer.hpp"

#ifdef EDITOR_ENABLED
#include "gizmo/tool_manager.hpp"
#endif

#include "chunk/chunk_flare.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_tree.hpp"
#include "chunk/chunk_water.hpp"
#include "duplo/py_splodge.hpp"
#include "moo/light_container.hpp"
#include "moo/render_context.hpp"
#include "moo/draw_context.hpp"
#include "moo/renderer.hpp"
#include "terrain/terrain2/terrain_lod_controller.hpp"
#include "terrain/base_terrain_renderer.hpp"
#include "romp/enviro_minder.hpp"
#include "romp/water.hpp"
#include "romp/fog_controller.hpp"
#include "speedtree/speedtree_renderer.hpp"
#include "texture_feeds.hpp"

#include "space/space_manager.hpp"
#include "space/client_space.hpp"
#include "space/deprecated_space_helpers.hpp"

#include "scene/scene_draw_context.hpp"
#include "scene/draw_helpers.hpp"
#include "scene/draw_operation.hpp"
#include "scene/intersection_set.hpp"
#include "scene/scene_intersect_context.hpp"


DECLARE_DEBUG_COMPONENT2( "Water", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Defines
// -----------------------------------------------------------------------------

//#define REFLECTION_FUDGE_FACTOR 0.8f
#define REFLECTION_FUDGE_FACTOR 0.1f
#define REFRACTION_FUDGE_FACTOR 0.5f
#define MAX_CAMERA_DIST 200.f

// -----------------------------------------------------------------------------
// Section: statics
// -----------------------------------------------------------------------------

static float reflectionFudge = 0.2f;
static float reflectionActualFudge = 0.f;
float   WaterSceneRenderer::s_textureSize_ = 1.f;
uint    WaterSceneRenderer::s_maxReflections_ = 20;
uint32  WaterSceneRenderer::reflectionCount_=0;
bool    WaterSceneRenderer::s_drawTrees_ = true;
bool    WaterSceneRenderer::s_drawDynamics_ = true;
bool    WaterSceneRenderer::s_drawPlayer_ = true;
bool    WaterSceneRenderer::s_simpleScene_ = false;
bool    WaterSceneRenderer::s_useClipPlane_ = true;
float   WaterSceneRenderer::s_maxReflectionDistance_ = 25.f;

PyModel*            WaterSceneRenderer::playerModel_;
const PyModel *     WaterSceneRenderer::skyBoxModel_;
WaterScene*         WaterSceneRenderer::currentScene_ = 0;

// -----------------------------------------------------------------------------
// Section: WaterScene
// -----------------------------------------------------------------------------

/**
 *  Constructor.
 */
WaterScene::WaterScene( float height, float waterWaveHeight ) :
    dynamic_(),
    /*drawingReflection_( false ),*/
    waterHeight_( height ),
    waterWaveHeight_(waterWaveHeight),
    camPos_( Vector4::ZERO ),
    reflectionScene_()
{
    BW_GUARD;
    //-- choose format of reflection scene based on the current rendering pipeline.
    D3DFORMAT format = D3DFMT_A8R8G8B8;
    if (Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
        format = D3DFMT_A16B16G16R16F;
    
    reflectionScene_ = new WaterSceneRenderer( this, true, format );
}


/**
 *  Destruction
 */
WaterScene::~WaterScene()
{
    BW_GUARD;
    TextureRenderer::delStaggeredDynamic( this );
    bw_safe_delete(reflectionScene_);
}


/**
 *  Recreate the render targets for this scene.
 */
bool WaterScene::recreate( )
{
    BW_GUARD;
    if (reflectionScene_->recreate())
    {
        staggeredDynamic(true);
        return true;        
    }
    return false;
}


/**
 *  Determine if the scene should be drawn.
 */
bool WaterScene::checkOwners() const
{
    BW_GUARD;
    OwnerList::const_iterator it = owners_.begin();

    for (;it != owners_.end(); it++)
    {
        if ((*it) && (*it)->canReflect())
        {
            return true;
        }
    }
    return false;
}

/**
 * This method gets the chunks that the waterscene is visible from
 * @param visibleChunks a vector of chunks to return the visible chunks in
 * @param outsideVisible return whether or not the outside chunks are visible
 * @param verts the vertices of the visible water volumes
 * @return true if there is anything to draw
 */
bool WaterScene::getVisibleChunks( BW::vector<Chunk*>& visibleChunks, bool& outsideVisible, BW::vector<Vector3>& verts )
{
    BW_GUARD;
    bool ret = false;
    outsideVisible = false;

    // Iterate over all the ownsers of this WaterScene and work out which chunks
    // it can be seen from
    OwnerList::const_iterator it = owners_.begin();
    for (;it != owners_.end(); it++)
    {
        const Water* pWater = *it;

        if (pWater && pWater->canReflect())
        {
            visibleChunks.insert( visibleChunks.end(),
                pWater->visibleChunks().begin(),
                pWater->visibleChunks().end() );

            if (pWater->outsideVisible())
            {
                outsideVisible = true;
            }

            const Vector3& min = pWater->bb().minBounds();
            const Vector3& max = pWater->bb().maxBounds();
            const Matrix& m = pWater->transform();

            verts.push_back( m.applyPoint( Vector3( min.x, 0, min.z ) ) );
            verts.push_back( m.applyPoint( Vector3( min.x, 0, max.z ) ) );
            verts.push_back( m.applyPoint( Vector3( max.x, 0, max.z ) ) );
            verts.push_back( m.applyPoint( Vector3( max.x, 0, min.z ) ) );

            ret = true;
        }
    }

    return ret;
}


/**
 * Retrieve the reflection texture.
 */
Moo::RenderTargetPtr WaterScene::reflectionTexture()
{
    BW_GUARD;
    return reflectionScene_->getTexture();
}


/**
 * Destroy the unmanaged objects
 */
void WaterScene::deleteUnmanagedObjects()
{
    BW_GUARD;
    if (reflectionScene_)
    {
        reflectionScene_->deleteUnmanagedObjects();
    }
}


/**
 * Destroy the unmanaged objects
 */
void WaterSceneRenderer::deleteUnmanagedObjects()
{
    BW_GUARD;

    if (pRT_)
    {
        TextureFeeds::delTextureFeed( sceneName_ );
        pRT_->release();
        pRT_ = NULL;    
    }
}


/**
 * Test a point is close to the water.
 */
bool WaterScene::closeToWater( const Vector3 & pos )
{
    BW_GUARD;
    bool closeToWater=false;
    OwnerList::iterator it = owners_.begin();

    for (;it != owners_.end() && closeToWater==false; it++)
    {
        const Vector3& p = (*it)->position();
        float dist = (p - pos).length() - (*it)->size().length() * 0.5f;

        closeToWater = (dist < WaterSceneRenderer::s_maxReflectionDistance_);
    }

    return closeToWater;
}


/**
 * Check if the given model should be drawn in the water scene
 */
bool WaterScene::shouldReflect( const Vector3 & pos, PyModel * model )
{
    BW_GUARD;
    return (reflectionScene_->shouldReflect( pos, model ) &&
            this->closeToWater( pos ));
}


/**
 * Check the water surfaces are drawing
 */
bool WaterScene::shouldDraw() const
{
    return this->checkOwners();
}


/**
 * Render the water scene textures, refraction first then reflection.
 */
void WaterScene::render( float dTime )
{
    BW_GUARD;
#if ENABLE_WATCHERS
    if(!Waters::instance().isVisible) 
        return;
#endif

    if (Waters::drawReflection())
    {
        reflectionScene_->render( dTime );
    }
}


#if defined( EDITOR_ENABLED )
/**
 * See if any of the water surfaces are using the read only red mode.
 */
bool WaterScene::drawRed()
{
    BW_GUARD;
    OwnerList::const_iterator it = owners_.begin();

    for (;it != owners_.end(); it++)
    {
        if ((*it)->drawRed())
        {
            return true;
        }
    }

    return false;
}


/**
 * See if any of the water surfaces are require highlighting.
 */
bool WaterScene::highlight()
{
    BW_GUARD;
    OwnerList::const_iterator it = owners_.begin();

    for (;it != owners_.end(); it++)
    {
        if ((*it)->highlight())
        {
            return true;
        }
    }

    return false;
}
#endif //EDITOR_ENABLED


/**
 * Tell the texture renderer management to treat this as a full dynamic
 */
void WaterScene::dynamic( bool isDynamic )
{
    BW_GUARD;
    dynamic_ = isDynamic;

    if (isDynamic)
    {
        TextureRenderer::addDynamic( this );
    }
    else
    {
        TextureRenderer::delDynamic( this );
    }
}


/**
 * Tell the texture renderer management to treat this as a staggered dynamic
 */
void WaterScene::staggeredDynamic( bool state )
{
    BW_GUARD;
    dynamic_ = state;

    if (state)
    {
        TextureRenderer::addStaggeredDynamic( this );
    }
    else
    {
        TextureRenderer::delStaggeredDynamic( this );
    }
}


// -----------------------------------------------------------------------------
// Section: WaterSceneRenderer
// -----------------------------------------------------------------------------


/**
 *  Constructor.
 */
WaterSceneRenderer::WaterSceneRenderer( WaterScene* scene, bool reflect, D3DFORMAT format ) :   
    TextureRenderer( 512, 512, format ), //default sizes
    scene_( scene ),
    reflection_( reflect ),
    eyeUnderWater_( false )
{
    BW_GUARD;
    //we don't want the texture renderer to clear our render target.
    clearTargetEachFrame_ = false;  

    static bool firstTime = true;

    s_useClipPlane_ = true;
    if(scene)
        reflectionActualFudge = scene->waterWaveHeight();

    if (firstTime)
    {
        MF_WATCH( "Client Settings/Water/Draw Trees",
                s_drawTrees_, Watcher::WT_READ_WRITE,
                "Draw trees in water scenes?" );
        MF_WATCH( "Client Settings/Water/Draw Dynamics",
                s_drawDynamics_, Watcher::WT_READ_WRITE,
                "Draw dynamic objects in water scenes?" );
        MF_WATCH( "Client Settings/Water/Draw Player",
                s_drawPlayer_, Watcher::WT_READ_WRITE,
                "Draw the players reflection/refraction?" );
        MF_WATCH( "Client Settings/Water/Max Reflections",
                s_maxReflections_, Watcher::WT_READ_WRITE,
                "Maximum number of dynamic objects to reflect/refract" );
        MF_WATCH( "Client Settings/Water/Max Reflection Distance",
                s_maxReflectionDistance_, Watcher::WT_READ_WRITE,
                "Maximum distance a dynamic object will reflect/refract" );
        MF_WATCH( "Client Settings/Water/Scene/Use Clip Plane",
                s_useClipPlane_, Watcher::WT_READ_WRITE,
                "Clip the water reflection/refraction along the water plane?" );

        MF_WATCH( "Client Settings/Water/Scene/Reflection Fudge Actual",
                reflectionActualFudge, Watcher::WT_READ_WRITE,
                "2nd Fudge factor for the clip planes of reflection" ); 

        MF_WATCH( "Client Settings/Water/Scene/Reflection Fudge",
                reflectionFudge, Watcher::WT_READ_WRITE,
                "Fudge factor for the clip planes of reflection" ); 
        MF_WATCH( "Client Settings/Water/Scene/Cull Distance",
                Water::s_sceneCullDistance_, Watcher::WT_READ_WRITE,
                "Distance where a water scene will stop being updated" );
        MF_WATCH( "Client Settings/Water/Scene/Cull Fade Distance", 
                Water::s_sceneCullFadeDistance_, Watcher::WT_READ_WRITE,
                "Distance which to fade the updating of the water scene up to "
                    "the point where it doesn't get updated" );

        firstTime=false;
    }

    this->createRenderTarget();

    waterDrawContext_ = new Moo::DrawContext( Moo::RENDERING_PASS_REFLECTION );
}


/**
 *  Destructor.
 */
WaterSceneRenderer::~WaterSceneRenderer()
{
    BW_GUARD;
    deleteUnmanagedObjects();
    //pRT_=NULL;
    //TextureRenderer::delStaggeredDynamic( this );
    bw_safe_delete( waterDrawContext_ );
}


/**
 *  Recreate the render targets for this scene.
 */
bool WaterSceneRenderer::recreate( )
{
    BW_GUARD;
    if (!pRT_)
    {
        this->createRenderTarget();
    }

    bool success = false;
    pRT_->clearOnRecreate( true, Moo::Colour(1,1,1,0) );
    //-- Render target size is now based on the frame-buffer size.
    success = pRT_->create(
        uint( float( Moo::rc().screenWidth() ) * s_textureSize_ ),
        uint( float( Moo::rc().screenHeight() ) * s_textureSize_ ),
        false , format_, NULL, D3DFMT_D16 
        );
    return success;
}


/**
 *  Determine if a certain model should be rendered.
 */
//TODO: only render objects close to the water AND close to the camera??
//TODO: optimise more...
bool WaterSceneRenderer::shouldReflect( const Vector3 & pos, PyModel * model )
{
    BW_GUARD;
    bool shouldReflect;

    
    if (skyBoxModel() == model)
    {
        // Sky box distance is meaningless. Should draw them always.
        return true;
    }
    // TODO: this will still draw all instances of the player model..fix!!!
    else if (playerModel() == model)
    {
        shouldReflect = s_drawPlayer_;
    }
    else
    {
        shouldReflect = ((reflectionCount_ < s_maxReflections_) &&
                            s_drawDynamics_);
    }

    Vector4 camPos = Moo::rc().invView().row( 3 );
    float camDist = (Vector3( camPos.x,camPos.y,camPos.z ) - pos).length();
    shouldReflect = shouldReflect && (camDist < MAX_CAMERA_DIST);

    return (shouldReflect);
}


/**
 *  Determine if the water scene should be drawn.
 */
bool WaterSceneRenderer::shouldDraw() const
{
    return (reflection_);
}


/**
 *
 */
bool WaterScene::eyeUnderWater() const
{
    return reflectionScene_->eyeUnderWater_;
}


/**
 *  Draw ourselves
 */
void WaterSceneRenderer::renderSelf( float dTime )
{
    BW_GUARD_PROFILER( WaterSceneRenderer_renderSelf );
    ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();

    if (!pSpace.exists())
    {
        return;
    }

    Moo::rc().setWriteMask( 1, 0 );

#if EDITOR_ENABLED
    if (scene()->drawRed())
    {
        Moo::rc().device()->Clear(
            0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0x0, 1.f, 0 );
        return;
    }
#endif

    waterDrawContext_->begin( Moo::DrawContext::ALL_CHANNELS_MASK );

    bool isCameraUnderWater = Waters::instance().insideVolume();

    // Determine if fog should be disabled, saving previous state for restoring
    bool oldFogEnabled = FogController::instance().enable();
    if ( oldFogEnabled && isCameraUnderWater )
    {
        FogController::instance().enable( false );
        FogController::instance().updateFogParams();
    }

    static DogWatch w3( "WaterScene" );
    w3.start();

    //first, setup some rendering options.
    //we need to draw as cheaply as possible, and
    //not clobber the main pipeline.
    ChunkWater::simpleDraw( true );
    ChunkFlare::ignore( true );
    PySplodge::s_ignoreSplodge_ = true;

#if SPEEDTREE_SUPPORT
    float oldMaxLod = speedtree::SpeedTreeRenderer::maxLod( 0.1f );
#endif

    //TODO: turn on low LOD 
    //TODO: turn off attachments? or base them on their owner?

    scene()->setCameraPos(Moo::rc().invView().row( 3 ));
    bool flipScene = reflection_;
    eyeUnderWater_ = (scene()->cameraPos().y < waterHeight());

    Moo::rc().reflectionScene( true );
    this->currentScene( this->scene() );

    float oldFOV = Moo::rc().camera().fov();    
    float oldFar = Moo::rc().camera().farPlane();

    //save some stuff before we stuff around ( haha )
    Matrix invView = Moo::rc().invView();

    D3DXPLANE clipPlaneOffset, clipPlaneActual;

    if (flipScene)
    {
        // flip the scene
        // 1   0   0   0
        // 0   1   0   0
        // 0   0  -1  2h
        // 0   0   0   1
        if (eyeUnderWater_)
        {
            clipPlaneOffset = D3DXPLANE( 0.f, -1.f, 0.f, (waterHeight() + reflectionFudge) ); //including error fudgery factor.

            clipPlaneActual = D3DXPLANE(0.f, -1.f, 0.f, (waterHeight() + reflectionActualFudge)); //including error fudgery factor.
        }
        else
        {
            clipPlaneOffset = D3DXPLANE(0.f, 1.f, 0.f, -(waterHeight() - reflectionFudge)); //including error fudgery factor.
            clipPlaneActual = D3DXPLANE(0.f, 1.f, 0.f, -(waterHeight() - reflectionActualFudge)); //including error fudgery factor.
        }

        Matrix refMatrix;
        refMatrix.setTranslate( 0.f, this->waterHeight() * 2.f, 0.f ); //water height*2     
        refMatrix._22 = -1.0f; // flip the scene

        Matrix view = Moo::rc().view();
        view.preMultiply( refMatrix );

        Moo::rc().view( view );
        Moo::rc().mirroredTransform( true );
    }
    else
    {
        if (eyeUnderWater_)
        {
            clipPlaneOffset = D3DXPLANE(0.f, 1.f, 0.f, -waterHeight() + REFRACTION_FUDGE_FACTOR); //including error fudgery factor.
            clipPlaneActual = D3DXPLANE(0.f, 1.f, 0.f, -waterHeight() + 0.1f); //including error fudgery factor.
        }
        else
        {
            clipPlaneOffset = D3DXPLANE(0.f, -1.f, 0.f, waterHeight() + REFRACTION_FUDGE_FACTOR); //including error fudgery factor.
            clipPlaneActual = D3DXPLANE(0.f, -1.f, 0.f, waterHeight() + 0.1f); //including error fudgery factor.
        }
    }
    Matrix world = Moo::rc().world();
    Moo::rc().world( Matrix::identity );

    DX::Viewport oldViewport;
    Moo::rc().getViewport( &oldViewport );

    DX::Viewport newViewport = oldViewport;
    //if (reflection_)
    {
        newViewport.Height = pRT_->height();
        newViewport.Width = pRT_->width();

        //newViewport.Height +=  underWater ? 0 : -2;

        Moo::rc().setViewport( &newViewport );      
        Moo::rc().screenWidth( newViewport.Width );
        Moo::rc().screenHeight( newViewport.Height );
    }
    /*else
    {
        newViewport.Width = pRT_->width();
        Moo::rc().setViewport( &newViewport );
        Moo::rc().screenWidth( newViewport.Width  );
        Moo::rc().screenHeight( newViewport.Height );
    }*/
    Moo::rc().updateProjectionMatrix();
    Moo::rc().updateViewTransforms();

    D3DXPLANE transformedClipPlaneOffset, transformedClipPlaneActual;

    // clip plane
    if (s_useClipPlane_ && !s_simpleScene_)
    {
        //TODO: optimise
        Matrix worldViewProjectionMatrix = Moo::rc().viewProjection();
        worldViewProjectionMatrix.preMultiply( world );

        Matrix worldViewProjectionInverseTransposeMatrix;
        D3DXMatrixInverse((D3DXMATRIX*)&worldViewProjectionInverseTransposeMatrix, NULL, (D3DXMATRIX*)&worldViewProjectionMatrix);
        D3DXMatrixTranspose((D3DXMATRIX*)&worldViewProjectionInverseTransposeMatrix,(D3DXMATRIX*)&worldViewProjectionInverseTransposeMatrix);       

        D3DXPlaneTransform(&transformedClipPlaneOffset, &clipPlaneOffset, (D3DXMATRIX*)&worldViewProjectionInverseTransposeMatrix);
        D3DXPlaneTransform(&transformedClipPlaneActual, &clipPlaneActual, (D3DXMATRIX*)&worldViewProjectionInverseTransposeMatrix);

        Moo::rc().device()->SetClipPlane(0, transformedClipPlaneActual);
        Moo::rc().setRenderState(D3DRS_CLIPPLANEENABLE, 1);
    }

    //draw the scene
#ifdef EDITOR_ENABLED
    Moo::rc().device()->Clear(
        0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0x0, 1.f, 0 );
#else
    Moo::rc().device()->Clear(
        0, NULL, D3DCLEAR_ZBUFFER, 0x0, 1.f, 0 );
#endif

    ChunkSpacePtr pChunkSpace = ChunkManager::instance().space( pSpace->id(), false );
    if (pChunkSpace)
    {
        // Override the camera chunk (to force correct reflection calculation)
        Chunk * cc=NULL;
        Vector3 pp = invView.applyToOrigin() + Moo::rc().camera().nearPlane() *
            invView.applyToUnitAxisVector( Z_AXIS );
        cc = pChunkSpace->findChunkFromPoint( pp );

        ChunkManager::instance().camera( Moo::rc().invView(), pChunkSpace, cc );
    }

#if SPEEDTREE_SUPPORT
    bool drawTrees = speedtree::SpeedTreeRenderer::drawTrees( s_drawTrees_ );
#endif

    //-- update per-view, per-screen, and per frame constants.
    Moo::rc().effectVisualContext().updateSharedConstants( 
        Moo::CONSTANTS_PER_SCREEN |     // reflection render target size
        Moo::CONSTANTS_PER_VIEW |       // view projection matrix
        Moo::CONSTANTS_PER_FRAME        // fog params
    );

    clearReflectionCount();

    Scene & scene = pSpace->scene();
    SceneDrawContext drawContext( scene, *waterDrawContext_ );
    drawContext.renderFlares( false );

    if (!s_simpleScene_)
    {
        if (pChunkSpace)
        {
            static BW::vector< Chunk * > visibleChunks;
            visibleChunks.clear();
            static BW::vector< Vector3 > verts;
            verts.clear();
            bool outsideVisible = false;

            if (scene_->getVisibleChunks( visibleChunks, outsideVisible, verts ))
            {
                float nearPoint = Moo::rc().camera().farPlane();

                for (uint32 i = 0; i < verts.size(); i++)
                {
                    Vector4 val;
                    Moo::rc().viewProjection().applyPoint( val, verts[i] );
                    nearPoint = std::min( val.w, nearPoint );
                }

                nearPoint = std::max( Moo::rc().camera().nearPlane(), nearPoint );

                ChunkManager::instance().drawReflection( *waterDrawContext_,
                    visibleChunks, outsideVisible, nearPoint );
            }
        }
        else
        {
            SceneIntersectContext cullingContext( scene );
            IntersectionSet visibleObjects;
            DrawHelpers::cull( cullingContext, scene, Moo::rc().viewProjection(), visibleObjects );
            DrawHelpers::drawSceneVisibilitySet( drawContext, scene, visibleObjects );
        }
    }

    waterDrawContext_->end( Moo::DrawContext::ALL_CHANNELS_MASK );
    waterDrawContext_->flush( Moo::DrawContext::OPAQUE_CHANNEL_MASK );

    if (s_useClipPlane_ && !s_simpleScene_)
    {
        Moo::rc().device()->SetClipPlane( 0, transformedClipPlaneOffset );
    }

    // Render the terrain
    if (s_simpleScene_ && !eyeUnderWater_)
    {
        Terrain::BaseTerrainRenderer::instance()->clearBlocks();
    }
    else
    {
        BW_SCOPED_DOG_WATCHER("Terrain");

        Terrain::BaseTerrainRenderer::instance()->drawAll(
            Moo::RENDERING_PASS_REFLECTION
            );
    }

    pSpace->enviro().drawHindDelayed( dTime );  


#if EDITOR_ENABLED
    // draw tools
    ToolPtr spTool = ToolManager::instance().tool();
    if (spTool)
    {
        waterDrawContext_->flush( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK |
            Moo::DrawContext::SHIMMER_CHANNEL_MASK );
        // TODO_DRAW_CONTEXT: tools should be using it's own context
        waterDrawContext_->begin( Moo::DrawContext::ALL_CHANNELS_MASK );
        spTool->render( *waterDrawContext_);
        waterDrawContext_->end( Moo::DrawContext::ALL_CHANNELS_MASK );
        waterDrawContext_->flush( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
    }
#endif
    waterDrawContext_->flush( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK |
        Moo::DrawContext::SHIMMER_CHANNEL_MASK );

    if (s_useClipPlane_)
    {
        Moo::rc().setRenderState( D3DRS_CLIPPLANEENABLE, 0 );
    }

    //set the stuff back
    Moo::rc().setViewport( &oldViewport );
    Moo::rc().screenWidth( oldViewport.Width );
    Moo::rc().screenHeight( oldViewport.Height );

    //restore drawing states!
    ChunkWater::simpleDraw( false );
    ChunkFlare::ignore( false );
    PySplodge::s_ignoreSplodge_ = false;

    if (pChunkSpace)
    {
        ChunkManager::instance().camera( invView, pChunkSpace );
    }

    //speedtree::SpeedTreeRenderer::lodMode(oldMode);
#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::drawTrees(drawTrees);
#endif

    Moo::rc().updateProjectionMatrix();

    Moo::rc().mirroredTransform(false);
    Moo::rc().reflectionScene(false);

#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::maxLod( oldMaxLod );
#endif

    // Revert fog enable.
    if (!oldFogEnabled == FogController::instance().enable())
    {
        FogController::instance().enable( oldFogEnabled );
        FogController::instance().updateFogParams();
        Moo::rc().effectVisualContext().updateSharedConstants( Moo::CONSTANTS_PER_FRAME );
    }

    w3.stop();
}


/**
 *
 */
void WaterSceneRenderer::createRenderTarget()
{
    BW_GUARD;
    static IncrementalNameGenerator sceneTargetNames( "WaterSceneTarget" );
    static IncrementalNameGenerator sceneNames( "waterScene" );

    pRT_ = new Moo::RenderTarget( sceneTargetNames.nextName() );    
    PyTextureProviderPtr feed = PyTextureProviderPtr(
        new PyTextureProvider( NULL, pRT_ ), true );

    sceneName_ = sceneNames.nextName();

    TextureFeeds::addTextureFeed( sceneName_, feed );
}

BW_END_NAMESPACE

// water_scene_renderer.cpp
