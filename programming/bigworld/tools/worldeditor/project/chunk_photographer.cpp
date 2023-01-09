#include "pch.hpp"
#include "worldeditor/project/chunk_photographer.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_thumbnail_cache.hpp"
#include "worldeditor/world/items/editor_chunk_link.hpp"
#include "appmgr/options.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_water.hpp"
#include "chunk/chunk_flare.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "moo/mrt_support.hpp"
#include "moo/render_context.hpp"
#include "moo/texture_compressor.hpp"
#include "moo/draw_context.hpp"
#include "duplo/py_splodge.hpp"
#include "moo/lod_settings.hpp"
#include "speedtree/speedtree_renderer.hpp"
#include "terrain/terrain2/terrain_lod_controller.hpp"
#include "terrain/base_terrain_renderer.hpp"

#include "speedtree/billboard_optimiser.hpp"
#include "cstdmf/debug.hpp"

#include "moo/renderer.hpp"
#include "romp/hdr_support.hpp"

DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( ChunkPhotographer );


/**
 *  Render a top-down picture of a chunk and save it as a thumbnail.
 *  @param chunk the chunk to render.
 */
bool ChunkPhotographer::photograph( Chunk& chunk )
{
    BW_GUARD;

    DataSectionPtr thumbSection =
        EditorChunkThumbnailCache::instance(chunk).pThumbSection();
    bool ret = ChunkPhotographer::instance().takePhoto( chunk, thumbSection, 128, 128, true );
    if (ret)
    {
        WorldManager::instance().chunkThumbnailUpdated( &chunk );
    }
    return ret;
}


/**
 *  Render a custom top-down picture of a chunk and save it to the given
 *  datasection.
 *  @param chunk the chunk to render.
 *  @param width the desired width of the image.
 *  @param height the desired height of the image.
 */
bool ChunkPhotographer::photograph( Chunk& chunk, DataSectionPtr ds, int width, int height )
{
    BW_GUARD;

    return ChunkPhotographer::instance().takePhoto( chunk, ds, width, height, false );
}


/**
 *  Render a custom top-down picture of a chunk and save it to the given file.
 *  @param chunk the chunk to render.
 *  @param filePath the file to save the image in.
 *  @param width the desired width of the image.
 *  @param height the desired height of the image.
 */
bool ChunkPhotographer::photograph( Chunk& chunk, const BW::string& filePath, int width, int height )
{
    BW_GUARD;
    return ChunkPhotographer::instance().takePhoto( chunk, filePath, width, height, false );
}



extern bool & getDisableSkyLightMapFlag();


ChunkPhotographer::ChunkPhotographer():
pRT_( NULL ),
savedLighting_( NULL ),
pOldChunk_( NULL )
{
    photographerDrawContext_ = new Moo::DrawContext( Moo::RENDERING_PASS_REFLECTION );
}

ChunkPhotographer::~ChunkPhotographer()
{
    bw_safe_delete( photographerDrawContext_ );
}


bool ChunkPhotographer::takePhotoInternal( Chunk& chunk, int width, int height )
{
    BW_GUARD;
    MF_ASSERT( chunk.loaded() );

    if (pRT_ && (pRT_->width() != width || pRT_->height() != height))
    {
        pRT_ = NULL;
    }

    // make the render target if necessary
    if (!pRT_)
    {
        pRT_ = new Moo::RenderTarget( "TextureRenderer" );
        pRT_->create( width, height, false, D3DFMT_X8R8G8B8 );
    }

    if (!pRT_)
    {
        ERROR_MSG( "ChunkPhotographer::takePhoto: Failed because render target was not created\n" );
        return false;
    }

    MF_ASSERT( pRT_.getObject() );
    MF_ASSERT( pRT_->valid() );

    bool ok = true;

    if (this->beginScene())
    {
        this->setStates(chunk);
        this->renderScene(chunk);
        this->resetStates();
        this->endScene();

        if (Moo::rc().device()->TestCooperativeLevel() != D3D_OK)
        {
            ERROR_MSG( "ChunkPhotographer::takePhoto: "
                "Failed enabling render target while processing %s.\n",
                chunk.identifier().c_str() );
            ok = false;
        }
    }
    return ok;
}

/**
 *  This method photographs a chunk.
 */
bool ChunkPhotographer::takePhoto( Chunk& chunk, const BW::string& filePath, int width, int height, bool useDXT )
{
    bool ok = false;
    BWResource::ensureAbsolutePathExists( filePath );
    if ( this->takePhotoInternal( chunk, width, height ) )
    {
        ok = this->savePhotoToFile( chunk, filePath, width, height, useDXT );
    }
    return ok;
}

/**
 *  This method photographs a chunk.
 */
bool ChunkPhotographer::takePhoto( Chunk& chunk, DataSectionPtr ds, int width, int height, bool useDXT )
{
    bool ok = false;
    if ( this->takePhotoInternal( chunk, width, height ) )
    {
        ok = this->savePhotoToDatasection( chunk, ds, width, height, useDXT );
    }
    return ok;
}


/**
 *  Saves the contents of the photo render target to a file.
 *  Uses the current render target. ie. pRT_.
 *  @param chunk the chunk that is being photographed.
 *  @param filePath the path of the file to save the image in.
 *  @param width the width of the image.
 *  @param height the height of the image.
 *  @param useDXT what format to use for texture compression.
 */
bool ChunkPhotographer::savePhotoToFile( Chunk& chunk,
    const BW::string& filePath, int width, int height, bool useDXT )
{
    BW_GUARD;

    MF_ASSERT( pRT_.hasObject() );
    MF_ASSERT( pRT_->valid() );
    MF_ASSERT( width == pRT_->width() );
    MF_ASSERT( height == pRT_->height() );

    const D3DFORMAT fmt = useDXT ? D3DFMT_DXT1 : D3DFMT_A8R8G8B8;

    Moo::TextureCompressor tc1(
        static_cast< DX::Texture* >( pRT_->pTexture() ) );
    const bool ok = tc1.save( filePath, fmt, 1 );

    return ok;
}


/**
 *  Saves the contents of the photo render target to the chunk.cdata.
 *  Uses the current render target. ie. pRT_.
 *  @param chunk the chunk that is being photographed.
 *  @param ds the chunk.cdata's datasection to save into.
 *  @param width the width of the image.
 *  @param height the height of the image.
 *  @param useDXT what format to use for texture compression.
 */
bool ChunkPhotographer::savePhotoToDatasection( Chunk& chunk,
    DataSectionPtr ds, int width, int height, bool useDXT )
{
    BW_GUARD;

    MF_ASSERT( pRT_.hasObject() );
    MF_ASSERT( pRT_->valid() );
    MF_ASSERT( width == pRT_->width() );
    MF_ASSERT( height == pRT_->height() );

    const D3DFORMAT fmt = useDXT ? D3DFMT_DXT1 : D3DFMT_A8R8G8B8;

    Moo::TextureCompressor tc0(
        static_cast< DX::Texture* >( pRT_->pTexture() ) );
    const bool ok = tc0.stow( ds, "", fmt, 1 );

    return ok;
}


bool ChunkPhotographer::beginScene()
{
    BW_GUARD;

    // set and clear the render target
    bool ok = false;
    if (pRT_->valid() && pRT_->push())
    {
        Moo::rc().beginScene();
        if (Moo::rc().mixedVertexProcessing())
            Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );
        Moo::rc().device()->Clear(
            0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff808080, 1.f, 0 );
        ok = true;
    }
    return ok;
}


void ChunkPhotographer::setStates( Chunk& chunk )
{
    BW_GUARD;

    //first, setup some rendering options.
    //we need to draw as cheaply as possible, and
    //not clobber the main pipeline.

    oldWaterSimulationEnabled_ = Waters::simulationEnabled();
    Waters::simulationEnabled(false);
    oldWaterDrawReflection_ = Waters::drawReflection();
    Waters::drawReflection(false);

    ChunkFlare::ignore( true );
    EditorChunkLink::enableDraw( false );
    EditorChunkStationNode::enableDraw( false );
    PySplodge::s_ignoreSplodge_ = true;
    getDisableSkyLightMapFlag() = true;

    oldIgnoreLods_ = LodSettings::instance().ignoreLods();
    LodSettings::instance().ignoreLods( true );

    //save some states we are about to change
    oldFOV_ = Moo::rc().camera().fov();
    savedLighting_ = Moo::rc().lightContainer();
    savedChunkLighting_ = &ChunkManager::instance().cameraSpace()->enviro().timeOfDay()->lighting();
    oldInvView_ = Moo::rc().invView();
    pOldChunk_ = ChunkManager::instance().cameraChunk();
    const float gridSize = ChunkManager::instance().cameraSpace()->gridSize();

    //create some lighting
    if ( !lighting_ )
    {
        Vector4 ambientColour( 0.08f,0.02f,0.1f,1.f );

        //outside lighting for chunks
        chunkLighting_.sunTransform.setRotateX( DEG_TO_RAD(90.f) );
        chunkLighting_.sunTransform.postRotateZ( DEG_TO_RAD(20.f) );
        chunkLighting_.sunColour.set( 1.f, 1.f, 1.f, 1.f );     
        chunkLighting_.ambientColour = ambientColour;

        //light container for terrain
        Moo::DirectionalLightPtr spDirectional = 
            new Moo::DirectionalLight( Moo::Colour( 1, 1, 1, 1 ), Vector3( 0, 0, -1.f ) );
        Vector3 lightPos = chunkLighting_.mainLightTransform().applyPoint( Vector3( 0, 0, -1 ) );
        Vector3 dir = Vector3( 0, 0, 0 ) - lightPos;
        dir.normalise();
        spDirectional->direction( dir ); 
        spDirectional->worldTransform( Matrix::identity );

        lighting_ = new Moo::LightContainer;
        lighting_->ambientColour( Moo::Colour( ambientColour ) );
        lighting_->addDirectional( spDirectional );
    }

    Moo::rc().lightContainer( lighting_ );

    //setup the correct transform for the given chunk.
    //adds of .25 is for the near clipping plane.
    BW::vector<ChunkItemPtr> items;
    EditorChunkCache::instance(chunk).allItems( items );
    BoundingBox bb( Vector3::zero(), Vector3::zero() );
    for( BW::vector<ChunkItemPtr>::iterator iter = items.begin(); iter != items.end(); ++iter )
        (*iter)->addYBounds( bb );
    Matrix view;
    Vector3 lookFrom( chunk.transform().applyToOrigin() );  
    lookFrom.x += gridSize / 2.f;
    lookFrom.z += gridSize / 2.f;
    lookFrom.y = bb.maxBounds().y + 0.25f + 300.f;

    float chunkHeight = bb.maxBounds().y - bb.minBounds().y + 320.f;
    view.lookAt( lookFrom, Vector3(0,-1,0), Vector3(0,0,1) );
    Moo::rc().push();
    Moo::rc().world( Matrix::identity );
    Matrix proj;
    proj.orthogonalProjection( gridSize, gridSize, 0.25f, chunkHeight + 0.25f );
    Moo::rc().view( view );
    Moo::rc().projection( proj );
    Moo::rc().updateViewTransforms();
    Terrain::BaseTerrainRenderer::instance()->enableSpecular( false );

    //make sure there are no states set into the main part of bigbang
    //that could upset the rendering
    Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    FogController::instance().enable( false );  
}


void ChunkPhotographer::renderScene( Chunk & chunk )
{
    BW_GUARD;

#if SPEEDTREE_SUPPORT
    // Force max LOD level
    const float oldTreeLodMode =
        speedtree::SpeedTreeRenderer::lodMode( 1.0f );
#endif

    photographerDrawContext_->begin( Moo::DrawContext::ALL_CHANNELS_MASK );

    // TODO hack to get lighting to work
    Moo::SunLight sun;
    sun.m_ambient = lighting_->ambientColour();
    sun.m_color   = lighting_->directionals().front()->colour();
    sun.m_dir     = lighting_->directionals().front()->worldDirection();

    Moo::rc().lightContainer( lighting_ );
    Moo::rc().effectVisualContext().sunLight(sun);
    ChunkManager::instance().camera( Moo::rc().invView(),
        ChunkManager::instance().cameraSpace(), &chunk );
    ChunkManager::instance().cameraSpace()->heavenlyLightSource( &chunkLighting_ );
    ChunkManager::instance().cameraSpace()->updateHeavenlyLighting();   

    //-- disable HDR because we use reflection
    bool isHDREnabled = false;
    if (Renderer::instance().pipeline()->hdrSupport())
    {
        isHDREnabled = Renderer::instance().pipeline()->hdrSupport()->enable();
        Renderer::instance().pipeline()->hdrSupport()->enable(false);
    }

    // send fog state to the renderer
    FogController::instance().updateFogParams();

    //-- update all shared constants.
    Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_ALL);

#if SPEEDTREE_SUPPORT
    speedtree::BillboardOptimiser::tick();
#endif//SPEEDTREE_SUPPORT

    ChunkManager::instance().draw( *photographerDrawContext_ );

    //turn off alpha writes, because we are saving in DXT1 format ( one-bit alpha )
    //and the terrain normally uses alpha channel encoding, which upsets the bitmap.
    //not sure why the terrain has to output its alpha, but hell this is a workaround.
    Moo::rc().setRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN);
    Moo::rc().lightContainer( lighting_ );
    Moo::rc().effectVisualContext().sunLight(sun);
    Terrain::BasicTerrainLodController::instance().setCameraPosition( Moo::rc().invView().applyToOrigin(), 1.0f );

    // hide overlays.
    bool isOverlaysEnabled = Terrain::BaseTerrainRenderer::instance()->overlaysEnabled();
    Terrain::BaseTerrainRenderer::instance()->enableOverlays(false);

    Terrain::BaseTerrainRenderer::instance()->drawAll( Moo::RENDERING_PASS_REFLECTION );

    Terrain::BaseTerrainRenderer::instance()->enableOverlays(isOverlaysEnabled);

    Moo::rc().setRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_ALPHA);

    // Draw water
    Waters::instance().projectView(true);   
    Waters::instance().drawDrawList(  0.f );
    Waters::instance().projectView(false);

    photographerDrawContext_->end( Moo::DrawContext::ALL_CHANNELS_MASK );
    photographerDrawContext_->flush( Moo::DrawContext::ALL_CHANNELS_MASK );

    //-- restore HDR state
    if (Renderer::instance().pipeline()->hdrSupport())
    {
        Renderer::instance().pipeline()->hdrSupport()->enable(isHDREnabled);
    }

    //-- update all shared constants.
    Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_ALL);

#if SPEEDTREE_SUPPORT
    // Restore lod level    
    speedtree::SpeedTreeRenderer::lodMode( oldTreeLodMode );
#endif
}


void ChunkPhotographer::resetStates()
{
    BW_GUARD;

    //set the stuff back
    Moo::rc().lightContainer( savedLighting_ );
    ChunkManager::instance().camera( oldInvView_,
                    ChunkManager::instance().cameraSpace(),
                    (pOldChunk_ && pOldChunk_->isBound()) ? pOldChunk_ : NULL );
    ChunkManager::instance().cameraSpace()->heavenlyLightSource( savedChunkLighting_ );
    Moo::rc().camera().fov( oldFOV_ );
    Moo::rc().updateProjectionMatrix();
    Moo::rc().pop();
    Terrain::BaseTerrainRenderer::instance()->enableSpecular( true );

    //restore drawing states!
    ChunkFlare::ignore( false );
    EditorChunkLink::enableDraw( true );
    EditorChunkStationNode::enableDraw( true );
    PySplodge::s_ignoreSplodge_ = false;
    LodSettings::instance().ignoreLods( oldIgnoreLods_ );
    getDisableSkyLightMapFlag() = false;

    Waters::drawReflection(oldWaterDrawReflection_);
    Waters::simulationEnabled(oldWaterSimulationEnabled_);
}


void ChunkPhotographer::endScene()
{
    BW_GUARD;

    // and reset everything
    Moo::rc().endScene();
    pRT_->pop();
}


/*~ function WorldEditor.photographChunk
 *  @components{ worldeditor }
 *
 *  Given an outside chunk identifier, this will save a top down orthographic
 *  snapshot of the chunk to the given filename, in DDS format.
 *
 *  @param  string      Name of the chunk to photograph
 *  @param  int         Width of the output image in pixels
 *  @param  int         Height of the output image in pixels
 *  @param  string      Filename to write as
 *
 *  @return bool        Whether or not the chunk photo was taken
 *
 */
static bool photographChunk( const BW::string& chunkId, 
                              int width, int height, 
                              const BW::string& filename )
{
    Chunk* chunk = 
        ChunkManager::instance().findChunkByName( 
            chunkId, 
            WorldManager::instance().geometryMapping() 
        );

    if (!chunk)
    {
        INFO_MSG( "photographChunk: Couldn't find chunk '%s'\n", chunkId.c_str() );
        return false;
    }

    // Force to memory:
    if (!chunk->completed())
    {
        ChunkManager::instance().loadChunkNow
        (
            chunkId,
            WorldManager::instance().geometryMapping()
        );
        ChunkManager::instance().checkLoadingChunks();
    }

    DataSectionPtr ds = BWResource::openSection( filename, true, BinSection::creator() );
    if (!ds)
    {
        ERROR_MSG( "photographChunk: failed to open '%s' for writing.\n",
            filename.c_str() );
        return false;
    }

    INFO_MSG( "photographChunk: chunkId=%s, width=%d, height=%d, filename=%s\n",
        chunkId.c_str(), width, height, filename.c_str() );

    ChunkPhotographer::photograph( *chunk, ds, width, height );

    ds->save();
    
    return true;
}

PY_AUTO_MODULE_FUNCTION( RETDATA, photographChunk, ARG( BW::string, ARG(int, ARG( int, ARG( BW::string, END )))), WorldEditor )
BW_END_NAMESPACE

