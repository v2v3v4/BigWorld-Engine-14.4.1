#include "pch.hpp"

#include "renderer.hpp"
#include "forward_pipeline.hpp"
#include "deferred_pipeline.hpp"
#include "moo/render_context.hpp"
#include "moo/managed_effect.hpp"
#include "moo/effect_macro_setting.hpp"
#include "moo/graphics_settings_picker.hpp"
#include "romp/ssao_support.hpp"

BW_BEGIN_NAMESPACE

//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
    
    //-- This class is some extension for the existing EffectMacroSetting class. By design the effect
    //-- macro setting updates its state only after creating EffectManager and creating render
    //-- device, but we want to get value before these events. This class help us use one setting for
    //-- controlling which one render pipeline currently enabled, and which one shader's set is used.
    //----------------------------------------------------------------------------------------------
    class RenderPipelineSetting : public Moo::EffectMacroSetting
    {
    public:
        RenderPipelineSetting(
            const BW::string& label, const BW::string& desc, const BW::string& macro, SetupFunc setupFunc
            )   :   EffectMacroSetting(label, desc, macro, setupFunc, true), m_fullyInited(false)
        {
        
        }

        virtual void onOptionSelected(int optionIndex)
        {
            if (m_fullyInited)
            {
                Moo::EffectMacroSetting::onOptionSelected(optionIndex);
            }
            m_fullyInited = true;
        }

    private:
        bool m_fullyInited;
    };

    //----------------------------------------------------------------------------------------------
    void configureKeywordSetting(Moo::EffectMacroSetting&)
    {
        
    }

    //-- For now there will be only one EffectMacroSetting called RENDER_PIPELINE and it gives us
    //-- opportunity to change shaders frameworks with respect to the current render pipeline.
    Moo::EffectMacroSetting::EffectMacroSettingPtr g_pipelineMacroSetting =
        new RenderPipelineSetting(
            "RENDER_PIPELINE", "Render pipeline", "BW_DEFERRED_SHADING", &configureKeywordSetting
            );
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.

//-- define singleton.
BW_SINGLETON_STORAGE(Renderer)

//--------------------------------------------------------------------------------------------------
IRendererPipeline& rp()
{
    MF_ASSERT(Renderer::pInstance());
    return *Renderer::instance().pipeline();
}

//--------------------------------------------------------------------------------------------------
Renderer::Renderer() : m_type(IRendererPipeline::TYPE_FORWARD_SHADING), m_inited(false)
{

}

//--------------------------------------------------------------------------------------------------
Renderer::~Renderer()
{
    g_pipelineMacroSetting = NULL;
}

//--------------------------------------------------------------------------------------------------
bool Renderer::init(bool isAssetProcessor, bool supportDS)
{
    BW_GUARD;

    g_pipelineMacroSetting->addOption("DEFERRED", "Deferred shading", supportDS, "1");
    g_pipelineMacroSetting->addOption("FORWARD",  "Forward shading", true, "0");
    Moo::GraphicsSetting::add(g_pipelineMacroSetting);

    if (isAssetProcessor)
    {
        return (m_inited = true);
    }

    if (g_pipelineMacroSetting->activeOption() == 0)
    {
        m_type = IRendererPipeline::TYPE_DEFERRED_SHADING;
        m_pipeline.reset(new DeferredPipeline());
    }
    else
    {
        m_type = IRendererPipeline::TYPE_FORWARD_SHADING;
        m_pipeline.reset(new ForwardPipeline());
    }

    //-- init common graphics settings.
    initGraphicsSettings();

    if (!(m_inited = m_pipeline->init()))
    {
        return false;
    }

    //-- ToDo (b_sviglo): Reconsider passing intial options into pipeline init method to give opportunity
    //--                  of different sub-systems to be initiliazed with desired graphics option.
    //--                  It may eliminate redudant resource allocation/deallocation in case if initial
    //--                  graphics option doesn't require some graphics resources to be initialized.
    for (uint i = 0; i < IRendererPipeline::GRAPHICS_SETTING_COUNT; ++i)
    {
        m_pipeline->onGraphicsOptionSelected((IRendererPipeline::EGraphicsSetting)i, m_options[i]);
    }

    return m_inited;
}

//--------------------------------------------------------------------------------------------------
void Renderer::tick(float dt)
{
    BW_GUARD_PROFILER( BW_Renderer_tick );
    MF_ASSERT(m_inited);

    m_pipeline->tick(dt);
}

//--------------------------------------------------------------------------------------------------
IRendererPipeline::EType Renderer::pipelineType() const
{
    BW_GUARD;
    MF_ASSERT(m_inited);

    return m_type;
}

//--------------------------------------------------------------------------------------------------
IRendererPipeline* Renderer::pipeline() const
{
    BW_GUARD;
    MF_ASSERT(m_inited);

    return m_pipeline.get();
}

//--------------------------------------------------------------------------------------------------
void Renderer::initGraphicsSettings()
{
    BW_GUARD;

    //-- disable some setting based on the available on chip video memory.
    bool supported =  true;

    //-- shadows quality.
    {
        Moo::GraphicsSetting::GraphicsSettingPtr setting = Moo::makeCallbackGraphicsSetting(
            "SHADOWS_QUALITY", "Shadows quality", *this, 
            &Renderer::onGraphicsOptionChanged<IRendererPipeline::GRAPHICS_SETTING_SHADOWS_QUALITY>,
            -1, false, false
            );

        m_options[IRendererPipeline::GRAPHICS_SETTING_SHADOWS_QUALITY] = -1;
        m_settings[IRendererPipeline::GRAPHICS_SETTING_SHADOWS_QUALITY] = setting;

        setting->addOption("MAX",       "Max",      supported, true);
        setting->addOption("HIGH",      "High",     supported, true);
        setting->addOption("MEDIUM",    "Medium",   supported, true);
        setting->addOption("LOW",       "Low",      supported, true);
        setting->addOption("OFF",       "Off",      true,      false);

        Moo::GraphicsSetting::add(setting);
    }

    //-- decals quality.
    {
        Moo::GraphicsSetting::GraphicsSettingPtr setting = Moo::makeCallbackGraphicsSetting(
            "DECALS_QUALITY", "Decals quality", *this, 
            &Renderer::onGraphicsOptionChanged<IRendererPipeline::GRAPHICS_SETTING_DECALS_QUALITY>,
            -1, false, false
            );

        m_options[IRendererPipeline::GRAPHICS_SETTING_DECALS_QUALITY] = -1;
        m_settings[IRendererPipeline::GRAPHICS_SETTING_DECALS_QUALITY] = setting;

        setting->addOption("MAX",       "Max",      true, true);
        setting->addOption("HIGH",      "High",     true, true);
        setting->addOption("MEDIUM",    "Medium",   true, true);
        setting->addOption("LOW",       "Low",      true, true);
        setting->addOption("OFF",       "Off",      true, false);

        Moo::GraphicsSetting::add(setting);
    }

    //-- lighting quality.
    {
        Moo::GraphicsSetting::GraphicsSettingPtr setting = Moo::makeCallbackGraphicsSetting(
            "LIGHTING_QUALITY", "Lighting quality", *this, 
            &Renderer::onGraphicsOptionChanged<IRendererPipeline::GRAPHICS_SETTING_LIGHTING_QUALITY>,
            -1, false, false
            );

        m_options[IRendererPipeline::GRAPHICS_SETTING_LIGHTING_QUALITY] = -1;
        m_settings[IRendererPipeline::GRAPHICS_SETTING_LIGHTING_QUALITY] = setting;

        setting->addOption("MAX",       "Max",      true, true);
        setting->addOption("HIGH",      "High",     true, true);
        setting->addOption("MEDIUM",    "Medium",   true, true);
        setting->addOption("LOW",       "Low",      true, true);
        setting->addOption("OFF",       "Off",      true, false);

        Moo::GraphicsSetting::add(setting);
    }
}

//--------------------------------------------------------------------------------------------------
void IRendererPipeline::resetStencil(uint8 stencilMask/* = 0xFF*/) const
{
    BW_GUARD;

    Moo::rc().setRenderState(D3DRS_STENCILENABLE, FALSE);
    Moo::rc().setRenderState(D3DRS_STENCILMASK, stencilMask);
    Moo::rc().setRenderState(D3DRS_STENCILWRITEMASK, stencilMask);
    Moo::rc().setRenderState(D3DRS_STENCILREF, 0);
    Moo::rc().setRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
    Moo::rc().setRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
    Moo::rc().setRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
    Moo::rc().setRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
}

//--------------------------------------------------------------------------------------------------
void IRendererPipeline::setupSystemStencil(IRendererPipeline::EStencilUsage usage) const
{
    BW_GUARD;

    Moo::rc().pushRenderState(D3DRS_STENCILENABLE);
    Moo::rc().pushRenderState(D3DRS_STENCILFUNC);
    Moo::rc().pushRenderState(D3DRS_STENCILREF);
    Moo::rc().pushRenderState(D3DRS_STENCILMASK);
    Moo::rc().pushRenderState(D3DRS_STENCILWRITEMASK);

    Moo::rc().pushRenderState(D3DRS_STENCILPASS);
    Moo::rc().pushRenderState(D3DRS_STENCILFAIL);
    Moo::rc().pushRenderState(D3DRS_STENCILZFAIL);

    Moo::rc().setRenderState(D3DRS_STENCILENABLE, TRUE);
    Moo::rc().setRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
    Moo::rc().setRenderState(D3DRS_STENCILMASK, systemStencilWriteMask());
    Moo::rc().setRenderState(D3DRS_STENCILWRITEMASK, systemStencilWriteMask());
    Moo::rc().setRenderState(D3DRS_STENCILREF, usage);

    Moo::rc().setRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
    Moo::rc().setRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
    Moo::rc().setRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
}

//--------------------------------------------------------------------------------------------------
void IRendererPipeline::restoreSystemStencil() const
{
    BW_GUARD;

    Moo::rc().popRenderState();
    Moo::rc().popRenderState();
    Moo::rc().popRenderState();
    Moo::rc().popRenderState();
    Moo::rc().popRenderState();

    Moo::rc().popRenderState();
    Moo::rc().popRenderState();
    Moo::rc().popRenderState();
}

BW_END_NAMESPACE
