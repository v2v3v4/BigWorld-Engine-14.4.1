#include "pch.hpp"
#include "terrain_renderer2.hpp"

#include "../manager.hpp"
#include "../terrain_settings.hpp"
#include "moo/effect_visual_context.hpp"
#include "terrain_block2.hpp"
#include "terrain_texture_layer2.hpp"
#include "terrain_lod_map2.hpp"
#include "terrain_photographer.hpp"
#include "vertex_lod_entry.hpp"
#include "cstdmf/profiler.hpp"
#include "cstdmf/watcher.hpp"
#include "terrain_height_map2.hpp"
#include "moo/line_helper.hpp"
#include "terrain_normal_map2.hpp"

#include "moo/renderer.hpp"
#include "moo/shadow_manager.hpp"
#include "moo/render_event.hpp"


#ifdef EDITOR_ENABLED
#include "ttl2cache.hpp"
#include "editor_terrain_block2.hpp"
#include "../../../tools/worldeditor/misc/options_helper.hpp"
#include "pyscript/script.hpp"
#endif // EDITOR_ENABLED

DECLARE_DEBUG_COMPONENT2( "Terrain", 0 )


BW_BEGIN_NAMESPACE

using namespace Terrain;

extern bool g_drawTerrain;

namespace
{
    /**
     * Flag adding of watches on first run of constructor.
     */
    bool g_firstTime    = true;

    /**
     *  This is a table of layer strings used in setting effects.
     */
    static const char *layerText[4] =
    {
        "layer0",
        "layer1",
        "layer2",
        "layer3"
    };

    /**
     *  This is a table of layer strings used in setting effects.
     */
    const char* const bumpLayerText[4] =
    {
        "layer0Bump",
        "layer1Bump",
        "layer2Bump",
        "layer3Bump"
    };

    /**
     *  This is a table of layer u-projection strings used in setting effects.
     */
    static const char *layerUProjText[4] =
    {
        "layer0UProjection",
        "layer1UProjection",
        "layer2UProjection",
        "layer3UProjection"
    };

    /**
     *  This is a table of layer v-projection strings used in setting effects.
     */
    static const char *layerVProjText[4] =
    {
        "layer0VProjection",
        "layer1VProjection",
        "layer2VProjection",
        "layer3VProjection"
    };

    /**
     *  This is a table of layer replacement color strings used in setting effects.
     */
    static const char *layerOverlayColorText[4] =
    {
        "layer0OverlayColor",
        "layer1OverlayColor",
        "layer2OverlayColor",
        "layer3OverlayColor"
    };

    /**
     *  This table is a mask for which indicies are used when there are zero,
     *  one etc layers in a combined layer when we use as small a texture
     *  size as possible.
     */
    static const Vector4 layerMaskTable[] = 
    {                                    // layers x  y  z  w
        Vector4(0.0f, 0.0f, 0.0f, 0.0f), // 0
        Vector4(1.0f, 0.0f, 0.0f, 0.0f), // 1      t0      
        Vector4(1.0f, 0.0f, 0.0f, 1.0f), // 2      t0       t1
        Vector4(1.0f, 1.0f, 1.0f, 0.0f), // 3      t2 t1 t0
        Vector4(1.0f, 1.0f, 1.0f, 1.0f)  // 4      t2 t1 t0 t3
    };

    /**
     *  This table is a mask for which indicies are used when there are zero,
     *  one etc layers in a combined layer when we use 32 bit textures.
     */
    static const Vector4 layerMaskTable32bit[] = 
    {                                    // layers x  y  z  w
        Vector4(0.0f, 0.0f, 0.0f, 0.0f), // 0
        Vector4(0.0f, 0.0f, 1.0f, 0.0f), // 1            t0      
        Vector4(0.0f, 1.0f, 1.0f, 0.0f), // 2         t1 t0 
        Vector4(1.0f, 1.0f, 1.0f, 0.0f), // 3      t2 t1 t0
        Vector4(1.0f, 1.0f, 1.0f, 1.0f)  // 4      t2 t1 t0 t3
    };

    /**
     *  Each row of the following map represents the order that combined layers
     *  map to textures.  This is for the case when we compress textures to
     *  8, 16 or 32 bit textures if possible.
     */
    static const uint32 layerTexMap[5][4] =
    {
        { 0, 1, 2, 3 },
        { 0, 1, 2, 3 },
        { 0, 3, 1, 2 }, 
        { 2, 1, 0, 3 },
        { 2, 1, 0, 3 }
    };

    /**
     *  Each row of the following map represents the order that combined layers
     *  map to textures.  This is for the case when we use 32 bit textures for 
     *  the blends.
     */
    static const uint32 layerTexMap32[5][4] =
    {
        { 0, 1, 2, 3 },
        { 2, 0, 1, 3 },
        { 2, 1, 0, 3 }, 
        { 2, 1, 0, 3 },
        { 2, 1, 0, 3 }
    };

    bool drawHeightmap = false;
    bool drawPlaceHolders = false;
    bool g_isHorizonShadowMapSuppressed = false;
} // anonymous namespace


namespace Terrain
{

// Reserved stencil value (and 7) for controlling pixel placement within a block.
const uint32 TERRAIN_STENCIL_OFFSET = 7;

// -----------------------------------------------------------------------------
// Section: TerrainRenderer2
// -----------------------------------------------------------------------------

TerrainRenderer2* TerrainRenderer2::s_instance_ = NULL;

/*
 * Default constructor
 */
TerrainRenderer2::TerrainRenderer2() :
    pDecl_( NULL ),
    frameTimestamp_( Moo::rc().frameTimestamp() ),
    nBlocksRendered_( 0 ),
    zBufferIsClear_( false ),
    useStencilMasking_( true ),
    horizonShadowsBlendDistance(0.f, 0.f),
    dummyHorizonShadowMap_( NULL ),
    isHorizonMapSuppressed(false)
{
    BW_GUARD;
    MF_ASSERT( Manager::pInstance() != NULL );

    texturedMaterial_.pEffect_ = NULL;
    lodTextureMaterial_.pEffect_ = NULL;
    zPassMaterial_.pEffect_ = NULL;
#ifdef EDITOR_ENABLED
    pPhotographer_ = new TerrainPhotographer(*this); 
    overlayAlpha = 0.5f;
    currTextureOverlayLayer_ = 0;
#endif
    forceRenderLowestLod_ = false;

    RESOURCE_COUNTER_ADD(   ResourceCounters::DescriptionPool("Terrain/TerrainRenderer2",
        (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
        (uint)sizeof(*this), 0 )
}

/**
 * Initialise renderer internals from values in given settings section.
 */
bool TerrainRenderer2::initInternal(const DataSectionPtr& pSettingsSection )
{
    BW_GUARD;

    // Validate section
    if ( !pSettingsSection )
    {
        ERROR_MSG( "Couldn't open settings section\n" );
        return false;
    }

    //-- 
    bool success = true;

    //-- try to load textured material effect.
    BW::string texturedEffect = pSettingsSection->readString( "texturedEffect" );
    if ( texturedEffect.empty() )
    {
        ERROR_MSG( "Couldn't get textured effect.\n" );
        return false;
    }
    else
    {
        texturedMaterial_.pEffect_ = new Moo::EffectMaterial;
        success &= texturedMaterial_.pEffect_->initFromEffect(texturedEffect);
        if (!success)
        {
            ERROR_MSG( "Couldn't initialise effect from file %s.\n", texturedEffect.c_str() );
            return false;
        }
        texturedMaterial_.pEffect_->hTechnique("MAIN");
        texturedMaterial_.getHandles();

        texturedReflectionMaterial_.pEffect_ = new Moo::EffectMaterial;
        success &= texturedReflectionMaterial_.pEffect_->initFromEffect(texturedEffect);
        texturedReflectionMaterial_.pEffect_->hTechnique("REFLECTION");
        texturedReflectionMaterial_.getHandles();
    }

    BW::string lodTextureEffect = pSettingsSection->readString( "lodTextureEffect" );
    if (lodTextureEffect.empty())
    {
        ERROR_MSG( "Couldn't get lod texture effect.\n" );
        return false;
    }
    else
    {
        lodTextureMaterial_.pEffect_ = new Moo::EffectMaterial;
        success &= lodTextureMaterial_.pEffect_->initFromEffect(lodTextureEffect);
        if (!success)
        {
            ERROR_MSG( "Couldn't initialise effect from file %s.\n", lodTextureEffect.c_str() );
            return false;
        }
        lodTextureMaterial_.pEffect_->hTechnique("MAIN");
        lodTextureMaterial_.getHandles();

        lodTextureReflectionMaterial_.pEffect_ = new Moo::EffectMaterial;
        success &= lodTextureReflectionMaterial_.pEffect_->initFromEffect(lodTextureEffect);
        lodTextureReflectionMaterial_.pEffect_->hTechnique("REFLECTION");
        lodTextureReflectionMaterial_.getHandles();
    }

    BW::string zPassEffect = pSettingsSection->readString( "zPassEffect" );
    if ( zPassEffect.empty() )
    {
        ERROR_MSG( "Couldn't get z pass effect.\n" );
        return false;
    }
    else
    {
        zPassMaterial_.pEffect_ = new Moo::EffectMaterial;
        success &= zPassMaterial_.pEffect_->initFromEffect(zPassEffect);
        if (!success)
        {
            ERROR_MSG( "Couldn't initialise effect from file %s.\n", zPassEffect.c_str() );
            return false;
        }
        zPassMaterial_.getHandles();    
    }

    BW::string shadowCastEffect = pSettingsSection->readString("shadowCastEffect");
    if (shadowCastEffect.empty())
    {
        ERROR_MSG( "Couldn't get shadow cast effect.\n" );
        return false;
    }
    else
    {
        shadowCastMaterial_.pEffect_ = new Moo::EffectMaterial;
        success &= shadowCastMaterial_.pEffect_->initFromEffect(shadowCastEffect);
        if (!success)
        {
            ERROR_MSG( "Couldn't initialise effect from file %s.\n", shadowCastEffect.c_str() );
            return false;
        }
        shadowCastMaterial_.getHandles();   
    }


#ifdef EDITOR_ENABLED
    overlayMaterial_.pEffect_ = new Moo::EffectMaterial;
    success &= overlayMaterial_.pEffect_->initFromEffect(
        "resources/shaders/terrain_overlay.fx" );
    overlayMaterial_.getHandles();
    
    lodMapGeneratorMaterial_.pEffect_ = new Moo::EffectMaterial;
    success &= lodMapGeneratorMaterial_.pEffect_->initFromEffect(
        "resources/shaders/terrain_lod_map_generator.fx" );
    lodMapGeneratorMaterial_.getHandles();
#endif

    if (!success)
    {
        ERROR_MSG( "Couldn't create the full set of terrain materials.\n" );
        return false;
    }

    pDecl_ = Moo::VertexDeclaration::get( "terrain" );
    if (!pDecl_)
    {
        ERROR_MSG( "Couldn't create terrain vertex format.\n" );
        return false;
    }

    // Bind textured material's specular constants to our local values
    ComObjectWrap<ID3DXEffect> pEffect = texturedMaterial_.pEffect_->pEffect()->pEffect();
    pEffect->GetFloat( "terrain2Specular.power",            &specularInfo_.power_);
    pEffect->GetFloat( "terrain2Specular.multiplier",       &specularInfo_.multiplier_ );
    pEffect->GetFloat( "terrain2Specular.fresnelExp",       &specularInfo_.fresnelExp_ );
    pEffect->GetFloat( "terrain2Specular.fresnelConstant",  &specularInfo_.fresnelConstant_ );

    // Are 8 and 16 bit textures supported?:
    supportSmallBlends_ = 
        Moo::rc().supportsTextureFormat(D3DFMT_L8)
        &&
        Moo::rc().supportsTextureFormat(D3DFMT_A8L8);

    if(!dummyHorizonShadowMap_.create(1)) {
        ERROR_MSG( "Couldn't create dummy horizon shadow map.\n" );
        return false;
    }
    
    useStencilMasking_ = true;

    if ( g_firstTime )
    {
    // init watchers
        MF_WATCH( "Render/Terrain/Terrain2/RenderedBlocks", nBlocksRendered_, 
            Watcher::WT_READ_ONLY );

        MF_WATCH( "Render/Terrain/Terrain2/VertexLODSizeMB", 
            VertexLodEntry::s_totalMB_, Watcher::WT_READ_ONLY  );

        MF_WATCH( "Render/Terrain/Terrain2/specular power", *this,
                MF_ACCESSORS( float, TerrainRenderer2, specularPower ),
                "Mathematical power of the specular reflectance function." );

        MF_WATCH( "Render/Terrain/Terrain2/specular multiplier", *this,
                MF_ACCESSORS( float, TerrainRenderer2, specularMultiplier ),
                "Overall multiplier on the terrain specular effect." );

        MF_WATCH( "Render/Terrain/Terrain2/specular fresnel constant", *this,
                MF_ACCESSORS( float, TerrainRenderer2, specularFresnelConstant ),
                "Fresnel constant for falloff calculations." );

        MF_WATCH( "Render/Terrain/Terrain2/specular fresnel falloff", *this,
                MF_ACCESSORS( float, TerrainRenderer2, specularFresnelExp ),
                "Fresnel falloff rate." );

        MF_WATCH( "Render/Performance/Enable Terrain Draw", g_drawTerrain,
            Watcher::WT_READ_WRITE,
            "Enable high level terrain renderering." );

        MF_WATCH( "Render/Terrain/Terrain2/Draw Heightmap", drawHeightmap,
            Watcher::WT_READ_WRITE,
            "Enable visualisation of terrain height map.");

        MF_WATCH( "Render/Terrain/Terrain2/Draw Placeholders", drawPlaceHolders,
            Watcher::WT_READ_WRITE,
            "Draw placeholders for missing or partially loaded terrain.");

        MF_WATCH( "Render/Terrain/Terrain2/horizon shadows suppress", g_isHorizonShadowMapSuppressed,
            Watcher::WT_READ_WRITE,
            "Enable or disable horizon shadows mechanism.");

#ifdef DEBUG_STENICL_MASKING
        // This allows developer to switch the feature off and on, only useful
        // if something related to it breaks.
        MF_WATCH( "Render/Terrain/Terrain2/useStencilMasking", useStencilMasking_,
            Watcher::WT_READ_WRITE );
#endif
        g_firstTime = false;
    }

    return true;
}

/**
 * Accessor for renderer instance.
 */
TerrainRenderer2* TerrainRenderer2::instance()
{
    MF_ASSERT( s_instance_ != NULL );
    return s_instance_;
}

//--------------------------------------------------------------------------------------------------
bool TerrainRenderer2::init()
{
    BW_GUARD;
    if ( s_instance_ != NULL )
        return true;

    s_instance_ = new TerrainRenderer2;

    // destroy the old terrain before creating the new one
    BaseTerrainRenderer::instance( s_instance_ );

    bool res = s_instance_->initInternal( Manager::instance().pTerrain2Defaults() );
    if (!res)
    {
        bw_safe_delete(s_instance_);
        BaseTerrainRenderer::instance( s_instance_ );
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
TerrainRenderer2::~TerrainRenderer2()
{
    BW_GUARD;
    s_instance_ = NULL;
    if (Manager::pInstance() != NULL && BaseTerrainRenderer::instance() == this)
        BaseTerrainRenderer::instance( NULL );

#ifdef EDITOR_ENABLED
    bw_safe_delete(pPhotographer_);

    // Reset the layer cache whenever the renderer2 is destroyed.
    if (Manager::pInstance() != NULL)
        Manager::instance().ttl2Cache().clear();
#endif // EDITOR_ENABLED
    RESOURCE_COUNTER_SUB(   ResourceCounters::DescriptionPool("Terrain/TerrainRenderer2",
        (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
        (uint)sizeof(*this), 0 )
}

//--------------------------------------------------------------------------------------------------
void TerrainRenderer2::drawSingle(
    Moo::ERenderingPassType passType, BaseRenderTerrainBlock* pBlock,
    const Matrix& transform, Moo::EffectMaterialPtr pOverrideMaterial)
{
    BW_GUARD;
    BW_SCOPED_RENDER_PERF_MARKER("Terrain");

    ensureEffectMaterialsUpToDate();

    Moo::rc().effectVisualContext().isOutside( true );

    BaseMaterial* pOverrideBaseMaterial = NULL;
    if ( pOverrideMaterial )
    {
        overrideMaterial_.pEffect_ = pOverrideMaterial;
        overrideMaterial_.getHandles();
        pOverrideBaseMaterial = &overrideMaterial_;
    }

    //-- push color writing states.
    Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );
    Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE1 );
    Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE2 );
    Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE3 );

    drawSingleInternal( passType,  pBlock, transform, false, pOverrideBaseMaterial );

    //-- pop color writing states.
    Moo::rc().popRenderState();
    Moo::rc().popRenderState();
    Moo::rc().popRenderState();
    Moo::rc().popRenderState();
}


/**
 *  Get the version of terrain that this renderer can render.
 *  @return terrain version that gets set in TerrainSettings.
 */
uint32 TerrainRenderer2::version() const
{
    BW_GUARD;
    return TerrainSettings::TERRAIN2_VERSION;
}


//--------------------------------------------------------------------------------------------------
void TerrainRenderer2::drawPlaceHolderBlock(TerrainBlock2*  pBlock,
                                            uint32          colour,
                                            bool            lowDetail)
{
    BW_GUARD;


    Matrix  transform = Moo::rc().world();


    if ( lowDetail )
    {
        TerrainHeightMap2Ptr    hmp     = pBlock->heightMap2();
        uint32                  size    = hmp->verticesWidth();

        float h[4] =
        {   
            hmp->heightAt( 0, 0 ),
            hmp->heightAt( 0, (int)size ),
            hmp->heightAt( (int)size, 0 ),
            hmp->heightAt( (int)size, (int)size )
        };
        Vector3 v[4];
        v[0].set( 0, h[0], 0 );
        v[1].set( 0, h[1], pBlock->blockSize() );
        v[2].set( pBlock->blockSize(), h[2], 0 );
        v[3].set( pBlock->blockSize(), h[3], pBlock->blockSize() );

        transform.applyPoint( v[0], v[0] );
        transform.applyPoint( v[1], v[1] );
        transform.applyPoint( v[2], v[2] );
        transform.applyPoint( v[3], v[3] );

        // draw diagonal cross
        LineHelper::instance().drawLine( v[0], v[3], colour );
        LineHelper::instance().drawLine( v[1], v[2], colour );
    }
    else
    {
        TerrainHeightMap2Ptr hmp =  pBlock->getHighestLodHeightMap();

        uint32  size = hmp->verticesWidth();
        float   step = pBlock->blockSize() / float(size);

        for ( uint32 x = 0; x < size; x++ )
        {
            for ( uint32 z = 0; z < size; z++ )
            {
                Vector3 p1( step * x, 
                    hmp->heightAt(int(x), int(z)), 
                    step * (z) );
                Vector3 p2( step * x, 
                    hmp->heightAt(int(x), int(z+1)), 
                    step * (z+1) );
                Vector3 p3( step * (x+1), 
                    hmp->heightAt(int(x+1), int(z+1)), 
                    step * (z+1) );
                transform.applyPoint( p1, p1 );
                transform.applyPoint( p2, p2 );
                transform.applyPoint( p3, p3 );

                LineHelper::instance().drawLine( p1, p2, colour );
                LineHelper::instance().drawLine( p1, p3, colour );
            }
        }
    }
}

/**
*   This method is the internal drawing method for a terrain block.
*   
*   @param pBlock               The block to draw.
*   @param transform            World transform of block.
*   @param pOverrideMaterial    An alternative material to use.
*/
void TerrainRenderer2::drawSingleInternal(
    Moo::ERenderingPassType passType, BaseRenderTerrainBlock* pBlock, const Matrix& transform,
    bool depthOnly, BaseMaterial* pOverrideMaterial)
{
    BW_GUARD;
    if (g_drawTerrain)
    {
        // Update world transform.
        TerrainBlock2* tb2 = static_cast<TerrainBlock2*>(pBlock);
        Moo::rc().world( transform );

        // Force draw of heightmap
        if ( drawHeightmap )
        {
            uint32 lod = tb2->getHighestLodHeightMap()->lodLevel();

            drawPlaceHolderBlock( tb2, 0xFFFFFFFF, lod > 0 );
        }

        // Draw big grid.
        if ( drawPlaceHolders )
            drawPlaceHolderBlock( tb2, 0xFFFF00FF, true );

        // update profiling
        if (!Moo::rc().frameDrawn( frameTimestamp_ ))
        {
            nBlocksRendered_ = 0;
        }

        // Prepare block for draw - always try to substitute, otherwise draw
        // a magenta placeholder.
        bool ok = tb2->preDraw( true );
        if ( !ok ) 
        {
            if ( drawPlaceHolders ) drawPlaceHolderBlock( tb2, 0xFFFF00FF );
            return;
        }

        // set the vertex declaration
        Moo::rc().setVertexDeclaration( pDecl_->declaration() );

        // get a copy of blends to use throughout draw
#if EDITOR_ENABLED
        TerrainBlendsPtr pBlends = tb2->currentDrawState_.blendsPtr_;

        // In the editor, the blends could have 
        if (!tb2->pBlendsResource_.hasObject() || tb2->pBlendsResource_->getState() != RS_Loaded)
        {
            pBlends = NULL;
        }
#else
        const TerrainBlendsPtr& pBlends = tb2->currentDrawState_.blendsPtr_;
#endif // EDITOR_ENABLED

        // Allow drawTerrainMaterial to keep track of how many passes have been done
        uint32 passCount = 0;

        if ( pOverrideMaterial )
        {
            // draw the block as override
            drawTerrainMaterial( tb2, pOverrideMaterial, DM_SINGLE_OVERRIDE, passCount );
        }
        else if (passType == Moo::RENDERING_PASS_SHADOWS)
        {
            drawTerrainMaterial( tb2, &shadowCastMaterial_, DM_SHADOW_CAST, passCount );
        }
        else
        {
            // Determine blends availability
            bool drawReflection = (passType == Moo::RENDERING_PASS_REFLECTION);
            bool drawBump       = false;
            bool drawBlend  = false;
            bool drawLod    = false;
            bool drawNormalLod = false;

            // Lod texture might be switched off (due to user settings) or 
            // missing (due to bad data).
            const bool  canDrawLodTexture = tb2->canDrawLodTexture();
            const uint8 rtMask            = tb2->lodRenderInfo_.renderTextureMask_;

            // We should have specified something to draw, or else there's a 
            // serious logic error.
            MF_ASSERT_DEBUG( rtMask > 0 );

            // If blends are available, and we can't draw lod textures or we
            // should draw blends anyway, then draw blend.
            if ( pBlends && ( !canDrawLodTexture || rtMask & RTM_DrawBlend ) )
            {
                drawBlend = true;
                drawBump  = (rtMask & RTM_DrawBump) != 0;
            }

            // If lod texture is available, and we don't have blends or we should
            // draw lod anyway, then draw lod.
            if ( canDrawLodTexture)
            {
                if ( (!pBlends && (rtMask & RTM_DrawBlend)) || (rtMask & RTM_DrawLOD) )
                {
                    drawLod = true;
                }
                if (rtMask & RTM_DrawLODNormals)
                {
                    drawNormalLod = true;
                }
            
                if (forceRenderLowestLod_)
                {
                    drawLod         = true;
                    drawNormalLod   = false;
                    drawBlend       = false;
                    drawBump        = false;
                }
            }

            if ( drawBlend || drawLod || drawNormalLod )
            {
                if ( depthOnly )
                {
                    bool res = drawTerrainMaterial( tb2, &zPassMaterial_, DM_Z_ONLY, passCount );
                    MF_ASSERT( res && "failed zpass draw" );
                    tb2->depthPassMark( Moo::rc().renderTargetCount() );
                }
                else
                {
                    // Determine if we need to do a z pass. A zpass is required if we have a
                    // holes map. Even then, only do it if Umbra hasn't already done it
                    // or if we are in wireframe mode, in which case the z buffer has been
                    // cleared and the Umbra pass would be lost.

                    bool drawZPass = false;

                    if ( tb2->pHolesMap() )
                    {
                        if ( tb2->depthPassMark() != Moo::rc().renderTargetCount() )
                            drawZPass = true;
                        if ( Terrain::Manager::instance().wireFrame() )
                            drawZPass = true;
                        if ( zBufferIsClear_ )
                            drawZPass = true;
                    }

                    // Draw a z pass if we need to.
                    if ( drawZPass )
                    {
                        bool res = drawTerrainMaterial( tb2, &zPassMaterial_, DM_Z_ONLY, passCount );
                        MF_ASSERT( res && "failed zpass draw" );
                    }

                    // Drawing policy : We "should" always have a lodTexture. 
                    // 
                    // When drawing, we can always fall back to this if blends 
                    // (which are streamed) aren't available. This includes the 
                    // situation when "useLodTexture" is set to false - that is, we 
                    // should only draw blends (a high quality graphics option). If 
                    // no lod texture or blend is available we don't get drawn.
                    if ( drawBlend )
                    {
                        // Record whether blends rendered ok, so lod pass can avoid
                        // blending to "blue".
                        tb2->currentDrawState_.blendsRendered_ = drawTerrainMaterial(
                            tb2, drawReflection ? &texturedReflectionMaterial_ : &texturedMaterial_,
                            (drawBump && !drawReflection) ? DM_BUMP : DM_BLEND, passCount
                            );

                        nBlocksRendered_++;
                    }

                    // Lod pass is done if the feature is enabled and if it has been 
                    // flagged for this block.
                    if ( drawLod )
                    {
                        bool res = drawTerrainMaterial( 
                            tb2, drawReflection ? &lodTextureReflectionMaterial_ : &lodTextureMaterial_, 
                            DM_SINGLE_LOD, passCount
                            );

                        if ( !res && drawPlaceHolders ) 
                            drawPlaceHolderBlock( tb2, 0xFFFFFFFF );

                        tb2->currentDrawState_.lodsRendered_ = true;

                        nBlocksRendered_++;
                    }

                    if (drawNormalLod)
                    {
                        bool res = drawTerrainMaterial(
                            tb2, drawReflection ? &lodTextureReflectionMaterial_ : &lodTextureMaterial_,
                            DM_LOD_NORMALS, passCount
                            );

                        if ( !res && drawPlaceHolders ) 
                            drawPlaceHolderBlock( tb2, 0xFFFF0000 );

                        nBlocksRendered_++;
                    }

#if defined(EDITOR_ENABLED) && !defined(IS_OFFLINE_PROCESSOR)
                    uint overlayMode =  overlaysEnabled() ? 
                                        terrainOverlayMode() : TERRAIN_OVERLAY_MODE_DEFAULT;
                    if (overlayMode == TERRAIN_OVERLAY_MODE_COLORIZE_GROUND_STRENGTH)
                    {
                        // note: using DM_BLEND because we need blendMapSampler and related stuff
                        drawTerrainMaterial( tb2, &overlayMaterial_, DM_BLEND, passCount );
                    }
                    else if (overlayMode == TERRAIN_OVERLAY_MODE_TEXTURE_OVERLAY)
                    {
                        for (currTextureOverlayLayer_ = 0;
                            currTextureOverlayLayer_ < textureOverlays.size();
                            currTextureOverlayLayer_++)
                        {
                            if (textureOverlays[currTextureOverlayLayer_]->visible)
                                drawTerrainMaterial(tb2, &overlayMaterial_, DM_BLEND, passCount);
                        }
                    }
                    else if (overlayMode == TERRAIN_OVERLAY_MODE_GROUND_TYPES_MAP)
                    {
                        drawTerrainMaterial(tb2, &overlayMaterial_, DM_BLEND, passCount);
                    }
#endif //-- EDITOR_ENABLED
                }
            }
            else
            {
                if ( drawPlaceHolders )
                    drawPlaceHolderBlock( tb2, 0xFF0000FF );
            }
        }

        HRESULT hr = Moo::rc().device()->SetStreamSource(1, NULL, 0, 0);

        if (FAILED(hr))
        {
            ERROR_MSG( "TerrainRenderer2::drawSingleInternal: SetStreamSource"
                " failed with error code %ld\n", hr );
        }

        MF_ASSERT(hr == D3D_OK);
    }
}

/**
 *  This method sets the transform constants on a material
 *  @param pBlock the block to set the constants for
 *  @param pMaterial the material to set the constants on
 */
void TerrainRenderer2::setTransformConstants( const TerrainBlock2* pBlock,
                                              BaseMaterial* pMaterial )
{
    BW_GUARD;
    pMaterial->SetParam( pMaterial->world_, &Moo::rc().world() );
    pMaterial->SetParam( pMaterial->terrainScale_, pBlock->blockSize() );
}

/**
 *
 */
void TerrainRenderer2::setAOMapConstants( const TerrainBlock2* pBlock, BaseMaterial* pMaterial )
{
    BW_GUARD;

    pMaterial->SetParam( pMaterial->hasAO_, pBlock->pAOMap() != NULL );
    if (pBlock->pAOMap())
    {
        pMaterial->SetParam( pMaterial->aoMap_, pBlock->pAOMap() );
        pMaterial->SetParam( pMaterial->aoMapSize_, (float)pBlock->aoMapSize() );
    }
}

/**
 *  This method sets the normal map constants on a material
 *  @param pBlock the block to set the constants for
 *  @param pMaterial the material to set the constants on
 */
void TerrainRenderer2::setNormalMapConstants( const TerrainBlock2* pBlock, 
                                              BaseMaterial* pMaterial,
                                              bool lodNormals)
{
    BW_GUARD;

    const TerrainNormalMap2Ptr& pNormals = pBlock->normalMap2();
    if (lodNormals)
    {
        pMaterial->SetParam( pMaterial->normalMap_, pNormals->pLodMap().pComObject() );
        pMaterial->SetParam( pMaterial->normalMapSize_, (float)pNormals->lodSize() );
    }
    else
    {
        pMaterial->SetParam( pMaterial->normalMap_, pNormals->pMap().pComObject() );
        pMaterial->SetParam( pMaterial->normalMapSize_, (float)pNormals->size() );
    }
}

/**
 *  This method sets the horizon map constants on a material
 *  @param pBlock the block to set the constants for
 *  @param pMaterial the material to set the constants on
 */
void TerrainRenderer2::setHorizonMapConstants( const TerrainBlock2* pBlock, 
                                               BaseMaterial* pMaterial )
{
    BW_GUARD;

    if(!isHorizonMapSuppressed && !g_isHorizonShadowMapSuppressed) {
    pMaterial->SetParam( pMaterial->horizonMap_, pBlock->pHorizonMap() );
    pMaterial->SetParam( pMaterial->horizonMapSize_, (float)pBlock->horizonMapSize() );
    } else {
        pMaterial->SetParam( pMaterial->horizonMap_, dummyHorizonShadowMap_.texture());
        pMaterial->SetParam( pMaterial->horizonMapSize_, (float)dummyHorizonShadowMap_.width());
    }
}

/**
 *  This method sets the holes map constants on a material
 *  @param pBlock the block to set the constants for
 *  @param pMaterial the material to set the constants on
 */
void TerrainRenderer2::setHolesMapConstants(const TerrainBlock2*    pBlock, 
                                            BaseMaterial*   pMaterial )
{
    BW_GUARD;

    pMaterial->SetParam( pMaterial->holesMap_, pBlock->pHolesMap() );
    pMaterial->SetParam( pMaterial->holesMapSize_, (float)pBlock->holesMapSize() );
    pMaterial->SetParam( pMaterial->holesSize_, (float)pBlock->holesSize() );
}

uint32 TerrainRenderer2::getLayerIdx(uint32 layerNo, const CombinedLayer& combinedLayer)
{
    return (supportSmallBlends_ && combinedLayer.smallBlended_) 
        ? layerTexMap[combinedLayer.textureLayers_.size()][layerNo]
        : layerTexMap32[combinedLayer.textureLayers_.size()][layerNo];
}

/**
 *  This sets up the constants for a single layer.
 *
 *  @param effect           The effect whose constants need setting.
 *  @param layerNo          The layerNo to set.
 *  @param combinedLayer    The combined layer that we are working with.
 */
void TerrainRenderer2::setSingleLayerConstants
(
    BaseMaterial*        pMaterial,
    uint32               layerNo,
    CombinedLayer const& combinedLayer,
    uint8&               bumpShaderMask,
    bool                 bumped
)
{
    BW_GUARD;
    if (layerNo < combinedLayer.textureLayers_.size())
    {
    // layerIdx is how we swizzle the order of textures from the packed texture
        const uint32 layerIdx               = getLayerIdx(layerNo, combinedLayer);
        const TerrainTextureLayerPtr& layer = combinedLayer.textureLayers_[layerNo];

        //-- update bumpShaderMask and set bump texture only in case if it's available.
        if (bumped && layer->bumpTexture().exists())
        {
            //-- update bump mask.
            bumpShaderMask |= 1 << layerIdx;

            //-- set texture.
            pMaterial->SetParam( pMaterial->bumpLayer_[layerIdx], layer->bumpTexture()->pTexture() );
        }

        pMaterial->SetParam( pMaterial->layer_[layerIdx], layer->texture()->pTexture() );
        pMaterial->SetParam( pMaterial->layerUProjection_[layerIdx], &layer->uProjection() );
        pMaterial->SetParam( pMaterial->layerVProjection_[layerIdx], &layer->vProjection() );

#if defined(EDITOR_ENABLED) && !defined(IS_OFFLINE_PROCESSOR)
        if (terrainOverlayMode() == TERRAIN_OVERLAY_MODE_COLORIZE_GROUND_STRENGTH)
        {
            TerrainTextureLayer *pLayer = combinedLayer.textureLayers_[layerNo].getObject();

            const BW::string &texName = pLayer->textureName();
            TerrainOverlayColorMap::iterator it = colorOverlays_.find(texName);

            Vector4 color;
            if (it != colorOverlays_.end())
                color = it->second;
            else
    {
                static PyObject *pyModule = PyImport_ImportModule("GroundStrengthHelper");
                static PyObject *pyCtrl = PyObject_GetAttrString(pyModule, "g_helper");
                static PyObject *pyFnGetGroundStrengthColor = PyObject_GetAttrString(pyCtrl, "getGroundStrengthColor");

                PyObject *pyArgs = PyTuple_New(1);
                PyTuple_SetItem(pyArgs, 0, Script::getData(texName));
                Py_XINCREF(pyFnGetGroundStrengthColor);
                PyObject *pRet = Script::ask(pyFnGetGroundStrengthColor, pyArgs);

                uint value = PyLong_AsLong(pRet);
                const float d = 1 / 255.0f;
                color = Vector4(d * ((value & 0xff0000) >> 16), d * ((value & 0xff00) >> 8), d * (value & 0xff), 1);

                colorOverlays_[texName] = color;

                Py_XDECREF(pRet);
            }

            pMaterial->SetParam(pMaterial->overlayMode_, TERRAIN_OVERLAY_MODE_COLORIZE_GROUND_STRENGTH);
            pMaterial->SetParam(pMaterial->layerOverlayColors_[layerIdx], &color);
            pMaterial->SetParam(pMaterial->overlayAlpha_, overlayAlpha);
        }
#endif //-- EDITOR_ENABLED
    }
}

/**
 *  This method sets the texture layer constants on a material
 *  @param pBlock the block to set the constants for
 *  @param pMaterial the material to set the constants on
 *  @param layer the index of the layer to set as active on the material
 */
void TerrainRenderer2::setTextureLayerConstants(
    const TerrainBlendsPtr& pBlends, BaseMaterial* pMaterial, uint32 layer, bool blended, bool bumped)
{
    BW_GUARD;
    MF_ASSERT( pBlends.exists() );

    // Grab the layer and set up the blend texture
    const CombinedLayer& theLayer = pBlends->combinedLayers_[layer];

    const Vector4& layerMask = 
        (supportSmallBlends_ && theLayer.smallBlended_)
            ? layerMaskTable[theLayer.textureLayers_.size()]
            : layerMaskTable32bit[theLayer.textureLayers_.size()];

    // Grab the layer and set up the blend texture
    pMaterial->SetParam( pMaterial->blendMap_, theLayer.pBlendTexture_.pComObject() );
    pMaterial->SetParam( pMaterial->blendMapSize_, float(theLayer.width_) );
    pMaterial->SetParam( pMaterial->layerMask_, &layerMask );

    //-- by defaults we don't have any bump textures.
    uint8 bumpShaderMask = 0;

    //-- Set up our texture layers
    //-- Note: bumpShaderMask may be updated insider these methods.
    setSingleLayerConstants( pMaterial, 0, theLayer, bumpShaderMask, bumped );
    setSingleLayerConstants( pMaterial, 1, theLayer, bumpShaderMask, bumped );
    setSingleLayerConstants( pMaterial, 2, theLayer, bumpShaderMask, bumped );
    setSingleLayerConstants( pMaterial, 3, theLayer, bumpShaderMask, bumped );

    //-- set up the relevant bump shader mask gathered from each individual layer.
    pMaterial->SetParam( pMaterial->bumpShaderMask_, bumpShaderMask );
    pMaterial->SetParam( pMaterial->useMultipassBlending_, blended );
}

/**
 *  This method sets the texture layer constants on a material
 *
 *  @param pMaterial the material to set the constants on
 */
void TerrainRenderer2::setLodTextureConstants(  const TerrainBlock2*    pBlock,
                                                BaseMaterial*           pMaterial,
                                                bool                    doBlends,
                                                bool                    lodNormals )
{
    BW_GUARD;
    const TerrainLodMap2Ptr& pLodMap = pBlock->lodMap();
    if (pLodMap.hasObject())
    {
        const TerrainSettingsPtr& settings = pBlock->settings();

        pMaterial->SetParam( pMaterial->blendMap_, pLodMap->pTexture().pComObject() );
        pMaterial->SetParam( pMaterial->blendMapSize_, float(pLodMap->size()) );
        if (lodNormals)
        {
            pMaterial->SetParam( pMaterial->lodTextureStart_, settings->lodNormalStart() );
            pMaterial->SetParam( pMaterial->lodTextureDistance_, settings->lodNormalDistance()  );
        }
        else
        {
            pMaterial->SetParam( pMaterial->lodTextureStart_, settings->lodTextureStart() );
            pMaterial->SetParam( pMaterial->lodTextureDistance_, settings->lodTextureDistance()  );
        }
        pMaterial->SetParam( pMaterial->useMultipassBlending_, doBlends );
    }
}

/**
 *  This method updates the constants for our terrain block
 *  @param pBlock the block to update constants for
 */
void TerrainRenderer2::updateConstants( const TerrainBlock2*    pBlock,
                                        const TerrainBlendsPtr& pBlends,
                                        BaseMaterial*           pMaterial,
                                        DrawMethod              drawMethod )
{
    BW_GUARD_PROFILER( TerrainRenderer2_updateConstants );

    // always set transform constants
    setTransformConstants( pBlock, pMaterial );

    if ( drawMethod == DM_Z_ONLY )
    {
        // Set up the holes constants
        if ( holeMapEnabled() )
        {
            setHolesMapConstants( pBlock, pMaterial );
        }
    }
    else if ( drawMethod == DM_SHADOW_CAST )
    {
        //-- ToDo: set shadow constants.
    }
    else
    {
        bool lodNormals = (drawMethod == DM_LOD_NORMALS);

        // set normal and horizon for non-z only passes
        setNormalMapConstants( pBlock,  pMaterial, lodNormals );
        setHorizonMapConstants( pBlock, pMaterial );
        setAOMapConstants( pBlock, pMaterial );

        pMaterial->SetParam( pMaterial->hasHoles_, ( pBlock->pHolesMap() && holeMapEnabled() ) );

        if ( drawMethod == DM_SINGLE_LOD || lodNormals )
        {
            // set lod constants if we are rendering with the lod texture .
            // if we are using multi-pass rendering set up blending.
            setLodTextureConstants(
                pBlock, pMaterial,
                pBlock->currentDrawState_.blendsRendered_ || pBlock->currentDrawState_.lodsRendered_,
                lodNormals
                );
        }
        else if ( drawMethod == DM_BLEND || drawMethod == DM_BUMP )
        {
            // set up first layer constants - others are set by draw
            if (pBlends->combinedLayers_.size())
            {
                setTextureLayerConstants( pBlends, pMaterial, 0, false, drawMethod == DM_BUMP );
            }

#if defined(EDITOR_ENABLED) && !defined(IS_OFFLINE_PROCESSOR)

            uint overlayMode =  overlaysEnabled() ? 
                                terrainOverlayMode() : TERRAIN_OVERLAY_MODE_DEFAULT;

            if (overlayMode == TERRAIN_OVERLAY_MODE_TEXTURE_OVERLAY
                && currTextureOverlayLayer_ < textureOverlays.size())
            {
                TextureOverlayLayer *pOverlayLayer = textureOverlays[currTextureOverlayLayer_].get();
                if (pOverlayLayer != NULL)
                {
                    pMaterial->SetParam(pMaterial->overlayMode_, TERRAIN_OVERLAY_MODE_TEXTURE_OVERLAY);
                    pMaterial->SetParam(pMaterial->texOverlay_, pOverlayLayer->texture.pComObject());
                    pMaterial->SetParam(pMaterial->overlayAlpha_, pOverlayLayer->alpha * overlayAlpha);
                    pMaterial->SetParam(pMaterial->overlayColor_, &(pOverlayLayer->color));
                    pMaterial->SetParam(pMaterial->overlayBounds_, &overlayBounds);
                }
            }
            else if (overlayMode == TERRAIN_OVERLAY_MODE_GROUND_TYPES_MAP)
            {
                if(groundTypesTexture.hasComObject()) // extra check for safety
                {
                    pMaterial->pEffect_->pEffect()->pEffect()->SetInt(pMaterial->overlayMode_, TERRAIN_OVERLAY_MODE_GROUND_TYPES_MAP);
                    pMaterial->SetParam(pMaterial->texOverlay_, groundTypesTexture.pComObject());
                    pMaterial->SetParam(pMaterial->overlayAlpha_, overlayAlpha);
                    pMaterial->SetParam(pMaterial->overlayBounds_, &overlayBounds);
                }
            }
#endif

            if (drawMethod == DM_BUMP)
            {
                const TerrainSettingsPtr& settings = pBlock->settings();

                Vector4 bumpFading(
                    settings->bumpFadingStart(),
                    settings->bumpFadingDistance(),
                    0, 0
                    );

                pMaterial->SetParam(pMaterial->bumpFading_, &bumpFading);
            }
        }
        else
        {
            MF_ASSERT( drawMethod == DM_SINGLE_OVERRIDE && "Unknown draw method.");
        }
    }

    if(pMaterial->horizonShadowsBlendDistance) {
        Vector4 v = Vector4(
            horizonShadowsBlendDistance.x, 
            horizonShadowsBlendDistance.y, 
            0.f, 0.f);
        pMaterial->SetParam(pMaterial->horizonShadowsBlendDistance, &v);
    }
}

#ifdef EDITOR_ENABLED

/**
 *
 *
 */
bool TerrainRenderer2::editorGenerateLodTexture(BaseRenderTerrainBlock* pBlock, const Matrix& transform)
{
    BW_GUARD;
    MF_ASSERT(pBlock);

    TerrainBlock2* tb2 = static_cast<TerrainBlock2*>(pBlock);
    Moo::rc().world( transform );

    //-- prepare block for draw.
    if (!tb2->preDraw(true)) 
        return false;

    bool result = true;

    // Grab the blends to be used throughout the render
    const TerrainBlendsPtr&       pBlends         = tb2->currentDrawState_.blendsPtr_;
    const Moo::EffectMaterialPtr& pEffectMaterial = lodMapGeneratorMaterial_.pEffect_;
    
    //-- if blend is invalid.
    if (pBlends->combinedLayers_.empty())
        return false;

    //-- set the vertex declaration
    Moo::rc().setVertexDeclaration(pDecl_->declaration());

    //-- set world transform constant.
    setTransformConstants(tb2, &lodMapGeneratorMaterial_);
    
    if (pEffectMaterial->begin())
    {
        if (pEffectMaterial->beginPass(0))
        {
            for (uint32 i = 0; i < pBlends->combinedLayers_.size(); ++i)
            {
                //-- draw combined layers
                setTextureLayerConstants(pBlends, &lodMapGeneratorMaterial_, i, (i != 0), false);
                pEffectMaterial->commitChanges();

                result &= tb2->draw(pEffectMaterial);
            }
            pEffectMaterial->endPass();
        }
        pEffectMaterial->end();
    }

    //--
    HRESULT hr = Moo::rc().device()->SetStreamSource(1, NULL, 0, 0);
    MF_ASSERT(hr == D3D_OK);

    return result;
}

#endif //-- EDITOR_ENABLED

/**
 *  This method draws a terrain block with a given material and method. This
 *  assumes "set" has been called on the passed in block.
 * 
 *  @param pBlock the block to render.
 *  @param pMaterial the material to use.
 *  @param drawMethod draw as a Z pass, single layer or multi layer.
 *  @param passCount Keeps track of how many passes have been drawn.
 *      caller should set to zero for first pass on a single block.
 */
bool TerrainRenderer2::drawTerrainMaterial( TerrainBlock2*          pBlock,
                                            BaseMaterial*           pMaterial, 
                                            DrawMethod              drawMethod,
                                            uint32&                 passCount )
{
    BW_GUARD_PROFILER( TerrainRenderer2_drawTerrainMaterial );

    bool result = true;

    MF_ASSERT( pBlock );
    MF_ASSERT( pMaterial );

    // Grab the blends to be used throughout the render
    const TerrainBlendsPtr& pBlends = pBlock->currentDrawState_.blendsPtr_;

    // Update constants for this draw method
    updateConstants( pBlock, pBlends, pMaterial, drawMethod );

    // If we're doing Z_ONLY then remember the color write state.
    if ( drawMethod == DM_Z_ONLY )
    {
        //-- push color writing states.
        Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );
        Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE1 );
        Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE2 );
        Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE3 );

        Moo::rc().setWriteMask( 0, 0 );
        Moo::rc().setWriteMask( 1, 0 );
        Moo::rc().setWriteMask( 2, 0 );
        Moo::rc().setWriteMask( 3, 0 );
    }

    // work out whether to do stencil draw or not
    bool doStencilDraw = !Moo::rc().reflectionScene() && useStencilMasking_ &&
        !Terrain::Manager::instance().wireFrame();

    const Moo::EffectMaterialPtr& pEffectMaterial = pMaterial->pEffect_;

    pEffectMaterial->commitChanges();

    if ( pEffectMaterial->begin() )
    {
        for ( uint32 i = 0; i < pEffectMaterial->numPasses(); i++ )
        {
            if ( pEffectMaterial->beginPass(i) )
            {
                // save stencil state for this pass before it begins
                Moo::rc().pushRenderState(D3DRS_STENCILENABLE);     
                Moo::rc().pushRenderState(D3DRS_STENCILREF);
                Moo::rc().pushRenderState(D3DRS_STENCILFUNC);

                // setup for single material
                if ( (DM_BLEND == drawMethod || DM_BUMP == drawMethod) && doStencilDraw )
                {
                    if ( passCount == 0)
                    {
                        // For first pass, write STENCIL_OFFSET to the buffer, without
                        // testing. This is because the stencil value is relevant only for
                        // this particular block.
                        Moo::rc().setRenderState(D3DRS_STENCILENABLE, TRUE);
                        Moo::rc().setRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
                        Moo::rc().setRenderState(D3DRS_STENCILMASK, 0xFF);
                        Moo::rc().setRenderState(D3DRS_STENCILWRITEMASK, 0xFF);
                        Moo::rc().setRenderState(D3DRS_STENCILFAIL , D3DSTENCILOP_KEEP);
                        Moo::rc().setRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
                        Moo::rc().setRenderState(D3DRS_STENCILPASS , D3DSTENCILOP_REPLACE);
                        Moo::rc().setRenderState(D3DRS_STENCILREF, IRendererPipeline::STENCIL_USAGE_TERRAIN | TERRAIN_STENCIL_OFFSET);
                    }
                    else
                    {
                        // For other passes, test and write alternate values.
                        Moo::rc().setRenderState(D3DRS_STENCILENABLE, TRUE);
                        Moo::rc().setRenderState(D3DRS_STENCILFUNC, D3DCMP_NOTEQUAL);
                        Moo::rc().setRenderState(D3DRS_STENCILMASK, rp().customStencilWriteMask());
                        Moo::rc().setRenderState(D3DRS_STENCILWRITEMASK, rp().customStencilWriteMask());

                        Moo::rc().setRenderState(D3DRS_STENCILFAIL , D3DSTENCILOP_KEEP);
                        Moo::rc().setRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
                        Moo::rc().setRenderState(D3DRS_STENCILPASS , D3DSTENCILOP_REPLACE);

                        Moo::rc().setRenderState(D3DRS_STENCILREF, ((passCount%2)+TERRAIN_STENCIL_OFFSET));
                    }

                    passCount++;
                }
                else if (DM_SHADOW_CAST != drawMethod && DM_Z_ONLY != drawMethod)
                {
                    Moo::rc().setRenderState(D3DRS_STENCILENABLE, TRUE);
                    Moo::rc().setRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
                    Moo::rc().setRenderState(D3DRS_STENCILFAIL , D3DSTENCILOP_KEEP);
                    Moo::rc().setRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
                    Moo::rc().setRenderState(D3DRS_STENCILPASS , D3DSTENCILOP_REPLACE);
                    Moo::rc().setRenderState(D3DRS_STENCILMASK, rp().systemStencilWriteMask());
                    Moo::rc().setRenderState(D3DRS_STENCILWRITEMASK, rp().systemStencilWriteMask());
                    Moo::rc().setRenderState(D3DRS_STENCILREF, IRendererPipeline::STENCIL_USAGE_TERRAIN);
                }

                // draw single material
                pBlock->draw( pEffectMaterial );

                // setup for combined layers
                if (DM_BLEND == drawMethod || DM_BUMP == drawMethod)
                {
                    if ( doStencilDraw )
                    {       
                        // setup stencil mask
                        Moo::rc().setRenderState(D3DRS_STENCILENABLE, TRUE);
                        Moo::rc().setRenderState(D3DRS_STENCILFUNC, D3DCMP_NOTEQUAL);
                        Moo::rc().setRenderState(D3DRS_STENCILMASK, rp().customStencilWriteMask());
                        Moo::rc().setRenderState(D3DRS_STENCILWRITEMASK, rp().customStencilWriteMask());

                        Moo::rc().setRenderState(D3DRS_STENCILFAIL , D3DSTENCILOP_KEEP);
                        Moo::rc().setRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
                        Moo::rc().setRenderState(D3DRS_STENCILPASS , D3DSTENCILOP_REPLACE);
                    }

                    for (uint32 j = 1; j < pBlends->combinedLayers_.size(); j++)
                    {
                        if ( doStencilDraw )
                        {
                            // update alternate values
                            Moo::rc().setRenderState(D3DRS_STENCILREF, 
                                ((passCount%2)+TERRAIN_STENCIL_OFFSET));
                            passCount++;
                        }

                        // draw combined layers
                        setTextureLayerConstants( pBlends, pMaterial, j, true, drawMethod == DM_BUMP );
                        pEffectMaterial->commitChanges();
                        result = pBlock->draw( pEffectMaterial ) && result;
                    }

                    if ( doStencilDraw )
                    {       
                        Moo::rc().setRenderState(D3DRS_STENCILMASK, rp().systemStencilWriteMask());
                        Moo::rc().setRenderState(D3DRS_STENCILWRITEMASK, rp().systemStencilWriteMask());
                    }
                }

                // restore old stencil state
                Moo::rc().popRenderState();
                Moo::rc().popRenderState();
                Moo::rc().popRenderState();

                // end pass
                pEffectMaterial->endPass();
            } // if begin pass
        } // for each pass

        pEffectMaterial->end();
    }

    // restore color write state.
    if ( drawMethod == DM_Z_ONLY )
    {
        Moo::rc().popRenderState();
        Moo::rc().popRenderState();
        Moo::rc().popRenderState();
        Moo::rc().popRenderState();
    }

    return result;
}

/**
 *  This method adds a terrain block to the render list.
 *
 *  @param pBlock     The block to render
 *  @param transform  Transformation matrix to use on the terrain block.
 */
void TerrainRenderer2::addBlock(BaseRenderTerrainBlock* pBlock, 
                                const Matrix&           transform )
{
    BW_GUARD;
#if ENABLE_WATCHERS
    if (!isEnabledInWatcher)
        return;
#endif
    TerrainBlock2* pTerrainBlock2 = static_cast<TerrainBlock2*>(pBlock);

    Block block;
    block.transform = transform;
    block.block = pTerrainBlock2;
    block.depthOnly = Moo::rc().depthOnly();

    blocks_.push_back( block );                             

    isVisible_ = true;
}

/**
 *  This method renders all our cached terrain blocks
 *  @param pOverride overridden material
 *  @param clearList whether or not to erase the blocks from the render list
 */
void TerrainRenderer2::drawAll(
    Moo::ERenderingPassType passType, Moo::EffectMaterialPtr pOverride, bool clearList)
{
    BW_GUARD;
    BW_SCOPED_RENDER_PERF_MARKER("Terrain");
    GPU_PROFILER_SCOPE(TerrainRenderer2_drawAll);

    if (blocks_.empty())
    {
        return;
    }


#if ENABLE_WATCHERS
    if (!isEnabledInWatcher)
        return;
#endif
    
    ensureEffectMaterialsUpToDate();

    Moo::rc().effectVisualContext().isOutside( true );

    BlockVector::iterator it = blocks_.begin();
    BlockVector::iterator end = blocks_.end();

    Moo::rc().push();

    BaseMaterial* pOverrideBaseMaterial = NULL;
    if ( pOverride )
    {
        overrideMaterial_.pEffect_ = pOverride;
        overrideMaterial_.getHandles();
        pOverrideBaseMaterial = &overrideMaterial_;
    }

    //-- push color writing states.
    Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );
    Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE1 );
    Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE2 );
    Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE3 );

    //-- render each terrain block
    for (; it != end; ++it)
    {
        // draw a single block with given transform, override material, and
        // use cached lighting state.
        drawSingleInternal(
            passType, it->block.get(), it->transform, it->depthOnly, pOverrideBaseMaterial
            );

        MF_ASSERT( Terrain::BaseTerrainRenderer::instance()->version() ==
            it->block->getTerrainVersion() );

        if (clearList)
            it->block = NULL;
    }

    //-- pop color writing states.
    Moo::rc().popRenderState();
    Moo::rc().popRenderState();
    Moo::rc().popRenderState();
    Moo::rc().popRenderState();

    if (clearList)
    {
        blocks_.clear();    
    }

    Moo::rc().pop();
}


/**
 *  This method clears all our cached blocks.  Usually this is done
 *  automatically in ::drawAll, however in some cases clear needs
 *  to be called explicitly (for example entity shadow casting detects
 *  shadows falling on outdoor terrain blocks and caches them, even
 *  if the terrain is not visible and the shadows won't be drawn.)
 */
void TerrainRenderer2::clearBlocks()
{
    BW_GUARD;

    //-- Note: we need to do this manually because we are using AVectorNoDestructor objects
    //--       instead of just BW::vector.
    BlockVector::iterator it = blocks_.begin();
    BlockVector::iterator end = blocks_.end();
    while (it != end)
    {
        it->block = NULL;
        ++it;
    }

    blocks_.clear();
}

/**
 * Watch accessor function for terrain specular power.
 */
void TerrainRenderer2::specularPower( float power )
{
    BW_GUARD;
    specularInfo_.power_ = power;
    ID3DXEffect* pEffect = texturedMaterial_.pEffect_->pEffect()->pEffect();
    pEffect->SetFloat( texturedMaterial_.specularPower_,        
        specularInfo_.power_ );
}

/**
* Watch accessor function for terrain specular power.
*/
void TerrainRenderer2::specularMultiplier( float multiplier )
{
    BW_GUARD;
    specularInfo_.multiplier_ = multiplier;
    ID3DXEffect* pEffect = texturedMaterial_.pEffect_->pEffect()->pEffect();
    pEffect->SetFloat( texturedMaterial_.specularMultiplier_,       
        specularEnabled() ? specularInfo_.multiplier_ : 0.0f );
}

/**
* Watch accessor function for Fresnel exponent.
*/
void TerrainRenderer2::specularFresnelExp( float exp )
{
    BW_GUARD;
    specularInfo_.fresnelExp_ = exp;
    ID3DXEffect* pEffect = texturedMaterial_.pEffect_->pEffect()->pEffect();
    pEffect->SetFloat( texturedMaterial_.specularFresnelExp_,       
        specularInfo_.fresnelExp_ );
}

/**
* Watch accessor function for terrain Fresnel constant.
*/
void TerrainRenderer2::specularFresnelConstant( float fresnelConstant )
{
    BW_GUARD;
    specularInfo_.fresnelConstant_ = fresnelConstant;
    ID3DXEffect* pEffect = texturedMaterial_.pEffect_->pEffect()->pEffect();
    pEffect->SetFloat( texturedMaterial_.specularFresnelConstant_,
        specularInfo_.fresnelConstant_ );
}


/**
 *  Classes that contain EffectMaterials and their parameter handles
 */

void TerrainRenderer2::BaseMaterial::getHandles()
{
    BW_GUARD;
    ID3DXEffect* pEffect = pEffect_->pEffect()->pEffect();

    world_          = pEffect->GetParameterByName( NULL, "world" );
    terrainScale_   = pEffect->GetParameterByName( NULL, "terrainScale" );
    aoMap_          = pEffect->GetParameterByName( NULL, "aoMap" );
    aoMapSize_      = pEffect->GetParameterByName( NULL, "aoMapSize" );
    normalMap_      = pEffect->GetParameterByName( NULL, "normalMap" );
    normalMapSize_  = pEffect->GetParameterByName( NULL, "normalMapSize" );
    horizonMap_     = pEffect->GetParameterByName( NULL, "horizonMap" );
    horizonMapSize_ = pEffect->GetParameterByName( NULL, "horizonMapSize" );
    holesMap_       = pEffect->GetParameterByName( NULL, "holesMap" );
    holesMapSize_   = pEffect->GetParameterByName( NULL, "holesMapSize" );
    holesSize_      = pEffect->GetParameterByName( NULL, "holesSize" );

    for ( uint i = 0; i < 4; i++ )
    {
        layer_[i]               = pEffect->GetParameterByName( NULL, layerText[i] );
        bumpLayer_[i]           = pEffect->GetParameterByName( NULL, bumpLayerText[i] );
        layerUProjection_[i]    = pEffect->GetParameterByName( NULL, layerUProjText[i] );
        layerVProjection_[i]    = pEffect->GetParameterByName( NULL, layerVProjText[i] );
        layerOverlayColors_[i]  = pEffect->GetParameterByName( NULL, layerOverlayColorText[i] );
    }

    texOverlay_             = pEffect->GetParameterByName(NULL, "texOverlay");
    overlayMode_            = pEffect->GetParameterByName(NULL, "overlayMode");
    overlayBounds_          = pEffect->GetParameterByName(NULL, "overlayBounds");
    overlayAlpha_           = pEffect->GetParameterByName(NULL, "overlayAlpha");
    overlayColor_           = pEffect->GetParameterByName(NULL, "overlayColor");

    blendMap_               = pEffect->GetParameterByName( NULL, "blendMap" );
    blendMapSize_           = pEffect->GetParameterByName( NULL, "blendMapSize" );
    layerMask_              = pEffect->GetParameterByName( NULL, "layerMask" );
    bumpShaderMask_         = pEffect->GetParameterByName( NULL, "bumpShaderMask" );
    bumpFading_             = pEffect->GetParameterByName( NULL, "bumpFading" );

    lodTextureStart_        = pEffect->GetParameterByName( NULL, "lodTextureStart" );
    lodTextureDistance_     = pEffect->GetParameterByName( NULL, "lodTextureDistance" );

    useMultipassBlending_   = pEffect->GetParameterByName( NULL, "useMultipassBlending" );
    hasHoles_               = pEffect->GetParameterByName( NULL, "hasHoles" );
    hasAO_                  = pEffect->GetParameterByName( NULL, "hasAO" );

    // float2 horizonShadowsBlendDistances : ShadowsBlendDistances = {0.0, 0.0}; \
    // float horizonShadowsK : ShadowsK = { 1.0f }; /* 0 -- dark, 1 -- light */ \
    // float dynamicShadowsEnabled : DynamicShadowsEnabled = { 0.0f }; /* 1 - horizon, 0 - disabled*/ \
    //
    horizonShadowsBlendDistance = pEffect->GetParameterByName(NULL, "horizonShadowsBlendDistances");
}

void TerrainRenderer2::TexturedMaterial::getHandles()
{
    BW_GUARD;
    BaseMaterial::getHandles();

    ID3DXEffect* pEffect = pEffect_->pEffect()->pEffect();

    specularPower_              = pEffect->GetParameterByName( NULL, "terrain2Specular.power" );
    specularMultiplier_         = pEffect->GetParameterByName( NULL, "terrain2Specular.multiplier" );
    specularFresnelExp_         = pEffect->GetParameterByName( NULL, "terrain2Specular.fresnelExp" );
    specularFresnelConstant_    = pEffect->GetParameterByName( NULL, "terrain2Specular.fresnelConstant" );
}

/**
 * Sets material parameter if it exists.
 */
void TerrainRenderer2::BaseMaterial::SetParam( D3DXHANDLE param, bool b )
{
    BW_GUARD;
    if ( param )
        pEffect_->pEffect()->pEffect()->SetBool( param, b );
}

void TerrainRenderer2::BaseMaterial::SetParam( D3DXHANDLE param, float f )
{
    BW_GUARD;
    if ( param )
        pEffect_->pEffect()->pEffect()->SetFloat( param, f );
}

void TerrainRenderer2::BaseMaterial::SetParam( D3DXHANDLE param, int i )
{
    BW_GUARD;
    if ( param )
        pEffect_->pEffect()->pEffect()->SetInt( param, i );
}


void TerrainRenderer2::BaseMaterial::SetParam( D3DXHANDLE param, const Vector4* v )
{
    BW_GUARD;
    if ( param )
        pEffect_->pEffect()->pEffect()->SetVector( param, v );
}

void TerrainRenderer2::BaseMaterial::SetParam( D3DXHANDLE param, const Matrix* m )
{
    BW_GUARD;
    if ( param )
        pEffect_->pEffect()->pEffect()->SetMatrix( param, m );
}

void TerrainRenderer2::BaseMaterial::SetParam( D3DXHANDLE param, DX::BaseTexture* t )
{
    BW_GUARD;
    if ( param )
        pEffect_->pEffect()->pEffect()->SetTexture( param, t );
}

/**  
 * This method takes a partial texture flag result (with preload flag possibly
 * set), and returns full result needed for draw. 
 */
uint8 TerrainRenderer2::getDrawFlag(
    const uint8 partialResult, float minDistance, float maxDistance, const TerrainSettingsPtr& pSettings)
{
    BW_GUARD;
    MF_ASSERT_DEBUG( partialResult != 0 );

    bool enableBump = pSettings->bumpMapping() && 
        (minDistance <= pSettings->bumpFadingStart() + pSettings->bumpFadingDistance());

    uint8 fullResult = partialResult;

    // If we can't use lod texture, then flag to draw blends
    if ( !TerrainSettings::useLodTexture() )
    {
        if (enableBump) fullResult |= RTM_DrawBlend | RTM_DrawBump;
        else            fullResult |= RTM_DrawBlend;

        return fullResult;
    }

    // Set texture rendering flags. If the block straddles or is above the 
    // lod texture start it needs lod flag set. Also, if block is inside
    // the blend zone ( start + distance ) then it needs blend flag set.

    // test within lod texture range
    if ( maxDistance >= pSettings->lodTextureStart() && 
        minDistance < (pSettings->lodNormalStart() + pSettings->lodNormalDistance()) )
    {
        fullResult |= RTM_DrawLOD;
    }

    if (maxDistance >= pSettings->lodNormalStart())
        fullResult |= RTM_DrawLODNormals;

    // If the partial result says to preload the blend, it should be tested
    // to see if drawing is required.
    if ( partialResult & RTM_PreLoadBlend )
    {
        // test within blend texture range
        if ( minDistance <= pSettings->lodTextureStart() + pSettings->lodTextureDistance() )
        {
            if (enableBump) fullResult |= RTM_DrawBlend | RTM_DrawBump;
            else            fullResult |= RTM_DrawBlend;
        }
    }

    return  fullResult;
}

#if defined(EDITOR_ENABLED) && !defined(IS_OFFLINE_PROCESSOR)

uint TerrainRenderer2::terrainOverlayMode()
{
    return TERRAIN_OVERLAY_MODE_DEFAULT;
}

void TerrainRenderer2::terrainOverlayMode(uint mode)
{
}

#endif

/**  
* If fx files are updated then we need to detect this and remap the handles
* stored in the effects used. This allows us to alter shader files at
* runtime and see the results without having to restart the application.
* Without this function, if someone alters an fx file while the application
* is running it will either crash or render strangely
*/
void TerrainRenderer2::ensureEffectMaterialsUpToDate()
{
    if (texturedMaterial_.pEffect_->checkEffectRecompiled())
    {
        texturedMaterial_.pEffect_->hTechnique("MAIN");
        texturedMaterial_.getHandles();
    }
    if (texturedReflectionMaterial_.pEffect_->checkEffectRecompiled())
    {
        texturedReflectionMaterial_.pEffect_->hTechnique("REFLECTION");
        texturedReflectionMaterial_.getHandles();
    }
    if (lodTextureMaterial_.pEffect_->checkEffectRecompiled())
    {
        lodTextureMaterial_.pEffect_->hTechnique("MAIN");
        lodTextureMaterial_.getHandles();
    }
    if (lodTextureReflectionMaterial_.pEffect_->checkEffectRecompiled())
    {
        lodTextureReflectionMaterial_.pEffect_->hTechnique("REFLECTION");
        lodTextureReflectionMaterial_.getHandles();
    }
    if (zPassMaterial_.pEffect_->checkEffectRecompiled())
    {
        zPassMaterial_.getHandles();
    }
    if (shadowCastMaterial_.pEffect_->checkEffectRecompiled())
    {
        shadowCastMaterial_.getHandles();
    }
}

} // namespace Terrain

BW_END_NAMESPACE

// terrain_renderer2.cpp
