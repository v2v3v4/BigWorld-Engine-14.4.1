#include "pch.hpp"
#include <pyport.h>
#include "me_module.hpp"

#include "appmgr/closed_captions.hpp"
#include "appmgr/module_manager.hpp"
#include "appmgr/options.hpp"
#include "ashes/simple_gui.hpp"
#include "chunk/chunk_item_amortise_delete.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "gizmo/gizmo_manager.hpp"
#include "gizmo/tool_manager.hpp"
#include "main_frm.h"
#include "math/planeeq.hpp"
#include "me_app.hpp"
#include "me_shell.hpp"
#include "me_consts.hpp"
#include "terrain/base_terrain_renderer.hpp"
#include "moo/effect_visual_context.hpp"
#include "terrain/terrain2/terrain_lod_controller.hpp"
#include "moo/texture_manager.hpp"
#include "moo/draw_context.hpp"
#include "page_materials.hpp"
#include "page_object.hpp"
#include "particle/actions/particle_system_action.hpp"
#include "pyscript/py_callback.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "moo/custom_mesh.hpp"
#include "romp/flora.hpp"
#include "moo/geometrics.hpp"
#include "moo/texture_renderer.hpp"
#include "romp/time_of_day.hpp"
#include "tools/common/romp_harness.hpp"
#include "tools/common/tools_camera.hpp"
#include "speedtree/speedtree_renderer.hpp"
#include "ual/ual_manager.hpp"
#include "fmodsound/sound_manager.hpp"

#include "modeleditor_core/Models/lights.hpp"
#include "modeleditor_core/Models/mutant.hpp"

#include "moo/renderer.hpp"
#include "moo/graphics_settings.hpp"
#include "romp/hdr_settings.hpp"
#include "romp/ssao_settings.hpp"
#include "moo/complex_effect_material.hpp"
#include "moo/complex_effect_material.hpp"
#include "terrain/terrain2/editor_terrain_block2.hpp"

DECLARE_DEBUG_COMPONENT2( "Shell", 0 )

BW_BEGIN_NAMESPACE

static DogWatch s_detailTick( "detail_tick" );
static DogWatch s_detailDraw( "detail_draw" );

// Here are all our tokens to get the various components to link
extern int ChunkModel_token;
extern int ChunkLight_token;
extern int ChunkTerrain_token;
extern int ChunkFlare_token;
extern int ChunkWater_token;
extern int ChunkTree_token;
extern int ChunkParticles_token;

static int s_chunkTokenSet = ChunkModel_token | ChunkLight_token |
    ChunkTerrain_token | ChunkFlare_token | ChunkWater_token |
    ChunkTree_token | ChunkParticles_token;

extern int genprop_gizmoviews_token;
static int giz = genprop_gizmoviews_token;

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

//--------------------------------------------------------------------------------------------------

namespace 
{
    //-- Code of those functions is stolen (by copy and paste) from 
    //-- ual/model_thumb_prov.cpp.
    static void zoomToExtents( const BoundingBox& bb, const float scale = 1.f )
    {
        BW_GUARD;

        Vector3 bounds = bb.maxBounds() - bb.minBounds();

        float radius = bounds.length() / 2.f;

        if (radius < 0.01f) 
        {
            return;
        }

        float dist = radius / tanf( Moo::rc().camera().fov() / 2.f );

        // special case to avoid near 
        // plane clipping of small objects
        if (Moo::rc().camera().nearPlane()  > dist - radius)
        {
            dist = Moo::rc().camera().nearPlane() + radius;
        }

        Matrix view = Moo::rc().view();
        view.invert();
        Vector3 forward = view.applyToUnitAxisVector( 2 );
        view.invert();

        Vector3 centre(
            (bb.minBounds().x + bb.maxBounds().x) / 2.f,
            (bb.minBounds().y + bb.maxBounds().y) / 2.f,
            (bb.minBounds().z + bb.maxBounds().z) / 2.f );

        Vector3 pos = centre - scale * dist * forward;
        view.lookAt( pos, forward, Vector3( 0.f, 1.f, 0.f ) );
        Moo::rc().view( view );
    }

    static void renderThumbnail()
    {
        // WARNING: This functionality is duplicated inside 
        // ModelThumbProvider::render(). There are already discrepancies
        // Ideally this function would be rewritten in terms of the other 
        // implementation. However, it seems that people have been modifying 
        // THIS implementation to get the behavior they want.

        Moo::rc().beginScene();

        Matrix rotation;

        //-- prepare lighting.
        Moo::SunLight savedSun = Moo::rc().effectVisualContext().sunLight();
        Moo::SunLight sun = savedSun;

        EnviroMinder &enviro = ChunkManager::instance().cameraSpace()->enviro();

        Moo::rc().effectVisualContext().sunLight(sun);
        Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_FRAME);

        //Make sure we set this before we try to draw
        Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

        Moo::rc().device()->Clear( 0, NULL,
            D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, RGB( 192, 192, 192 ), 1, 0 );

        // Set the projection matrix
        Moo::Camera cam = Moo::rc().camera();
        cam.aspectRatio( 1.f );
        Moo::rc().camera( cam );
        Moo::rc().updateProjectionMatrix();

        // camera should look along +Z axis
        //// Set a standard view
        //Matrix view (Matrix::identity);
        //Moo::rc().world( view );
        //rotation.setRotateX( - MATH_PI / 8.f );
        //view.preMultiply( rotation );
        //rotation.setRotateY( + MATH_PI / 8.f );
        //view.preMultiply( rotation );
        //Moo::rc().view( view );

        // Zoom to the models bounding box
        zoomToExtents( MeApp::instance().mutant()->modelBoundingBox() );

        Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_VIEW);

        // Draw the model
        Moo::DrawContext    thumbnailDrawContext(
            Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING ?
            Moo::RENDERING_PASS_REFLECTION : Moo::RENDERING_PASS_COLOR );

        thumbnailDrawContext.begin( Moo::DrawContext::ALL_CHANNELS_MASK );
        thumbnailDrawContext.pushImmediateMode();

        MeApp::instance().mutant()->render( thumbnailDrawContext, 0.f, SHOW_MODEL );

        thumbnailDrawContext.popImmediateMode();
        thumbnailDrawContext.end( Moo::DrawContext::ALL_CHANNELS_MASK );
        thumbnailDrawContext.flush( Moo::DrawContext::ALL_CHANNELS_MASK );

        //Make sure we restore this after we are done
        Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

        //-- reset global rendering state.
        Moo::rc().effectVisualContext().sunLight(savedSun);
        Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_FRAME);

        Moo::rc().endScene();
    }
}

//--------------------------------------------------------------------------------------------------

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

IMPLEMENT_CREATOR(MeModule, Module);

static AutoConfigString s_light_only( "system/lightOnlyEffect" ); 

static bool s_enableScripting = true;
static const float NUM_FRAMES_AVERAGE = 16.f;

namespace {
    enum SettingsValue {
        UNINITIALISED = -1,
    };
}   // anonymous namespace


MeModule* MeModule::s_instance_ = NULL;

class WireframeMaterialOverride : public Moo::DrawContext::OverrideBlock
{
public:
    WireframeMaterialOverride():
        Moo::DrawContext::OverrideBlock( s_light_only.value().substr( 0, s_light_only.value().rfind(".") ), true )
    {
    }
};

MeModule::MeModule()
    : selectionStart_( Vector2::zero() )
    , currentSelection_( GridRect::zero() )
    , localToWorld_( GridCoord::zero() )
    , averageFps_(0.0f)
    , scriptDict_(NULL)
    , renderingThumbnail_(false)
    , materialPreviewMode_(false)
    //, pShadowCommon_( NULL )
    , rt_( NULL )
    , lastShadowsOn_(true)
    , lastHDREnabled_(SettingsValue::UNINITIALISED)
    , lastSSAOEnabled_(SettingsValue::UNINITIALISED)
    , lastShadowsQuality_(SettingsValue::UNINITIALISED)
    , lastDTime_( 0.0f )
    , mainFrame_( NULL )
    , editorReadyCallback_()
{
    BW_GUARD;

    ASSERT(s_instance_ == NULL);
    s_instance_ = this;
    REGISTER_SINGLETON( MeModule )

    lastCursorPosition_.x = 0;
    lastCursorPosition_.y = 0;
}

MeModule::~MeModule()
{
    BW_GUARD;

    ASSERT(s_instance_);
    delete wireframeRenderer_;
    s_instance_ = NULL;
}

bool MeModule::init( DataSectionPtr pSection )
{
    BW_GUARD;

    //Make a MaterialDraw override for wireframe rendering
    wireframeRenderer_ = new WireframeMaterialOverride;

    // Set callback for PyScript so it can know total game time
    Script::setTotalGameTimeFn( getTotalTime );
    return true;
}

void MeModule::onStart()
{
    BW_GUARD;

    if (s_enableScripting)
        initPyScript();

    cc_ = new ClosedCaptions( 
        Options::getOptionInt( "consoles/numMessageLines", 5 ) );
    Commentary::instance().addView( &*cc_ );
    cc_->visible( true );
}


bool MeModule::initPyScript()
{
    BW_GUARD;

    // make a python dictionary here
    PyObject * pScript = PyImport_ImportModule("ModelEditorDirector");

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


bool MeModule::finiPyScript()
{
    BW_GUARD;

    // make a python dictionary here
    PyObject * pScript = PyImport_ImportModule("ModelEditorDirector");

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



int MeModule::onStop()
{
    BW_GUARD;

    ::ShowCursor( 0 );

    if (s_enableScripting)
        finiPyScript();

    //TODO: Work out why I can't call this here...
    //Save all options when exiting
    //Options::save();

    Py_DecRef(cc_.getObject());
    cc_ = NULL;

    rt_ = NULL;

    return 0;
}


bool MeModule::updateState( float dTime )
{
    BW_GUARD;

    s_detailTick.start();

    static float s_lastDTime = 1.f/60.f;
    if (dTime != 0.f)
    {
        dTime = dTime/NUM_FRAMES_AVERAGE + (NUM_FRAMES_AVERAGE-1.f)*s_lastDTime/NUM_FRAMES_AVERAGE;
        s_lastDTime = dTime;
    }

    MeShell::instance().renderer()->tick(dTime);

    //Update the commentary
    cc_->update( dTime );
    
    SimpleGUI::instance().update( dTime );

    // update the camera.  this interprets the view direction from the mouse input
    MeApp::instance().camera()->update( dTime, true );

    // tick time and update the other components, such as romp
    MeShell::instance().romp().update( dTime, false );

    // gizmo manager
    Vector2 cursorPos = mainFrame_->currentCursorPosition();
    Vector3 worldRay = mainFrame_->getWorldRay( ( int ) cursorPos.x, ( int ) cursorPos.y);
    ToolPtr spTool = ToolManager::instance().tool();
    if (spTool)
    {
        spTool->calculatePosition( worldRay );
        spTool->update( dTime );
    }
    else if (GizmoManager::instance().update(worldRay))
        GizmoManager::instance().rollOver();

    // set input focus as appropriate
    bool acceptInput = mainFrame_->cursorOverGraphicsWnd();
    app_->handleSetFocus( acceptInput );
        
    ChunkManager::instance().tick( dTime );
    AmortiseChunkItemDelete::instance().tick();
    BgTaskManager::instance().tick();
    FileIOTaskManager::instance().tick();
    ProviderStore::tick( dTime );
    incrementTotalTime( dTime );
    Script::tick( getTotalTime() );

#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::tick( dTime );
#endif

#if FMOD_SUPPORT
    //Tick FMod by setting the camera position
    Matrix view = MeApp::instance().camera()->view();
    view.invert();
    Vector3 cameraPosition = view.applyToOrigin();
    Vector3 cameraDirection = view.applyToUnitAxisVector( 2 );
    Vector3 cameraUp = view.applyToUnitAxisVector( 1 );
    SoundManager::instance().setListenerPosition(
        cameraPosition, cameraDirection, cameraUp, dTime );
#endif // FMOD_SUPPORT

    s_detailTick.stop();

    IRendererPipeline* rendererPipeline = Renderer::instance().pipeline();
    bool isDeffered = Renderer::instance().pipelineType() == 
        IRendererPipeline::TYPE_DEFERRED_SHADING;
    rendererPipeline->tick(dTime);

    return true;
}

void MeModule::beginRender()
{
    BW_GUARD;

    Moo::Camera cam = Moo::rc().camera();
    cam.nearPlane( Options::getOptionFloat( "render/misc/nearPlane", 0.01f ));
    cam.farPlane( Options::getOptionFloat( "render/misc/farPlane", 300.f ));
    Moo::rc().camera( cam );
    
    if (Moo::rc().mixedVertexProcessing())
        Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );


    Moo::rc().reset();
    Moo::rc().updateProjectionMatrix();
    Moo::rc().updateViewTransforms();
    Moo::rc().updateLodViewMatrix();
}

bool MeModule::renderThumbnail( const BW::string& fileName )
{
    BW_GUARD;

    BW::string modelName = BWResource::resolveFilename( fileName );
    std::replace( modelName.begin(), modelName.end(), '/', '\\' );
    BW::wstring screenShotName = bw_utf8tow( BWResource::removeExtension( modelName ).to_string() );
    screenShotName += L".thumbnail.jpg";
        
    if (!rt_)
    {
        rt_ = new Moo::RenderTarget( "Model Thumbnail" );
        if (!rt_ || !rt_->create( 128, 128 ))
        {
            WARNING_MSG( "Could not create render target for model thumbnail.\n" );
            rt_ = NULL;
            return false;
        }
    }
    
    if ( rt_->push() )
    {
        updateFrame( 0.f );
        
        BW::renderThumbnail();

        HRESULT hr = D3DXSaveTextureToFile( screenShotName.c_str(),
            D3DXIFF_JPG, rt_->pTexture(), NULL );

        rt_->pop();

        if ( FAILED( hr ) )
        {
            WARNING_MSG( "Could not create model thumbnail \"%s\" (D3D error 0x%x).\n", screenShotName.c_str(), hr);
            return false;
        }

        PageObject::currPage()->OnUpdateThumbnail();
        UalManager::instance().updateItem( bw_utf8tow( modelName ) );

        return true;
    }
    
    return false;
}

bool setShadowGraphicsSetting( int shadowQualityOptionIndex, 
    bool enableShadows )
{
    static const char * s_shadowsSettingLabel = "SHADOWS_QUALITY";
    static const int s_disabledShadowsQualityIndex = 4;

    if (!enableShadows)
    {
        shadowQualityOptionIndex = s_disabledShadowsQualityIndex;
    }

    Moo::GraphicsSetting::GraphicsSettingPtr shadowSettings = 
        Moo::GraphicsSetting::getFromLabel( s_shadowsSettingLabel );

    // Update render pipeline shadow settings if selected option doesn't match
    if (shadowSettings->activeOption() != shadowQualityOptionIndex)
    {
        if (!shadowSettings->selectOption( shadowQualityOptionIndex ))
        {
            WARNING_MSG("MeModule::setShadowGraphicsSetting: "
                "unable to set shadowing option %d\n", 
                shadowQualityOptionIndex );
            return false;
        }
    }
    return true;
}


/**
 *  Updates animated objects in this module.
 */
void MeModule::updateAnimations()
{
    BW_GUARD_PROFILER( MeModule_updateAnimations );

    static DogWatch updateAnimations( "Scene update" );
    ScopedDogWatch sdw( updateAnimations );

    Mutant* mutant = MeApp::instance().mutant();
    MF_ASSERT( mutant != NULL );

    ChunkManager& chunkManager = ChunkManager::instance();
    chunkManager.updateAnimations();

    ChunkSpacePtr pSpace = chunkManager.cameraSpace();
    if (pSpace.exists())
    {
        EnviroMinder & enviro = pSpace->enviro();
        enviro.updateAnimations();
    }

    if (mutant->hasModel())
    {
        const bool groundModel =
            Options::getOptionInt( "settings/groundModel", 0 ) != 0;
        const bool centreModel =
            Options::getOptionInt( "settings/centreModel", 0 ) != 0;

        mutant->groundModel( groundModel );
        mutant->centreModel( centreModel );

        mutant->updateActions( lastDTime_ );

        mutant->updateModelAnimations( -1.0f );
        MeApp::instance().mutant()->updateFrameBoundingBox( );
    }

    ParticleSystemAction::flushLateUpdates();
}


//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################

#if 1
void MeModule::render( float dTime )
{
    BW_GUARD;

    updateFrame( 0.f );

    ScopedDogWatch scopedDogWatchDetailDraw(s_detailDraw);

    //-- dTime

    static float s_lastDTime = 1.f/60.f;
    if (dTime != 0.f)
        dTime = dTime/NUM_FRAMES_AVERAGE + (NUM_FRAMES_AVERAGE-1.f)*s_lastDTime/NUM_FRAMES_AVERAGE;
    s_lastDTime = dTime;

    // This is used to limit the number of rebuildCombinedLayer calls per frame
    // because they are very expensive.
    Terrain::EditorTerrainBlock2::nextBlendBuildMark();

    //  Make sure lodding occurs
    Terrain::BasicTerrainLodController::instance().setCameraPosition( 
        Moo::rc().lodInvView().applyToOrigin(), Moo::rc().lodZoomFactor() );

    //-- renderer

    IRendererPipeline* rendererPipeline = Renderer::instance().pipeline();
    bool isDeffered = Renderer::instance().pipelineType() == 
        IRendererPipeline::TYPE_DEFERRED_SHADING;

    //-- options

    bool groundModel = !!Options::getOptionInt( "settings/groundModel", 0 );
    bool centreModel = !!Options::getOptionInt( "settings/centreModel", 0 );
    bool showLightModels = !!Options::getOptionInt( "settings/showLightModels", 1 );
    bool useCustomLighting = !!Options::getOptionInt( "settings/useCustomLighting", 0 );
    bool checkForSparkles = !!Options::getOptionInt( "settings/checkForSparkles", 0 );
    bool showAxes = !checkForSparkles && !!Options::getOptionInt( "settings/showAxes", 0 );
    bool showBoundingBox = !checkForSparkles && !!Options::getOptionInt( "settings/showBoundingBox", 0 );
    bool useTerrain = !checkForSparkles && !!Options::getOptionInt( "settings/useTerrain", 1 );
    bool useFloor = !checkForSparkles && !useTerrain && !!Options::getOptionInt( "settings/useFloor", 1 );
    bool showBsp = !checkForSparkles && !!Options::getOptionInt( "settings/showBsp", 0 );
    int useHDR = checkForSparkles ? 0 : Options::getOptionInt( "settings/enableHDR", 0 );
    int useSSAO = Options::getOptionInt( "settings/enableSSAO", 1 );
    bool special = checkForSparkles || showBsp;
    bool showModel = special || !!Options::getOptionInt( "settings/showModel", 1 );
    bool showWireframe = !special && !!Options::getOptionInt( "settings/showWireframe", 0 );
    bool showSkeleton = !checkForSparkles && !!Options::getOptionInt( "settings/showSkeleton", 0 );
    bool showPortals = !special && !!Options::getOptionInt( "settings/showPortals", 0 );
    bool showShadowing = showModel && !showWireframe && !showBsp &&
        !!Options::getOptionInt( "settings/showShadowing", 1 ) && 
        //pCaster_ && !isDeffered &&
        !MeApp::instance().mutant()->isSkyBox();
    bool showOriginalAnim = !special && !!Options::getOptionInt( "settings/showOriginalAnim", 0 );
    bool showNormals = !special && showModel && !!Options::getOptionInt( "settings/showNormals", 0 );
    bool showBinormals = !special && showModel && !!Options::getOptionInt( "settings/showBinormals", 0 );
    bool showHardPoints = !special && showModel && !!Options::getOptionInt( "settings/showHardPoints", 0 );
    bool showCustomHull = !special && showModel && !!Options::getOptionInt( "settings/showCustomHull", 0 );
    // The proxy should be visible independent of the primary model.
    bool showEditorProxy = !special && !!Options::getOptionInt( "settings/showEditorProxy", 0 );
    bool showActions = !special && showModel && !!Options::getOptionInt( "settings/showActions", 0 );
    Vector3 bkgColour = Options::getOptionVector3( "settings/bkgColour", Vector3( 255.f, 255.f, 255.f )) / 255.f;

    if (useTerrain && !MeApp::instance().mutant()->isSkyBox())
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

    //-- update model
    // TODO_DRAW_CONTEXT: a_radchenko: investigate why update function draws a model
    Moo::DrawContext drawContext( Moo::RENDERING_PASS_COLOR );
    drawContext.begin( Moo::DrawContext::ALL_CHANNELS_MASK );
    drawContext.pushImmediateMode();
    updateModel( drawContext, dTime);

    //-- begin 

    TextureRenderer::updateDynamics( dTime );
    rendererPipeline->begin();
    MeApp::instance().camera()->render(dTime);
    beginRender();

    MeShell::instance().romp().drawPreSceneStuff( checkForSparkles, useTerrain );

    //-- constants

    //-- Turn off the light in the checkForSparkles mode for filling the model into 
    //-- black color.
    Moo::SunLight sun;
    if(!checkForSparkles)
    {
        Moo::DirectionalLightPtr dir = ChunkManager::instance().cameraSpace()->sunLight();
        sun.m_dir = dir->direction();
        sun.m_color = dir->colour();
        sun.m_ambient = ChunkManager::instance().cameraSpace()->ambientLight();
    } 
    else
    {
        sun.m_dir = Vector3(1.f, 0.f, 0.f);
        sun.m_ambient = Moo::Colour(0.f, 0.f, 0.f, 0.f);
        sun.m_color = Moo::Colour(0.f, 0.f, 0.f, 0.f);
    }

    Moo::RenderContext & rc = Moo::rc();
    rc.effectVisualContext().sunLight(sun);
    rc.effectVisualContext().updateSharedConstants(Moo::CONSTANTS_ALL);

    //-- shadows
    int renderState = 0;
    if(!showBsp)
    {
        renderState |= showModel ? SHOW_MODEL : 0;
    }

    // Update Shadows
    {
        // Get shadow quality index from editor options
        int shadowQualityOptionIndex = 
            Options::getOptionInt( "renderer/shadows/quality" );

        // Only when a shadow-affecting setting has changed.
        if (lastShadowsQuality_ != shadowQualityOptionIndex || 
            lastShadowsOn_ != showShadowing )
        {
            // Update last shadow values regardless so that
            // we don't keep re-attempting/warning every frame.
            lastShadowsQuality_ = shadowQualityOptionIndex;
            lastShadowsOn_ = showShadowing;
            if (!setShadowGraphicsSetting( 
                shadowQualityOptionIndex, showShadowing ))
            {
                WARNING_MSG( "MeModule::render: "
                    "Could not set new shadow graphics setting option: %d\n", 
                    shadowQualityOptionIndex );
            }
        }

        shadowRenderItem_.update( MeApp::instance().mutant(), dTime, renderState );
        shadowRenderItem_.addToScene();
    }

    EnviroMinder & enviroMinder = 
        ChunkManager::instance().cameraSpace()->enviro();

    // Update HDR if changed
    if (useHDR != lastHDREnabled_)
    {
        bool enableHDR = (useHDR != 0);
        HDRSettings hdrCfg = enviroMinder.hdrSettings();
        if (hdrCfg.m_enabled != enableHDR)
        {
            hdrCfg.m_enabled = enableHDR;
            enviroMinder.hdrSettings( hdrCfg );
        }

        lastHDREnabled_ = useHDR;
    }

    // Update SSAO if changed
    if (useSSAO != lastSSAOEnabled_)
    {
        bool enableSSAO = (useSSAO != 0);

        SSAOSettings ssaoSettings = enviroMinder.ssaoSettings();
        ssaoSettings.m_enabled = (useSSAO != 0);
        enviroMinder.ssaoSettings( ssaoSettings );

        lastSSAOEnabled_ = useSSAO;
    }

    if (!materialPreviewMode_)
    {
        //Can't render semi-dynamic shadows if the terrain has any semi's on it
        // as the shadows render regardless of if it model is visible or not
        rendererPipeline->dynamicShadow()->set_semiEnabled(useTerrain);
        // Don't draw shadows during preview mode. The reason is that the
        // preview object does not render in the shadow pass and it
        // can make the effect techniques get stuck in shadow rendering
        // technique even when rendering to color buffer.
        Moo::DrawContext shadowDrawContext( Moo::RENDERING_PASS_SHADOWS );
        rendererPipeline->beginCastShadows( shadowDrawContext );
        rendererPipeline->endCastShadows();
    }

    rc.effectVisualContext().updateSharedConstants(Moo::CONSTANTS_ALL);

#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::beginFrame(
        &enviroMinder,
        Moo::RENDERING_PASS_COLOR,
        rc.lodView() );
#endif

    rendererPipeline->beginOpaqueDraw();
    //-- terrain, floor
    this->renderOpaque( drawContext, dTime );
    //-- draw flora
    MeShell::instance().romp().drawSceneStuff( useTerrain, useTerrain );
    rendererPipeline->endOpaqueDraw();
    rendererPipeline->applyLighting();
    this->renderFixedFunction( drawContext, dTime);

#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::endFrame();
#endif

    // If the current model is a sky box, then we draw it explicitly later 
    // instead of writing to g-buffer.
    if (MeApp::instance().mutant()->isSkyBox())
    {
        MeApp::instance().mutant()->render( drawContext, dTime, SHOW_MODEL );
    }

    //-- sky dome

    MeShell::instance().romp().drawDelayedSceneStuff();

    rendererPipeline->beginSemitransparentDraw();
    rendererPipeline->endSemitransparentDraw();

    //-- end
    MeShell::instance().romp().drawPostSceneStuff( drawContext, useTerrain );

    if (!renderingThumbnail_ &&
        showOriginalAnim &&
        !materialPreviewMode_ &&
        MeApp::instance().mutant())
    {
        rc.setRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
        drawContext.pushOverrideBlock( wireframeRenderer_ );

        MeApp::instance().mutant()->drawOriginalModel( drawContext );

        rc.setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
        drawContext.popOverrideBlock( wireframeRenderer_ );
    }

    if(showBsp)
    {
        renderState |= SHOW_BSP;
        MeApp::instance().mutant()->render( drawContext, dTime, renderState );
    }
    drawContext.popImmediateMode();
    drawContext.end( Moo::DrawContext::ALL_CHANNELS_MASK );
    drawContext.flush( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK | Moo::DrawContext::SHIMMER_CHANNEL_MASK );


    rendererPipeline->drawDebugStuff();
    endRender();
    rendererPipeline->end();
    Chunks_drawCullingHUD();
    SimpleGUI::instance().draw();

    //-- update FPS status

    BW::wstring fps;
    // This is done every frame to have constant fluid frame rate.
    if (dTime != 0.f)
    {
        fps = Localise(L"MODELEDITOR/APP/ME_MODULE/POSITIVE_FPS", Formatter( 1.f/dTime, L"%.1f" ) );
    }
    else
    {
        fps = Localise(L"MODELEDITOR/APP/ME_MODULE/ZERO_FPS");
    }
    
    mainFrame_->setStatusText( ID_INDICATOR_FRAMERATE, fps.c_str() );
}
#endif

//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################

//-- This is old render function (before introducing of deferred render pipeline).
//-- May be convenient for bug hunting or something else.

#if 0
{
    BW_GUARD_PROFILER( MeModule_render );

    ScopedDogWatch sdw( s_detailDraw );

    lastDTime_ = 1.f/60.f;
    if (dTime != 0.f)
    {
        dTime = dTime/NUM_FRAMES_AVERAGE +
            (NUM_FRAMES_AVERAGE-1.f)*lastDTime_/NUM_FRAMES_AVERAGE;
    }
    lastDTime_ = dTime;

    IRendererPipeline* rendererPipeline = Renderer::instance().pipeline();
    bool isDeffered = Renderer::instance().pipelineType() == 
        IRendererPipeline::TYPE_DEFERRED_SHADING;

    bool groundModel = !!Options::getOptionInt( "settings/groundModel", 0 );
    bool centreModel = !!Options::getOptionInt( "settings/centreModel", 0 );
    bool showLightModels =
        !!Options::getOptionInt( "settings/showLightModels", 1 );
    bool useCustomLighting =
        !!Options::getOptionInt( "settings/useCustomLighting", 0 );
    bool checkForSparkles =
        !!Options::getOptionInt( "settings/checkForSparkles", 0 );
    bool showAxes = !checkForSparkles &&
        !!Options::getOptionInt( "settings/showAxes", 0 );
    bool showBoundingBox = !checkForSparkles &&
        !!Options::getOptionInt( "settings/showBoundingBox", 0 );
    bool useTerrain = !checkForSparkles &&
        !!Options::getOptionInt( "settings/useTerrain", 1 );
    bool useFloor = !checkForSparkles && !useTerrain &&
        !!Options::getOptionInt( "settings/useFloor", 1 );
    bool showBsp = !checkForSparkles &&
        !!Options::getOptionInt( "settings/showBsp", 0 );
    bool special = checkForSparkles || showBsp;
    bool showModel = special ||
        !!Options::getOptionInt( "settings/showModel", 1 );
    bool showWireframe = !special &&
        !!Options::getOptionInt( "settings/showWireframe", 0 );
    bool showSkeleton = !checkForSparkles &&
        !!Options::getOptionInt( "settings/showSkeleton", 0 );
    bool showPortals = !special &&
        !!Options::getOptionInt( "settings/showPortals", 0 );
    bool showShadowing = !isDeffered && showModel && !showWireframe && !showBsp &&
            !!Options::getOptionInt( "settings/showShadowing", 1 ) &&
            pCaster_ &&
            !MeApp::instance().mutant()->isSkyBox();
    bool showOriginalAnim = !special &&
        !!Options::getOptionInt( "settings/showOriginalAnim", 0 );
    bool showNormals = !special && showModel &&
        !!Options::getOptionInt( "settings/showNormals", 0 );
    bool showBinormals = !special && showModel &&
        !!Options::getOptionInt( "settings/showBinormals", 0 );
    bool showHardPoints = !special && showModel &&
        !!Options::getOptionInt( "settings/showHardPoints", 0 );
    bool showCustomHull = !special && showModel &&
        !!Options::getOptionInt( "settings/showCustomHull", 0 );
    bool showEditorProxy = !special &&
        !!Options::getOptionInt( "settings/showEditorProxy", 0 );
    bool showActions = !special && showModel &&
        !!Options::getOptionInt( "settings/showActions", 0 );

    if (useTerrain && !MeApp::instance().mutant()->isSkyBox())
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

    Vector3 bkgColour = Options::getOptionVector3( "settings/bkgColour",
        Vector3( 255.f, 255.f, 255.f )) / 255.f;

    int numTris = 0;
    bool posChanged = true;
    
    rendererPipeline->begin();
    rendererPipeline->beginOpaqueDraw();

    MeApp::instance().camera()->render( dTime );
    
    TextureRenderer::updateDynamics( dTime );
    
    this->beginRender();

    Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_ALL);

    Moo::RenderContext& rc = Moo::rc();
    DX::Device* pDev = rc.device();
    
    rc.device()->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
            Moo::Colour( 
            checkForSparkles ? 1.f : bkgColour[0], 
            checkForSparkles ? 1.f : bkgColour[1], 
            checkForSparkles ? 1.f : bkgColour[2],
            0.f), 1, 0 );

    rc.setVertexShader( NULL );
    rc.setPixelShader( NULL );
    rc.device()->SetPixelShader( NULL );

    rc.setRenderState( D3DRS_CLIPPING, TRUE );
    rc.setRenderState( D3DRS_LIGHTING, FALSE ); 

    static Vector3 N(0.0, 0.0, 0.0);
    static Vector3 X(1.0, 0.0, 0.0); 
    static Vector3 Y(0.0, 1.0, 0.0); 
    static Vector3 Z(0.0, 0.0, 1.0); 
    static Moo::Colour colourRed( 0.5f, 0.f, 0.f, 1.f );
    static Moo::Colour colourGreen( 0.f, 0.5f, 0.f, 1.f );
    static Moo::Colour colourBlue( 0.f, 0.f, 0.5f, 1.f );

    Moo::Visual::VisualStatics & visualStatics = Moo::Visual::statics();

    this->setLights(checkForSparkles, useCustomLighting);
    
    // Use the sun direction as the shadowing direction.
    EnviroMinder & enviro = ChunkManager::instance().cameraSpace()->enviro();
    Vector3 lightTrans = enviro.timeOfDay()->lighting().sunTransform[2];

    if ((useCustomLighting) && (MeApp::instance().lights()->lightContainer()->directionals().size()))
    {
        lightTrans = Vector3(0,0,0) - MeApp::instance().lights()->lightContainer()->directional(0)->direction();
    }

    MeShell::instance().romp().drawPreSceneStuff(
        checkForSparkles, useTerrain );

    if (useTerrain)
    {
        Moo::rc().effectVisualContext().initConstants();

        speedtree::SpeedTreeRenderer::beginFrame(&enviro, Moo::RENDERING_PASS_COLOR,
            Renderer::instance().pipeline()->lodViewMatrix());

        this->renderChunks( drawContext );

        speedtree::SpeedTreeRenderer::endFrame();
        
        // chunks don't keep the same light container set
        this->setLights(checkForSparkles, useCustomLighting);

        this->renderTerrain( dTime, showShadowing );

        // draw sky etc.
        MeShell::instance().romp().drawDelayedSceneStuff( drawContext );
    }

    //Make sure we restore valid texture stages before continuing
    rc.setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
    rc.setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    rc.setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
    rc.setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
    rc.setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

    // Draw the axes
    if (showAxes)
    {
        Geometrics::drawLine(N, X, colourGreen);
        Geometrics::drawLine(N, Y, colourBlue);
        Geometrics::drawLine(N, Z, colourRed);
    }

    this->setLights(checkForSparkles, useCustomLighting);

    if (useFloor)
    {
        MeApp::instance().floor()->render();
    }

    if (showBsp)
    {
        //rc.lightContainer( MeApp::instance().whiteLight() );
        //rc.specularLightContainer( MeApp::instance().blackLight() );
    }

    if ((showWireframe) && (!materialPreviewMode_))
    {
        rc.setRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
        //rc.lightContainer( MeApp::instance().blackLight() );
        //rc.specularLightContainer( MeApp::instance().blackLight() );
        
        // Use a lightonly renderer in case the material will render nothing (e.g. some alpha shaders)
        visualStatics.s_pDrawOverride = wireframeRenderer_; 
    }
    else
    {
        rc.setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    }

    if (materialPreviewMode_)
    {
        if (PageMaterials::currPage()->previewObject())
        {
            PageMaterials::currPage()->previewObject()->draw( drawContext );
            if (showBoundingBox)
            {
                Geometrics::wireBox( PageMaterials::currPage()->previewObject()->boundingBox(), 0x000000ff );
            }
        }
    }
    else if (MeApp::instance().mutant())
    {
        numTris = rc.liveProfilingData().nPrimitives_;
        // Render the model
        int renderState = 0;
        renderState |= showModel ? SHOW_MODEL : 0;
        renderState |= showBoundingBox ? SHOW_BB : 0;
        renderState |= showSkeleton ? SHOW_SKELETON : 0;
        renderState |= showPortals ? SHOW_PORTALS : 0;
        renderState |= showBsp ? SHOW_BSP : 0;
        renderState |= showEditorProxy ? SHOW_EDITOR_PROXY : 0;
        renderState |= showNormals ? SHOW_NORMALS : 0;
        renderState |= showBinormals ? SHOW_BINORMALS : 0;
        renderState |= showHardPoints ? SHOW_HARD_POINTS : 0;
        renderState |= showCustomHull ? SHOW_CUSTOM_HULL : 0;
        renderState |= showActions ? SHOW_ACTIONS : 0;
        Moo::rc().updateProjectionMatrix();
        MeApp::instance().mutant()->render( drawContext, dTime, renderState );

        numTris = rc.liveProfilingData().nPrimitives_ - numTris;
        
        if (showShadowing)
        {
            //TODO: Move the following line so that the watcher can work correctly.
            //Set the shadow quailty
            pShadowCommon_->shadowQuality( Options::getOptionInt( "renderer/shadows/quality", 2 )  );

            // Render to the shadow caster
            pCaster_->begin( MeApp::instance().mutant()->shadowVisibililtyBox(),
                lightTrans );
            MeApp::instance().mutant()->drawModel( drawContext, Moo::rc().frameTimestamp() );
            pCaster_->end();
        }
    }

    //Make sure we restore these after we are done
    rc.setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    visualStatics.s_pDrawOverride = NULL;

    this->setLights(checkForSparkles, useCustomLighting);
        
    // render the other components, such as romp and flora.
    MeShell::instance().romp().drawPostSceneStuff( drawContext, useTerrain, useTerrain, showShadowing );

    if ((!materialPreviewMode_) && (showShadowing))
    {
        speedtree::SpeedTreeRenderer::beginFrame( &enviro, Moo::RENDERING_PASS_COLOR,
            Renderer::instance().pipeline()->lodViewMatrix());
        
        // Start the shadow rendering and render the shadow onto the terrain (if required)
        pCaster_->beginReceive( drawContext, useTerrain );

        speedtree::SpeedTreeRenderer::endFrame();

        //Do self shadowing
        if (MeApp::instance().mutant())
        {
            MeApp::instance().mutant()->render( drawContext, 0, showModel ? SHOW_MODEL : 0 );
        }

        //Shadow onto the floor
        if (useFloor)
        {
            MeApp::instance().floor()->render( pShadowCommon_->pReceiverOverride()->pRigidEffect_ );
        }

        pCaster_->endReceive();
    }

    if (!renderingThumbnail_)
    {   
        if ((showOriginalAnim) && (!materialPreviewMode_) && (MeApp::instance().mutant()))
        {
            //Moo::LightContainerPtr lc = rc.lightContainer();

            rc.setRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
            visualStatics.s_pDrawOverride = wireframeRenderer_; 

            //rc.lightContainer( MeApp::instance().blackLight() );
            //rc.specularLightContainer( MeApp::instance().blackLight() );

            MeApp::instance().mutant()->drawOriginalModel( drawContext );

            rc.setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
            visualStatics.s_pDrawOverride = NULL;

            //rc.lightContainer( lc );
            //rc.specularLightContainer( lc );
        }
        
        if (showLightModels)
        {
            // gizmo me
            ToolPtr spTool = ToolManager::instance().tool();
            if (spTool)
            {
                spTool->render( drawContext );
            }

            GizmoManager::instance().draw( drawContext );
        }
        
        SimpleGUI::instance().draw( drawContext );

        BW::wstring fps;
        
        // This is done every frame to have constant fluid frame rate.
        if (dTime != 0.f)
        {
            fps = Localise(L"MODELEDITOR/APP/ME_MODULE/POSITIVE_FPS", Formatter( 1.f/dTime, L"%.1f" ) );
        }
        else
        {
            fps = Localise(L"MODELEDITOR/APP/ME_MODULE/ZERO_FPS");
        }
        mainFrame_->setStatusText( ID_INDICATOR_FRAMERATE, fps.c_str() );

        static int s_lastNumTris = -1;
        if (numTris != s_lastNumTris)
        {
            mainFrame_->setStatusText( ID_INDICATOR_TRIANGLES, Localise(L"MODELEDITOR/APP/ME_MODULE/TRIANGLES", numTris) );
            s_lastNumTris = numTris;
        }
    }

    Moo::Visual::drawBatches();

    rendererPipeline->endOpaqueDraw();

    rendererPipeline->beginSemitransparentDraw();
    rendererPipeline->endSemitransparentDraw();

    rendererPipeline->applyLighting();

    rendererPipeline->end();

    Chunks_drawCullingHUD();

    // render the camera with the new view direction
    this->endRender();
}
#endif

//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################


void MeModule::setLights( bool checkForSparkles, bool useCustomLighting )
{
    BW_GUARD;

    if (checkForSparkles)
    {
        MeApp::instance().lights()->unsetLights();

        //Moo::rc().lightContainer( MeApp::instance().blackLight() );
        //Moo::rc().specularLightContainer( MeApp::instance().blackLight() );
    }
    else if (useCustomLighting)
    {
        MeApp::instance().lights()->setLights();
    }
    else // Game Lighting
    {
        MeApp::instance().lights()->unsetLights();

        //Moo::LightContainerPtr lc = new Moo::LightContainer;
        //lc->addDirectional( ChunkManager::instance().cameraSpace()->sunLight() );
        //lc->ambientColour( ChunkManager::instance().cameraSpace()->ambientLight() );
        //Moo::rc().lightContainer( lc );
        //Moo::rc().specularLightContainer( lc );       

        Moo::DirectionalLightPtr dl = ChunkManager::instance().cameraSpace()->sunLight();

        Moo::SunLight sl;
        sl.m_dir = dl->worldDirection();
        sl.m_ambient = ChunkManager::instance().cameraSpace()->ambientLight();
        sl.m_color = dl->colour();

        Moo::rc().effectVisualContext().sunLight(sl);
    }
}

void MeModule::renderChunks( Moo::DrawContext& drawContext )
{
    BW_GUARD;

    Moo::rc().setRenderState( D3DRS_FILLMODE,
        Options::getOptionInt( "render/scenery/wireFrame", 0 ) ?
            D3DFILL_WIREFRAME : D3DFILL_SOLID );

    ChunkManager::instance().camera( Moo::rc().invView(), ChunkManager::instance().cameraSpace() ); 
    ChunkManager::instance().draw( drawContext );

    Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
}

void MeModule::renderTerrain( float dTime /* = 0.f */, bool shadowing /* = false */ )
{
    BW_GUARD;

    if (Options::getOptionInt( "render/terrain", 1 ))
    {
        // draw terrain
        bool canSeeTerrain = 
            Terrain::BaseTerrainRenderer::instance()->canSeeTerrain();

        // Set camera position for terrain, so lods can be set up per frame. 
        Terrain::BasicTerrainLodController::instance().setCameraPosition( 
            Moo::rc().invView().applyToOrigin(), 1.f );

        Moo::rc().setRenderState( D3DRS_FILLMODE,
            Options::getOptionInt( "render/terrain/wireFrame", 0 ) ?
                D3DFILL_WIREFRAME : D3DFILL_SOLID );

        Terrain::BaseTerrainRenderer::instance()->drawAll(Moo::RENDERING_PASS_COLOR);

        Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    }
    else
    {
        Terrain::BaseTerrainRenderer::instance()->clearBlocks();
    }
}

void MeModule::renderOpaque( Moo::DrawContext& drawContext, float dTime)
{
    IRendererPipeline* rendererPipeline = Renderer::instance().pipeline();
    bool isDeffered = Renderer::instance().pipelineType() == 
        IRendererPipeline::TYPE_DEFERRED_SHADING;

    bool groundModel = !!Options::getOptionInt( "settings/groundModel", 0 );
    bool centreModel = !!Options::getOptionInt( "settings/centreModel", 0 );
    bool showLightModels = !!Options::getOptionInt( "settings/showLightModels", 1 );
    bool useCustomLighting = !!Options::getOptionInt( "settings/useCustomLighting", 0 );
    bool checkForSparkles = !!Options::getOptionInt( "settings/checkForSparkles", 0 );
    bool showAxes = !checkForSparkles && !!Options::getOptionInt( "settings/showAxes", 0 );
    bool showBoundingBox = !checkForSparkles && !!Options::getOptionInt( "settings/showBoundingBox", 0 );
    bool useTerrain = !checkForSparkles && !!Options::getOptionInt( "settings/useTerrain", 1 );
    bool useFloor = !checkForSparkles && !useTerrain && !!Options::getOptionInt( "settings/useFloor", 1 );
    bool showBsp = !checkForSparkles && !!Options::getOptionInt( "settings/showBsp", 0 );
    bool special = checkForSparkles || showBsp;
    bool showModel = special || !!Options::getOptionInt( "settings/showModel", 1 );
    bool showWireframe = !special && !!Options::getOptionInt( "settings/showWireframe", 0 );
    bool showSkeleton = !checkForSparkles && !!Options::getOptionInt( "settings/showSkeleton", 0 );
    bool showPortals = !special && !!Options::getOptionInt( "settings/showPortals", 0 );
    //bool showShadowing = !isDeffered && showModel && !showWireframe && !showBsp &&
    //  !!Options::getOptionInt( "settings/showShadowing", 1 ) && pCaster_ &&
    //  !MeApp::instance().mutant()->isSkyBox();
    bool showOriginalAnim = !special && !!Options::getOptionInt( "settings/showOriginalAnim", 0 );
    bool showNormals = !special && showModel && !!Options::getOptionInt( "settings/showNormals", 0 );
    bool showBinormals = !special && showModel && !!Options::getOptionInt( "settings/showBinormals", 0 );
    bool showHardPoints = !special && showModel && !!Options::getOptionInt( "settings/showHardPoints", 0 );
    bool showCustomHull = !special && showModel && !!Options::getOptionInt( "settings/showCustomHull", 0 );
    // The proxy should be visible independent of the primary model.
    bool showEditorProxy = !special && !!Options::getOptionInt( "settings/showEditorProxy", 0 );
    bool showActions = !special && showModel && !!Options::getOptionInt( "settings/showActions", 0 );

    Vector3 bkgColour = Options::getOptionVector3( "settings/bkgColour", Vector3( 255.f, 255.f, 255.f )) / 255.f;

    Moo::RenderContext& rc = Moo::rc();
    DX::Device* pDev = rc.device();

    Moo::rc().effectVisualContext().initConstants();
    if (useTerrain)
    {
        this->renderChunks( drawContext );
        this->renderTerrain(dTime);
    }

    //Make sure we restore valid texture stages before continuing
    rc.setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
    rc.setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    rc.setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
    rc.setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
    rc.setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

    if (useFloor)
    {
        MeApp::instance().floor()->render();
    }

    //----------------------------------------------------------------------------------------------

    if ((showWireframe) && (!materialPreviewMode_))
    {
        rc.setRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
        // Use a lightonly renderer in case the material will render nothing (e.g. some alpha shaders)
        drawContext.pushOverrideBlock( wireframeRenderer_ );
    }
    else
    {
        rc.setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    }

    if (materialPreviewMode_)
    {
        if (PageMaterials::currPage()->previewObject())
        {
            PageMaterials::currPage()->previewObject()->draw( drawContext );
            if (showBoundingBox)
            {
                Geometrics::wireBox( PageMaterials::currPage()->previewObject()->boundingBox(), 
                                     0x000000ff );
            }
        }
    }
    else if (MeApp::instance().mutant())
    {
        int numTris = rc.liveProfilingData().nPrimitives_;

        // Render the model
        int renderState = 0;
        if(!showBsp)
        {
            // for sky box, we will draw it explicitly later 
            renderState |= 
                showModel && !MeApp::instance().mutant()->isSkyBox()? SHOW_MODEL : 0;
            renderState |= showEditorProxy ? SHOW_EDITOR_PROXY : 0;
        }
        //renderState |= showBoundingBox ? SHOW_BB : 0;
        //renderState |= showSkeleton ? SHOW_SKELETON : 0;
        //renderState |= showPortals ? SHOW_PORTALS : 0;
        //renderState |= showBsp ? SHOW_BSP : 0;
        //renderState |= showNormals ? SHOW_NORMALS : 0;
        //renderState |= showBinormals ? SHOW_BINORMALS : 0;
        //renderState |= showHardPoints ? SHOW_HARD_POINTS : 0;
        //renderState |= showCustomHull ? SHOW_CUSTOM_HULL : 0;
        //renderState |= showActions ? SHOW_ACTIONS : 0;
        Moo::rc().updateProjectionMatrix();

        MeApp::instance().mutant()->render( drawContext, dTime, renderState );

        numTris = rc.liveProfilingData().nPrimitives_ - numTris;

        // Update counter with number of the polygons which was drawn

        static int s_lastNumTris = -1;
        if (numTris != s_lastNumTris)
        {
            mainFrame_->setStatusText( ID_INDICATOR_TRIANGLES, Localise(L"MODELEDITOR/APP/ME_MODULE/TRIANGLES", numTris) );
            s_lastNumTris = numTris;
        }

        //if (showShadowing)
        //{
            ////TODO: Move the following line so that the watcher can work correctly.
            ////Set the shadow quailty
            //pShadowCommon_->shadowQuality( Options::getOptionInt( "renderer/shadows/quality", 2 )  );

            //// Render to the shadow caster
            //pCaster_->begin( MeApp::instance().mutant()->shadowVisibililtyBox(), lightTrans );
            //MeApp::instance().mutant()->drawModel( false, modelDist );
            //pCaster_->end();
        //}
    }

    if ((showWireframe) && (!materialPreviewMode_))
    {
        //Make sure we restore these after we are done
        rc.setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
        drawContext.popOverrideBlock( wireframeRenderer_ );
    }
}

void MeModule::renderFixedFunction( Moo::DrawContext& drawContext, float dTime)
{
    IRendererPipeline* rendererPipeline = Renderer::instance().pipeline();
    bool isDeffered = Renderer::instance().pipelineType() == 
        IRendererPipeline::TYPE_DEFERRED_SHADING;

    bool groundModel = !!Options::getOptionInt( "settings/groundModel", 0 );
    bool centreModel = !!Options::getOptionInt( "settings/centreModel", 0 );
    bool showLightModels = !!Options::getOptionInt( "settings/showLightModels", 1 );
    bool useCustomLighting = !!Options::getOptionInt( "settings/useCustomLighting", 0 );
    bool checkForSparkles = !!Options::getOptionInt( "settings/checkForSparkles", 0 );
    bool showAxes = !checkForSparkles && !!Options::getOptionInt( "settings/showAxes", 0 );
    bool showBoundingBox = !checkForSparkles && !!Options::getOptionInt( "settings/showBoundingBox", 0 );
    bool useTerrain = !checkForSparkles && !!Options::getOptionInt( "settings/useTerrain", 1 );
    bool useFloor = !checkForSparkles && !useTerrain && !!Options::getOptionInt( "settings/useFloor", 1 );
    bool showBsp = !checkForSparkles && !!Options::getOptionInt( "settings/showBsp", 0 );
    bool special = checkForSparkles || showBsp;
    bool showModel = special || !!Options::getOptionInt( "settings/showModel", 1 );
    bool showWireframe = !special && !!Options::getOptionInt( "settings/showWireframe", 0 );
    bool showSkeleton = !checkForSparkles && !!Options::getOptionInt( "settings/showSkeleton", 0 );
    bool showPortals = !special && !!Options::getOptionInt( "settings/showPortals", 0 );
    //bool showShadowing = !isDeffered && showModel && !showWireframe && !showBsp &&
    //  !!Options::getOptionInt( "settings/showShadowing", 1 ) && pCaster_ &&
    //  !MeApp::instance().mutant()->isSkyBox();
    bool showOriginalAnim = !special && !!Options::getOptionInt( "settings/showOriginalAnim", 0 );
    bool showNormals = !special && showModel && !!Options::getOptionInt( "settings/showNormals", 0 );
    bool showBinormals = !special && showModel && !!Options::getOptionInt( "settings/showBinormals", 0 );
    bool showHardPoints = !special && showModel && !!Options::getOptionInt( "settings/showHardPoints", 0 );
    bool showCustomHull = !special && showModel && !!Options::getOptionInt( "settings/showCustomHull", 0 );
    // The proxy should be visible independent of the primary model.
    bool showEditorProxy = !special && !!Options::getOptionInt( "settings/showEditorProxy", 0 );
    bool showActions = !special && showModel && !!Options::getOptionInt( "settings/showActions", 0 );

    static const bool showBackground = true;
    Vector3 bkgColourV = Options::getOptionVector3( "settings/bkgColour", Vector3( 255.f, 255.f, 255.f )) / 255.f;
    Moo::Colour backgroundColour( bkgColourV[0], bkgColourV[1], bkgColourV[2], 0.0f );
    static const Moo::Colour s_sparklesCheckColour( 1.0f, 1.0f, 1.0f, 0.0f );

    Moo::RenderContext& rc = Moo::rc();
    DX::Device* pDev = rc.device();

    //Make sure we restore valid texture stages before continuing
    //-- TODO: reconsider
    rc.setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
    rc.setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    rc.setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
    rc.setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
    rc.setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );


    if (showBackground)
    {
        // Background colour
        const Moo::Colour & rectColour = 
            checkForSparkles ? s_sparklesCheckColour : backgroundColour;

        // Setup full screen rect
        static Vector2 topLeft( 0.0f, 0.0f );
        Vector2 bottomRight( rc.screenWidth(), rc.screenHeight() );
        static const float rectDepth = 1.0f;

        // Draw rect, respecting depth.
        rc.pushRenderState( D3DRS_ZENABLE );
        rc.setRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
        {
            rc.pushRenderState( D3DRS_ZFUNC );
            rc.setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
            {
                Geometrics::drawRect( topLeft, bottomRight, 
                    rectColour, rectDepth );
            }
            rc.popRenderState();
        }
        rc.popRenderState();
    }

    if (showAxes)
    {
        static Vector3 N(0.0, 0.0, 0.0);
        static Vector3 X(1.0, 0.0, 0.0); 
        static Vector3 Y(0.0, 1.0, 0.0); 
        static Vector3 Z(0.0, 0.0, 1.0); 
        static Moo::Colour colourRed( 0.5f, 0.f, 0.f, 1.f );
        static Moo::Colour colourGreen( 0.f, 0.5f, 0.f, 1.f );
        static Moo::Colour colourBlue( 0.f, 0.f, 0.5f, 1.f );

        Geometrics::drawLine(N, X, colourGreen, false);
        Geometrics::drawLine(N, Y, colourBlue, false);
        Geometrics::drawLine(N, Z, colourRed, false);
    }

    //----------------------------------------------------------------------------------------------

    if ((showWireframe) && (!materialPreviewMode_))
    {
        rc.setRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
        // Use a lightonly renderer in case the material will render nothing (e.g. some alpha shaders)
        drawContext.pushOverrideBlock( wireframeRenderer_ );
    }
    else
    {
        rc.setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    }

    if (materialPreviewMode_)
    {
        //if (PageMaterials::currPage()->previewObject())
        //{
        //  PageMaterials::currPage()->previewObject()->draw();
        //  if (showBoundingBox)
        //  {
        //      Geometrics::wireBox( PageMaterials::currPage()->previewObject()->boundingBox(), 0x000000ff );
        //  }
        //}
    }
    else if (MeApp::instance().mutant())
    {
        //-- TODO (a_cherkes): numTris
        //numTris = rc.liveProfilingData().nPrimitives_;
        // Render the model
        int renderState = 0;
        //renderState |= showModel ? SHOW_MODEL : 0;
        renderState |= showBoundingBox ? SHOW_BB : 0;
        renderState |= showSkeleton ? SHOW_SKELETON : 0;
        renderState |= showPortals ? SHOW_PORTALS : 0;
        renderState |= showBsp ? SHOW_BSP : 0;
        //renderState |= showEditorProxy ? SHOW_EDITOR_PROXY : 0;
        renderState |= showNormals ? SHOW_NORMALS : 0;
        renderState |= showBinormals ? SHOW_BINORMALS : 0;
        renderState |= showHardPoints ? SHOW_HARD_POINTS : 0;
        renderState |= showCustomHull ? SHOW_CUSTOM_HULL : 0;
        renderState |= showActions ? SHOW_ACTIONS : 0;
        Moo::rc().updateProjectionMatrix();

         MeApp::instance().mutant()->render( drawContext, dTime, renderState );

        //numTris = rc.liveProfilingData().nPrimitives_ - numTris;

        //if (showShadowing)
        //{
            ////TODO: Move the following line so that the watcher can work correctly.
            ////Set the shadow quailty
            //pShadowCommon_->shadowQuality( Options::getOptionInt( "renderer/shadows/quality", 2 )  );

            //// Render to the shadow caster
            //pCaster_->begin( MeApp::instance().mutant()->shadowVisibililtyBox(), lightTrans );
            //MeApp::instance().mutant()->drawModel( false, modelDist );
            //pCaster_->end();
        //}
    }

    //Make sure we restore these after we are done
    if ((showWireframe) && (!materialPreviewMode_))
    {
        rc.setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
        drawContext.popOverrideBlock( wireframeRenderer_ );
    }
}

void MeModule::updateModel( Moo::DrawContext& drawContext, float dTime )
{
    //-- update all animations and nodes

    bool groundModel = !!Options::getOptionInt( "settings/groundModel", 0 );
    bool centreModel = !!Options::getOptionInt( "settings/centreModel", 0 );

    if ( MeApp::instance().mutant()->modelName() != "" )
    {
        MeApp::instance().mutant()->groundModel( groundModel );
        MeApp::instance().mutant()->centreModel( centreModel );
        MeApp::instance().mutant()->updateActions( dTime );
        MeApp::instance().mutant()->updateFrameBoundingBox();

        if (!materialPreviewMode_)
        {
            static bool s_last_groundModel = !groundModel;
            static bool s_last_centreModel = !centreModel;
            if ((s_last_groundModel != groundModel) || (s_last_centreModel != centreModel))
            {
                //Do this to ensure the model nodes are up to date
                MeApp::instance().mutant()->drawModel( drawContext );
                MeApp::instance().camera()->boundingBox(
                    MeApp::instance().mutant()->zoomBoundingBox() );
                s_last_groundModel = groundModel;
                s_last_centreModel = centreModel;
                MeApp::instance().camera()->update();
                bool posChanged = true;
            }
        }
    }
}

void MeModule::endRender()
{
    //Moo::rc().endScene();
}


bool MeModule::handleKeyEvent( const KeyEvent & event )
{
    BW_GUARD;

    // usually called through py script
    bool handled = MeApp::instance().camera()->handleKeyEvent(event);

    if (event.key() == KeyCode::KEY_LEFTMOUSE)
    {
        if (event.isKeyDown())
        {
            handled = true;

            GizmoManager::instance().click();
        }
    }

    return handled;
}

bool MeModule::handleMouseEvent( const MouseEvent & event )
{
    BW_GUARD;

    // Its ugly, but for the mean time...it needs to be done in CPP
    if ( InputDevices::isKeyDown( KeyCode::KEY_SPACE ) && ( event.dz() != 0 ) )
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
        float speed = newSpeed == "Medium" ? 8.f : ( newSpeed == "Fast" ? 24.f : ( newSpeed == "SuperFast" ? 48.f : 1.f ) );
        MeApp::instance().camera()->speed( Options::getOptionFloat( "camera/speed/"+newSpeed, speed ) );
        MeApp::instance().camera()->turboSpeed( Options::getOptionFloat( "camera/speed/"+newSpeed+"/turbo", 2*speed ) );
        GUI::Manager::instance().update();
        return true;
    }
    else
        return MeApp::instance().camera()->handleMouseEvent(event);
}

Vector2 MeModule::currentGridPos()
{
    BW_GUARD;

    POINT pt = MeShell::instance().currentCursorPosition();
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


Vector3 MeModule::gridPosToWorldPos( Vector2 gridPos )
{
    Vector2 w = (gridPos + Vector2( float(localToWorld_.x), float(localToWorld_.y) )) *
        ChunkManager::instance().cameraSpace()->gridSize();

    return Vector3( w.x, 0, w.y);
}



// ----------------
// Python Interface
// ----------------

/**
 *  The static python render method
 */
PyObject * MeModule::py_render( PyObject * args )
{
    BW_GUARD;

    float dTime = 0.033f;

    if (!PyArg_ParseTuple( args, "|f", &dTime ))
    {
        PyErr_SetString( PyExc_TypeError, "ModelEditor.render() "
            "takes only an optional float argument for dtime" );
        return NULL;
    }

    s_instance_->render( dTime );

    Py_RETURN_NONE;
}


PyObject * MeModule::py_onEditorReady( PyObject * args )
{
    BW_GUARD;

    ScriptObject callback;

    if (!PyArg_ParseTuple( args, "O", &callback ) || !callback.isCallable())
    {
        PyErr_SetString( PyExc_TypeError, "ModelEditor.onEditorReady() "
            "expects a callable argument.");
        Py_RETURN_NONE;
    }

    instance().editorReadyCallback_ = callback;

    ChunkManager::instance().processPendingChunks();
    ChunkManager::instance().startTimeLoading(
        &MeModule::chunksLoadedCallback, true );

    Py_RETURN_NONE;
}

/* static */ void MeModule::chunksLoadedCallback( const char * )
{
    if (instance().editorReadyCallback_)
    {
        instance().editorReadyCallback_.callFunction( ScriptErrorPrint() );
        instance().editorReadyCallback_ = ScriptObject();
    }
}

PY_MODULE_STATIC_METHOD( MeModule, render, ModelEditor )
PY_MODULE_STATIC_METHOD( MeModule, onEditorReady, ModelEditor )

BW_END_NAMESPACE

