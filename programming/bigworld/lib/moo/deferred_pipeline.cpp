#include "pch.hpp"

#include "deferred_pipeline.hpp"
#include "deferred_decals_manager.hpp"
#include "deferred_lights_manager.hpp"
#include "shadow_manager.hpp"
#include "semi_dynamic_shadow.hpp"
#include "romp/hdr_support.hpp"
#include "romp/ssao_support.hpp"
#include "moo/fullscreen_quad.hpp"
#include "moo/moo_utilities.hpp"
#include "moo/render_target.hpp"
#include "moo/texture_manager.hpp"
#include "moo/effect_visual_context.hpp"
#include "romp/hdr_support.hpp"
#include "cstdmf/dogwatch.hpp"
#include "cstdmf/watcher.hpp"
#include "resmgr/auto_config.hpp"
#include "speedtree/speedtree_renderer.hpp"
#include "romp/flora.hpp"
#include "moo/line_helper.hpp"

BW_BEGIN_NAMESPACE

//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace 
{
    //-- watcher variable for debug displaying g-buffer channels.
    int  g_showGBufferChannel = -1;

    //-- speedtree optimizer watchers.
    bool g_useSpeedTreeOptimizer = true;
    bool g_drawSemiTransparentTrees = true;
    bool g_blurSemiTransparentTrees = true;

    //-- for optimization gain we disable clearing the g-buffer render targets.
    bool g_clearGBuffer =
#ifdef EDITOR_ENABLED
    true;
#else
    false;
#endif

    //--
    const D3DRENDERSTATETYPE g_colorWriteMask[] = {
        D3DRS_COLORWRITEENABLE,
        D3DRS_COLORWRITEENABLE1,
        D3DRS_COLORWRITEENABLE2,
        D3DRS_COLORWRITEENABLE3
    };

    //----------------------------------------------------------------------------------------------
    class GBufferChannelConstant : public Moo::EffectConstantValue
    {
    public:
        GBufferChannelConstant(int channel, const DeferredPipeline& dp) : m_channel(channel), m_dp(dp)
        {
        }

        bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
        {
            BW_GUARD;

            pEffect->SetTexture(constantHandle, m_dp.gbufferTexture(m_channel));
            return true;
        }

    private:
        const DeferredPipeline& m_dp;
        const int               m_channel;
    };


    //-- Helper class for visualization a texture as a full-screen quad.
    //----------------------------------------------------------------------------------------------
    class MapVisualizer
    {
    public:

        //-- struct describes channel display info. Offset mask uses to indicate which component and in
        //-- which order we will need and visibility mask indicates which channels to display.
        struct Desc
        {
            Desc(const Vector4& offsetMask, const Vector4& visibilityMask, DX::BaseTexture* map)
                :   m_offsetMask(offsetMask), m_visibilityMask(visibilityMask), m_map(map) { }

            Vector4             m_offsetMask;
            Vector4             m_visibilityMask;
            DX::BaseTexture*    m_map;
        };

    public:
        MapVisualizer()
        {
            bool success = createEffect(m_material, "shaders/std_effects/debug_g_buffer.fx");
            MF_ASSERT_DEV(success && "Not all system resources have been loaded correctly.");
        }

        //------------------------------------------------------------------------------------------
        void visualize(const Desc& desc)
        {
            BW_GUARD;

            //-- set shader properties and display channel as a fullscreen quad.
            ID3DXEffect* effect = m_material->pEffect()->pEffect();
            {
                effect->SetTexture  ("g_srcMap", desc.m_map);
                effect->SetVector   ("g_offsetMask", &desc.m_offsetMask);
                effect->SetVector   ("g_visibilityMask", &desc.m_visibilityMask);
                effect->CommitChanges();
            }
            Moo::rc().fsQuad().draw(*m_material.get());
        }

    private:
        Moo::EffectMaterialPtr m_material;
    };
    std::auto_ptr<MapVisualizer> g_mapVisualizer;

    //----------------------------------------------------------------------------------------------
    void doGaussianBlur(
        Moo::RenderTarget* inoutRT, Moo::RenderTarget* inRT, Moo::EffectMaterial& mat, DX::Viewport& vp)
    {
        //-- prepare common constants.
        uint    vpWidth  = vp.Width;
        uint    vpHeight = vp.Height;
        Vector4 screen   = Vector4(
            static_cast<float>(vp.Width), static_cast<float>(vp.Height), 1.0f / vp.Width, 1.0f / vp.Height
            );

        //-- 1. do horizontal blur stage.
        mat.hTechnique("HORIZONTAL_PASS");
        if (inRT->push())
        {
            Moo::rc().setViewport(&vp);

            ID3DXEffect* effect = mat.pEffect()->pEffect();
            {
                effect->SetTexture("g_srcMap", inoutRT->pTexture());
                effect->SetVector ("g_srcSize", &screen);
                mat.commitChanges();

                Moo::rc().fsQuad().draw(mat, vpWidth, vpHeight);
            }

            inRT->pop();
        }

        //-- 2. do vertical blur stage.
        mat.hTechnique("VERTICAL_PASS");
        if (inoutRT->push())
        {
            Moo::rc().setViewport(&vp);

            ID3DXEffect* effect = mat.pEffect()->pEffect();
            {
                effect->SetTexture("g_srcMap", inRT->pTexture());
                effect->SetVector ("g_srcSize", &screen);
                mat.commitChanges();

                Moo::rc().fsQuad().draw(mat, vpWidth, vpHeight);
            }

            inoutRT->pop();
        }
    }
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.

//-- ToDo: reconsider.
//extern AutoConfigString s_shadowsXML;

using namespace Moo;

//--------------------------------------------------------------------------------------------------
DeferredPipeline::DeferredPipeline()
    :   m_decalsManager(new DecalsManager()),
        m_lightsManager(new LightsManager()),
        m_dynamicShadowManager(new ShadowManager()),
        m_HDRSupport(new HDRSupport()),
        m_SSAOSupport(new SSAOSupport()),
        m_spt(new SpeedTreeOptimizer())
{
    MF_WATCH("Render/DS/show g-buffer channel",
        g_showGBufferChannel, Watcher::WT_READ_WRITE,
        "-1 nothing to display, 0 - depth, 1 - object kind, 2 - albedo,"
        "3 - normal, 4 - user data #2, 5 - spec amount, 6 - user data #1"
        );

    MF_WATCH("Render/DS/clear g-buffer",
        g_clearGBuffer, Watcher::WT_READ_WRITE,
        "Disable clearing g-buffers render targets to save fillrate. On some system that may give "
        "additional performance boost."
        );

    MF_WATCH("Render/DS/use speedtree optimizer",
        g_useSpeedTreeOptimizer, Watcher::WT_READ_WRITE,
        "ToDo: document."
        );

    MF_WATCH("Render/DS/draw smit-transparent trees",
        g_drawSemiTransparentTrees, Watcher::WT_READ_WRITE,
        "ToDo: document."
        );

    MF_WATCH("Render/DS/blur smit-transparent trees",
        g_blurSemiTransparentTrees, Watcher::WT_READ_WRITE,
        "ToDo: document."
        );
}

//--------------------------------------------------------------------------------------------------
DeferredPipeline::~DeferredPipeline()
{
#if ENABLE_WATCHERS
    if (Watcher::hasRootWatcher())
    {
        Watcher::rootWatcher().removeChild("Render/DS");
    }
#endif //-- ENABLE_WATCHERS

    //-- ToDo: reconsider. Why we need to call fini method. May be the all operation performed in
    //--       this method we can move to destructor of class ShadowManager.
    m_dynamicShadowManager->fini();
    m_dynamicShadowManager.reset();
}

//--------------------------------------------------------------------------------------------------
bool DeferredPipeline::init()
{
    BW_GUARD;
    bool success = true;

    //-- initialize sub-system related to this pipeline: shadows, decals, lighting and so on.
    success &= m_decalsManager->init(); 
    success &= m_lightsManager->init();

    //-- initialize shadows sub-system.
    //-- ToDo: reconsider.
    //DataSectionPtr spSection = BWResource::instance().openSection(s_shadowsXML);
    success &= m_dynamicShadowManager->init(/*spSection*/NULL);

    return success;
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::tick(float dt)
{
    BW_GUARD;
    m_decalsManager->tick(dt);
    m_lightsManager->tick(dt);
    m_dynamicShadowManager->tick(dt);
    m_HDRSupport->tick(dt);
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::begin()
{
    BW_GUARD;

    DX::Viewport viewport;
    Moo::rc().getViewport(&viewport);
    viewport.Width  = (DWORD)Moo::rc().screenWidth();
    viewport.Height = (DWORD)Moo::rc().screenHeight();
    viewport.X      = 0;
    viewport.Y      = 0;
    Moo::rc().setViewport(&viewport);

    uint32 clearFlags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
    if (Moo::rc().stencilAvailable())
    {
        clearFlags |= D3DCLEAR_STENCIL;
    }

    Moo::rc().device()->Clear(0, NULL, clearFlags, Moo::Colour(0,0,0,0), 1, 0);

    //-- reset stencil.
    resetStencil(customStencilWriteMask());
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::end()
{
    BW_GUARD;
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::drawDebugStuff()
{
    BW_GUARD;

    LineHelper::instance().purge();

    //-- display g-buffer channel.
    if (g_showGBufferChannel != -1)
    {
        uint channel = Math::clamp<uint>(0, g_showGBufferChannel, G_BUFFER_CHANNEL_COUNT - 1);
        displayGBufferChannel(static_cast<EGBufferChannel>(channel));

        g_showGBufferChannel = channel;
    }
    else if (m_SSAOSupport->showBuffer())
    {
        g_mapVisualizer->visualize(MapVisualizer::Desc(
            Vector4(0,0,0,0), Vector4(1,1,1,0), m_SSAOSupport->screenSpaceAmbienOcclusionMap()
            ));
    }
    else if (m_decalsManager->showStaticDecalsAtlas())
    {
        g_mapVisualizer->visualize(MapVisualizer::Desc(
            Vector4(0,1,2,0), Vector4(1,1,1,0), m_decalsManager->staticDecalAtlas()
            ));
    }

    //-- ToDo: reconsider.
    m_dynamicShadowManager->drawDebugInfo();
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::beginCastShadows( Moo::DrawContext& shadowDrawContext )
{
    BW_GUARD;
    
    m_dynamicShadowManager->cast( shadowDrawContext );
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::endCastShadows()
{
    BW_GUARD;
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::beginOpaqueDraw()
{
    BW_GUARD;

    //-- save previous device state before drawing into the GBuffer.
    Moo::rc().pushRenderTarget();

    for (uint i = 0; i < COLOR_BUFFERS; ++i)
    {
        Moo::rc().setWriteMask(i, 0xFFFFFFFF);
        Moo::rc().setRenderTarget(i, m_surfaces[i].pComObject());
    }
    
    //-- Note: clear only color RTs, depth-stencil surface cleared from the main code.
    if (g_clearGBuffer)
    {
        Moo::rc().device()->Clear(0, NULL, D3DCLEAR_TARGET, Moo::Colour(0,0,0,0), 1, 0);
    }

    //-- Note: depth stencil buffer already installed, so don't worry about it. 

    //-- prepare system stencil usage.
    setupSystemStencil(STENCIL_USAGE_OTHER_OPAQUE);
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::endOpaqueDraw()
{
    BW_GUARD;

    //-- prepare stencil usage.
    restoreSystemStencil();

    //-- restore state.
    Moo::rc().popRenderTarget();

    //--
    Moo::rc().setWriteMask(0, D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN);
    Moo::rc().setWriteMask(1, 0);
    Moo::rc().setWriteMask(2, 0);
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::beginSemitransparentDraw()
{
    BW_GUARD;
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::endSemitransparentDraw()
{
    BW_GUARD;

    //-- draw semitransparent trees.
    m_spt->draw();

    m_HDRSupport->resolve();
    drawPostDeferred();
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::beginGUIDraw()
{
    BW_GUARD;

    resetStencil();
    Moo::rc().pushRenderTarget();
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::endGUIDraw()
{
    BW_GUARD;

    Moo::rc().popRenderTarget();
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::onGraphicsOptionSelected(EGraphicsSetting setting, int option)
{
    BW_GUARD;

    switch (setting)
    {
    case GRAPHICS_SETTING_SHADOWS_QUALITY:
        {
            m_dynamicShadowManager->setQualityOption(option);
            break;
        }
    case GRAPHICS_SETTING_DECALS_QUALITY:
        {
            m_decalsManager->setQualityOption(option);
            break;
        }
    case GRAPHICS_SETTING_LIGHTING_QUALITY:
        {
            //-- ToDo (b_sviglo): reconsider.
            m_SSAOSupport->setQualityOption(option);
            m_HDRSupport->setQualityOption(option);
            break;
        }
    default:
        {
            MF_ASSERT(!"Undefined graphics setting.");
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::applyLighting()
{
    BW_GUARD;

    //-- save stencil state because it's used for optimization in underlying sub-systems.
    Moo::rc().pushRenderState( D3DRS_STENCILENABLE );
    //-- save color write because it's modified by sun material pass
    Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );


    //-- draw decals.
    m_decalsManager->draw();

    //-- resolve SSAO
    m_SSAOSupport->resolve();

    //-- resolve shadows to screen space shadow map.
    m_dynamicShadowManager->receive();

    //-- 
    m_HDRSupport->intercept();

    m_lightsManager->draw();

    Moo::rc().popRenderState();
    Moo::rc().popRenderState();
}

//--------------------------------------------------------------------------------------------------
DX::BaseTexture* DeferredPipeline::getGBufferTextureCopyFrom(uint idx) const
{
    BW_GUARD;

    copySurface(m_gbufferCopyRT.get(), m_surfaces[idx]);
    return m_gbufferCopyRT->pTexture();
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::setupColorWritingMask(bool flag) const
{
    BW_GUARD;

    for (uint i = 0; i < COLOR_BUFFERS; ++i)
    {
        Moo::rc().pushRenderState(g_colorWriteMask[i]);
        Moo::rc().setRenderState(g_colorWriteMask[i], flag ? 0xFF : 0x00);
    }
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::restoreColorWritingMask() const
{
    BW_GUARD;

    for (uint i = 0; i < COLOR_BUFFERS; ++i)
    {
        Moo::rc().popRenderState();
    }
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::createUnmanagedObjects()
{
    BW_GUARD;

    bool success = true;

    //-- g-buffer layout.
    //-- Warning: should be in sync with G_BUFFER_LAYOUT in write_g_buffer.fxh.
    const D3DFORMAT dxFormats[] =
    {
        D3DFMT_A8R8G8B8, //-- RGBA8: RGB - depth. A - object kind.
        D3DFMT_A8R8G8B8, //-- RGBA8: RG - normal. B - specular amount. A - user data #2     
        D3DFMT_A8R8G8B8  //-- RGBA8: RGB - albedo (diffuse) color, A - user data #1.
    };

    //-- create g-buffer render targets.
    for (uint i = 0; i < COLOR_BUFFERS; ++i)
    {
        //-- create render target.
        m_rts[i] =  Moo::rc().createTexture(
            static_cast<UINT>(Moo::rc().screenWidth()), static_cast<UINT>(Moo::rc().screenHeight()),
            1, D3DUSAGE_RENDERTARGET, dxFormats[i], D3DPOOL_DEFAULT, "g-buffer channel"
            );
        success &= (m_rts[i].pComObject() != NULL);
                    
        //-- extract surface from the render target.
        if (success)
        {
            success &= SUCCEEDED(m_rts[i]->GetSurfaceLevel(0, &m_surfaces[i]));
        }

        MF_ASSERT(success && "Can't create some one of the gBuffer's color render targets.");
    }

    Moo::BaseTexturePtr pTex = Moo::TextureManager::instance()->get("backBufferCopy", false, true, false);
    if (pTex.get())
    {
        //-- ToDo: more checking.
        m_gbufferCopyRT = dynamic_cast<Moo::RenderTarget*>(pTex.get());
        MF_ASSERT(m_gbufferCopyRT.exists());
        INFO_MSG("FloraRendere - sharing PostProcessing render target.\n");
    }
    else
    {
        const D3DSURFACE_DESC& bbDesc = Moo::rc().backBufferDesc();

        createRenderTarget(
            m_gbufferCopyRT, bbDesc.Width, bbDesc.Height, D3DFMT_A8R8G8B8, "g-buffer copy map", true, true
            );
    }
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::deleteUnmanagedObjects()
{
    BW_GUARD;

    for (uint i = 0; i < COLOR_BUFFERS; ++i)
    {
        m_rts[i] = NULL;
        m_surfaces[i] = NULL;
    }

    m_gbufferCopyRT = NULL;
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::createManagedObjects()
{
    BW_GUARD;

    //-- register shared auto-constants.
    Moo::rc().effectVisualContext().registerAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "GBufferChannel0",
        new GBufferChannelConstant(0, *this)
        );

    Moo::rc().effectVisualContext().registerAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "GBufferChannel1",
        new GBufferChannelConstant(1, *this)
        );

    Moo::rc().effectVisualContext().registerAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "GBufferChannel2",
        new GBufferChannelConstant(2, *this)
        );
    
    //--
    g_mapVisualizer.reset(new MapVisualizer());
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::deleteManagedObjects()
{
    BW_GUARD;

    //-- unregister shared auto-constant.
    Moo::rc().effectVisualContext().unregisterAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "GBufferChannel0"
        );

    Moo::rc().effectVisualContext().unregisterAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "GBufferChannel1"
        );

    Moo::rc().effectVisualContext().unregisterAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "GBufferChannel2"
        );

    //--
    g_mapVisualizer.reset();
}

//--------------------------------------------------------------------------------------------------
bool DeferredPipeline::recreateForD3DExDevice() const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::displayGBufferChannel(EGBufferChannel idx)
{
    BW_GUARD;
    MF_ASSERT(idx < G_BUFFER_CHANNEL_COUNT);

    //-- prepare channels info.
    MapVisualizer::Desc channels[] =
    {
        MapVisualizer::Desc(Vector4(0,1,2,0), Vector4(1,1,1,0), m_rts[0].pComObject()), //-- DEPTH
        MapVisualizer::Desc(Vector4(3,3,3,0), Vector4(1,1,1,0), m_rts[0].pComObject()), //-- OBJECT_KIND
        MapVisualizer::Desc(Vector4(0,1,2,0), Vector4(1,1,1,0), m_rts[2].pComObject()), //-- ALBEDO
        MapVisualizer::Desc(Vector4(0,1,0,0), Vector4(1,1,0,0), m_rts[1].pComObject()), //-- NORMAL
        MapVisualizer::Desc(Vector4(3,3,3,0), Vector4(1,1,1,0), m_rts[1].pComObject()), //-- USER_DATA #2
        MapVisualizer::Desc(Vector4(2,2,2,0), Vector4(1,1,1,0), m_rts[1].pComObject()), //-- SPEC_AMOUNT
        MapVisualizer::Desc(Vector4(3,3,3,0), Vector4(1,1,1,0), m_rts[2].pComObject())  //-- USER_DATA #1
    };

    g_mapVisualizer->visualize(channels[idx]);
}

//--------------------------------------------------------------------------------------------------
void DeferredPipeline::drawPostDeferred()
{
    BW_GUARD;

    //-- ToDo: reconsider.
    const BW::vector< Flora* > &floras = Flora::floras();
    BW::vector< Flora* >::const_iterator it;
    for( it = floras.begin(); it != floras.end(); it++ )
        (*it)->drawPostDeferred();
}

//--------------------------------------------------------------------------------------------------
void SpeedTreeOptimizer::draw()
{
    BW_GUARD;

#if 0

    //-- 1. use usual way of drawing semi-tranparent trees.
    if (!speedtree::SpeedTreeRenderer::s_hideClosest || !g_useSpeedTreeOptimizer)
    {
        speedtree::SpeedTreeRenderer::drawSemitransparent(!g_drawSemiTransparentTrees);
        return;
    }

    //-- 2. draw semi-transparent tress into the small buffer.
    if (m_rt1->push())
    {
        //-- setup view port.
        DX::Viewport vp;
        Moo::rc().getViewport(&vp);
        vp.Width  = m_rt1->width();
        vp.Height = m_rt1->height();
        Moo::rc().setViewport(&vp);
        Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_SCREEN);

        uint32 clearFlags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;
        Moo::rc().device()->Clear(0, NULL, clearFlags, Colour(0,0,0,0), 1, 0);

        ID3DXEffect* effect = m_mat->pEffect()->pEffect();
        {
            effect->SetBool("g_useAlpha", true);
            effect->CommitChanges();
        }

        //--
        speedtree::SpeedTreeRenderer::drawSemitransparent(!g_drawSemiTransparentTrees);

        {
            effect->SetBool("g_useAlpha", false);
            effect->CommitChanges();
        }

        m_rt1->pop();
    }

    //-- 3. do optional blur pass.
    if (g_blurSemiTransparentTrees)
    {
        //-- setup view port.
        DX::Viewport vp;
        Moo::rc().getViewport(&vp);
        vp.Width  = m_rt1->width();
        vp.Height = m_rt1->height();

        doGaussianBlur(m_rt1.get(), m_rt2.get(), *m_blurMat.get(), vp);
    }

    //-- 4. and finally apply this image (upsample) on top of the back-buffer.
    {
        Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_SCREEN);

        m_mat->hTechnique("UPSAMPLE_RT");
        ID3DXEffect* effect = m_mat->pEffect()->pEffect();
        {
            effect->SetTexture("g_srcMap", m_rt1->pTexture());
            effect->CommitChanges();
        }

        Moo::rc().fsQuad().draw(*m_mat.get());
    }
#endif
}

//--------------------------------------------------------------------------------------------------
void SpeedTreeOptimizer::createManagedObjects()
{
    BW_GUARD;

    bool success = true;

    m_rt1 = new Moo::RenderTarget("speedtree/down sampler rt #1");
    m_rt2 = new Moo::RenderTarget("speedtree/down sampler rt #2");

    success &= m_rt1->create(-2, -2, false, D3DFMT_A16B16G16R16F, 0, D3DFMT_UNKNOWN, false);
    success &= m_rt2->create(-2, -2, false, D3DFMT_A16B16G16R16F, 0, D3DFMT_UNKNOWN, false);
    success &= createEffect(m_mat, "shaders/speedtree/semitransparent.fx");
    success &= createEffect(m_blurMat, "shaders/hdr/fast_gaussian_blur.fx");

    MF_ASSERT(success && "Not all system resources have been loaded correctly.");
}

//--------------------------------------------------------------------------------------------------
void SpeedTreeOptimizer::deleteManagedObjects()
{
    BW_GUARD;
    
    m_rt1 = NULL;
    m_rt2 = NULL;
    m_mat = NULL;
    m_blurMat = NULL;
}

BW_END_NAMESPACE
