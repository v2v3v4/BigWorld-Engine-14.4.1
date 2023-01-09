#pragma once

#include "cstdmf/singleton.hpp"
#include "cstdmf/smartpointer.hpp"
#include "moo/forward_declarations.hpp"
#include "moo/moo_math.hpp"
#include "moo/moo_dx.hpp"
#include "moo/com_object_wrap.hpp"
#include "moo/device_callback.hpp"
#include "moo/graphics_settings.hpp"
#include "shadow_manager.hpp"
#include "semi_dynamic_shadow.hpp"
#include <memory>

BW_BEGIN_NAMESPACE

//-- forward declaration.
class HDRSupport;
class SSAOSupport;

namespace Moo
{
    class DrawContext;
    class ShadowManager;
}


//-- Base class represents renderer's pipeline. I.e. it manages how object will draw and how 
//-- they will receive lighting, shadows and any other environment effects.
//-- There are two main types of the pipeline: Deferred Shading and Forward Shading.
//-- The second one is much more suited for low spec machines, but the first one adds a lot of 
//-- additional geometrical info to make advanced effects like shadows, multiple lighting,
//-- deferred decals and so on through some initial overhead for filling fat back buffer,
//-- called GBuffer.
//--------------------------------------------------------------------------------------------------
class IRendererPipeline : public Moo::DeviceCallback
{
private:
    //-- make non-copyable.
    IRendererPipeline(const IRendererPipeline&);
    IRendererPipeline& operator = (const IRendererPipeline&);

public:

    //-- graphics setting specific for each rendering pipeline.
    enum EGraphicsSetting
    {
        GRAPHICS_SETTING_SHADOWS_QUALITY = 0,
        GRAPHICS_SETTING_DECALS_QUALITY,
        GRAPHICS_SETTING_LIGHTING_QUALITY,
        GRAPHICS_SETTING_COUNT
    };

    //-- pipeline types.
    enum EType
    {
        TYPE_DEFERRED_SHADING,
        TYPE_FORWARD_SHADING
    };

    //-- Type of an object on the screen.
    //-- Note: 4 highest bits of stencil are reserved by system for storing pixel type.
    //--       4 lower bits of stencil are free fro custom use.
    enum EStencilUsage
    {
        STENCIL_USAGE_TERRAIN       = 1 << 4,
        STENCIL_USAGE_SPEEDTREE     = 1 << 5,
        STENCIL_USAGE_FLORA         = 1 << 6,
        STENCIL_USAGE_OTHER_OPAQUE  = 1 << 7
    };

public:
    IRendererPipeline() { }
    virtual ~IRendererPipeline() = 0 { }

    virtual bool                init() = 0;
    virtual void                tick(float dt) = 0;

    virtual void                begin() = 0;
    virtual void                end() = 0;

    virtual void                drawDebugStuff() = 0;

    virtual void                beginCastShadows( Moo::DrawContext& shadowDrawContext ) = 0;
    virtual void                endCastShadows() = 0;

    //-- begin/end drawing opaque objects.
    virtual void                beginOpaqueDraw() = 0;
    virtual void                endOpaqueDraw() = 0;

    //-- apply lights and shadows to the scene.
    virtual void                applyLighting() = 0;
    
    //-- begin/end drawing transparent objects.
    virtual void                beginSemitransparentDraw() = 0;
    virtual void                endSemitransparentDraw() = 0;

    //-- begin/end gui drawing.
    virtual void                beginGUIDraw() = 0;
    virtual void                endGUIDraw() = 0;

    //-- ToDo:
    virtual void                onGraphicsOptionSelected(EGraphicsSetting setting, int option) = 0;

    //-- returns object responsible for doing HDR Rendering and only if it's available.
    virtual HDRSupport*         hdrSupport() const { return NULL; }

    //-- returns object responsible for doing SSAO.
    virtual SSAOSupport*        ssaoSupport() const { return NULL; }

    //-- shadow managers
    virtual Moo::ShadowManager* dynamicShadow() const { return NULL; }
    
    //-- provides some additional info about the scene. Available only in the DS mode.
    virtual DX::Texture*        gbufferTexture(uint idx) const = 0;
    virtual DX::Surface*        gbufferSurface(uint idx) const = 0;
    
    //-- makes copy from g-buffer idx layer and returns it.
    virtual DX::BaseTexture*    getGBufferTextureCopyFrom(uint idx) const = 0;

    //-- color writing usage
    virtual void                setupColorWritingMask(bool flag) const = 0;
    virtual void                restoreColorWritingMask() const = 0;

    //-- stencil usage.
    uint32                      systemStencilWriteMask() const  { return 0xF0; }
    uint32                      customStencilWriteMask() const  { return 0x0F; }

    void                        resetStencil(uint8 stencilMask = 0xFF) const;
    void                        setupSystemStencil(EStencilUsage usage) const;
    void                        restoreSystemStencil() const;

    //-- interface DeviceCallback.
    virtual bool                recreateForD3DExDevice() const { return true; }
    virtual void                createUnmanagedObjects()    { }
    virtual void                deleteUnmanagedObjects()    { }
    virtual void                createManagedObjects()      { }
    virtual void                deleteManagedObjects()      { }
};


//-- Class manages current renderer pipeline.
//--------------------------------------------------------------------------------------------------
class Renderer : public Singleton<Renderer>
{
private:
    //-- make non-copyable.
    Renderer(const Renderer&);
    Renderer& operator = (const Renderer&);

public:
    Renderer();
    ~Renderer();
    
    bool                        init(bool isAssetProcessor = false, bool supportDS = false);
    void                        tick(float dt);
    IRendererPipeline::EType    pipelineType() const;
    IRendererPipeline*          pipeline() const;

private:
    void initGraphicsSettings();

    //--
    template<IRendererPipeline::EGraphicsSetting setting>
    void onGraphicsOptionChanged(int option)
    {
        m_options[setting] = option;

        if (m_inited)
        {
            m_pipeline->onGraphicsOptionSelected(setting, option);
        }
    }

private:
    bool                                        m_inited;
    IRendererPipeline::EType                    m_type;
    std::auto_ptr<IRendererPipeline>            m_pipeline;

    //-- common graphics settings.
    int                                         m_options[IRendererPipeline::GRAPHICS_SETTING_COUNT];
    Moo::GraphicsSetting::GraphicsSettingPtr    m_settings[IRendererPipeline::GRAPHICS_SETTING_COUNT];
};

//-- shortcut to the current render pipeline.
IRendererPipeline& rp();

BW_END_NAMESPACE
