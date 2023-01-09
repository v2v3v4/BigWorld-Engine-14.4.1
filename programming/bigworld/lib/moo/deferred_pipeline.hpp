#pragma once

#include "renderer.hpp"
#include <memory>

BW_BEGIN_NAMESPACE

//-- forward declaration.
class DecalsManager;
class LightsManager;
class HDRSupport;

namespace Moo
{
    class ShadowManager;
    class SemiDynamicShadow;
}

//-- Optimizes speedtree rendering through the sniper camera.
//----------------------------------------------------------------------------------------------
class SpeedTreeOptimizer : public Moo::DeviceCallback
{
public:
    void            draw();
    virtual void    createManagedObjects();
    virtual void    deleteManagedObjects();

private:
    Moo::RenderTargetPtr    m_rt1;
    Moo::RenderTargetPtr    m_rt2;
    Moo::EffectMaterialPtr  m_mat;
    Moo::EffectMaterialPtr  m_blurMat;
};


//-- ToDo: document.
//----------------------------------------------------------------------------------------------
class DeferredPipeline : public IRendererPipeline
{
public:
    DeferredPipeline();
    virtual ~DeferredPipeline();

    virtual bool                init();
    virtual void                tick(float dt);

    //-- begin/end pipeline.
    virtual void                begin();
    virtual void                end();

    virtual void                drawDebugStuff();

    virtual void                beginCastShadows( Moo::DrawContext& shadowDrawContext );
    virtual void                endCastShadows();

    //-- begin/end drawing opaque objects.
    virtual void                beginOpaqueDraw();
    virtual void                endOpaqueDraw();

    //-- begin/end drawing semitransparent objects.
    virtual void                beginSemitransparentDraw();
    virtual void                endSemitransparentDraw();

    //-- begin/end gui drawing.
    virtual void                beginGUIDraw();
    virtual void                endGUIDraw();

    virtual void                onGraphicsOptionSelected(EGraphicsSetting setting, int option);

    //-- apply lighting to the scene.
    virtual void                applyLighting();

    //-- returns object responsible for doing HDR Rendering.
    virtual HDRSupport*         hdrSupport() const { return m_HDRSupport.get(); }

    //-- returns object responsible for doing SSAO.
    virtual SSAOSupport*        ssaoSupport() const { return m_SSAOSupport.get(); }

    //-- shadow managers
    virtual Moo::ShadowManager* dynamicShadow() const { return m_dynamicShadowManager.get(); }

    //-- g-buffer data accessors.
    virtual DX::Texture*        gbufferTexture(uint idx) const  { return m_rts[idx].pComObject(); }
    virtual DX::Surface*        gbufferSurface(uint idx) const  { return m_surfaces[idx].pComObject(); }

    //-- makes copy from g-buffer idx layer and returns it.
    virtual DX::BaseTexture*    getGBufferTextureCopyFrom(uint idx) const;

    //-- color writing usage
    virtual void                setupColorWritingMask(bool flag) const;
    virtual void                restoreColorWritingMask() const;

    //-- interface DeviceCallback.
    virtual void                createUnmanagedObjects();
    virtual void                deleteUnmanagedObjects();
    virtual void                createManagedObjects();
    virtual void                deleteManagedObjects();
    virtual bool                recreateForD3DExDevice() const;

private:

    //-- debug visualization of desired changes of g-buffer.
    enum EGBufferChannel
    {
        G_BUFFER_CHANNEL_DEPTH = 0,
        G_BUFFER_OBJECT_KIND,
        G_BUFFER_CHANNEL_ALBEDO,
        G_BUFFER_CHANNEL_NORMAL,
        G_BUFFER_CHANNEL_BACKED_SHADOWS,
        G_BUFFER_CHANNEL_SPEC_AMOUNT,
        G_BUFFER_CHANNEL_MATERIAL_ID,
        G_BUFFER_CHANNEL_COUNT
    };

    //-- show g-buffer channel.
    void displayGBufferChannel(EGBufferChannel idx);
    void drawPostDeferred();
private:
    
    enum { COLOR_BUFFERS = 3 };

    bool                            m_enabled;

    //-- ToDo: reconsider.
    std::auto_ptr<HDRSupport>  m_HDRSupport;
    std::auto_ptr<SSAOSupport> m_SSAOSupport;
    
    //-- copy of one of the g-buffer textures.
    //-- Note: for now this copy is only for RGBA8 format.
    Moo::RenderTargetPtr            m_gbufferCopyRT;

    //-- g-buffer layout.
    ComObjectWrap<DX::Texture>      m_rts[COLOR_BUFFERS];
    ComObjectWrap<DX::Surface>      m_surfaces[COLOR_BUFFERS];

    //-- sub-systems.
    std::auto_ptr<DecalsManager>    m_decalsManager;
    std::auto_ptr<LightsManager>    m_lightsManager;
    std::auto_ptr<Moo::ShadowManager> m_dynamicShadowManager;
    std::auto_ptr<SpeedTreeOptimizer> m_spt;
};

BW_END_NAMESPACE
