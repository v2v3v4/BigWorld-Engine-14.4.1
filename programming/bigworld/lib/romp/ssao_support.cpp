#include "pch.hpp"
#include "ssao_support.hpp"
#include "moo/render_target.hpp"
#include "moo/effect_material.hpp"
#include "moo/moo_utilities.hpp"
#include "moo/fullscreen_quad.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/renderer.hpp"
#include "romp/ssao_settings.hpp"
#include "moo/render_event.hpp"

BW_BEGIN_NAMESPACE

//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
    bool g_bluring = true;
    bool g_useNormal = true;
    bool g_useStencilOptimization = true;

    int g_ssaoBufferDownsampleFactor = 0;
    int g_depthBufferDownsampleFactor = 0;

    int g_sampleNum = 2; // from 1 to 8
    bool g_showSSAOBuffer = false;

    SSAOSettings g_settings;

    //-- SSAO params shared auto-constant.
    //----------------------------------------------------------------------------------------------
    class SSAOParamsConstant : public Moo::EffectConstantValue
    {
    public:
        SSAOParamsConstant(const SSAOSupport& ssao) : m_ssao(ssao)
        {
            m_params.setZero();
        }

        bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
        {
            BW_GUARD;

            if (m_ssao.enable())
            {
                const SSAOSettings::InfluenceFactors& cfg = m_ssao.settings().m_influences;

                m_params.row(0, Vector4(cfg.m_speedtree.x, cfg.m_speedtree.y, 0, 0));
                m_params.row(1, Vector4(cfg.m_terrain.x, cfg.m_terrain.y, 0, 0));
                m_params.row(2, Vector4(cfg.m_objects.x, cfg.m_objects.y, 0, 0));
            }

            pEffect->SetMatrix(constantHandle, &m_params);
            return true;
        }

    private:
        const SSAOSupport&  m_ssao;
        Matrix              m_params;
    };
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.

SSAOSupport::SSAOSupport()
    : m_enabled(true)
    , m_noiseMap(NULL)
    , m_rt(NULL)
    , m_rtBlure(NULL)
    , m_mat(NULL)
    , m_matBlure(NULL)
    , m_matDepth(NULL)
{
    BW_GUARD;

    MF_WATCH("Render/SSAO/enabled", g_settings.m_enabled, Watcher::WT_READ_WRITE,
        "Enable/disable SSAO.");

    MF_WATCH("Render/SSAO/dev/bluring", g_bluring, Watcher::WT_READ_WRITE, 
        "Enable/disable bluring of SSAO map.");

    MF_WATCH("Render/SSAO/dev/useNormal", g_useNormal, Watcher::WT_READ_WRITE, 
        "Enable/disable using normals during SSAO calculation.");

    MF_WATCH("Render/SSAO/dev/useStencilOptimization", g_useStencilOptimization, 
        Watcher::WT_READ_WRITE, "Enable/disable stencil optimization during SSAO calculation.");

    MF_WATCH("Render/SSAO/dev/samplesNum", g_sampleNum, Watcher::WT_READ_WRITE, 
        "Multiplier for number of samples. Valid value range is from 1 to 8. "
        "Real number of samples is 4 * samplesNum.");

    MF_WATCH("Render/SSAO/radius", g_settings.m_radius, Watcher::WT_READ_WRITE, 
        "Sampling radius. In screen space.");

    MF_WATCH("Render/SSAO/amplify", g_settings.m_amplify, Watcher::WT_READ_WRITE, 
        "Amplify resultign SSAO value. "
        "If from 0 to 1 -- SSAO becomes darker. "
        "If above 1 -- becomes lighter. "
        "If 1 -- SSAO value isn't affected. ");

    MF_WATCH("Render/SSAO/fade", g_settings.m_fade, Watcher::WT_READ_WRITE, 
        "Fade SSAO value if the distance between sample point and occludder gets hight."
        "Measured in g_radius in linear depth space.");

    //--
    MF_WATCH("Render/SSAO/influence factors/speedtree/ambient",
        g_settings.m_influences.m_speedtree.x, Watcher::WT_READ_WRITE, "0.0 - 1.0."
        );

    MF_WATCH("Render/SSAO/influence factors/speedtree/lighting",
        g_settings.m_influences.m_speedtree.y, Watcher::WT_READ_WRITE, "0.0 - 1.0."
        );

    //--
    MF_WATCH("Render/SSAO/influence factors/terrain/ambient",
        g_settings.m_influences.m_terrain.x, Watcher::WT_READ_WRITE, "0.0 - 1.0."
        );

    MF_WATCH("Render/SSAO/influence factors/terrain/lighting",
        g_settings.m_influences.m_terrain.y, Watcher::WT_READ_WRITE, "0.0 - 1.0."
        );

    MF_WATCH("Render/SSAO/influence factors/objects/ambient",
        g_settings.m_influences.m_objects.x, Watcher::WT_READ_WRITE, "0.0 - 1.0."
        );

    MF_WATCH("Render/SSAO/influence factors/objects/lighting",
        g_settings.m_influences.m_objects.y, Watcher::WT_READ_WRITE, "0.0 - 1.0."
        );

    MF_WATCH("Render/SSAO/dev/bufferSsaoDownsampleFactor", *this,  
        &SSAOSupport::getSsaoBufferDownsampleFactor, 
        &SSAOSupport::setSsaoBufferDownsampleFactor);

    MF_WATCH("Render/SSAO/dev/bufferDepthDownsampleFactor", *this,
        &SSAOSupport::getDepthBufferDownsampleFactor, 
        &SSAOSupport::setDepthBufferDownsampleFactor);

    MF_WATCH("Render/SSAO/dev/showBuffer", g_showSSAOBuffer, Watcher::WT_READ_WRITE, 
        "Show SSAO buffer.");
}

//--------------------------------------------------------------------------------------------------

SSAOSupport::~SSAOSupport()
{
    BW_GUARD;

#if ENABLE_WATCHERS
    if (Watcher::hasRootWatcher())
    {
        Watcher::rootWatcher().removeChild("Render/SSAO");
    }
#endif //-- ENABLE_WATCHERS
}

//--------------------------------------------------------------------------------------------------

void SSAOSupport::settings(const SSAOSettings* settings)
{
    BW_GUARD;

    //-- set current settings.
    g_settings = settings ? *settings : SSAOSettings();
}

//--------------------------------------------------------------------------------------------------

const SSAOSettings& SSAOSupport::settings() const
{
    return g_settings;
}


//----------------------------------------------------------------------------------------------

bool SSAOSupport::enable() const
{
    BW_GUARD;

    return g_settings.m_enabled && m_enabled;
}

//--------------------------------------------------------------------------------------------------
void SSAOSupport::setQualityOption(int optionIndex)
{
    BW_GUARD;
    MF_ASSERT(optionIndex >= 0 && optionIndex <= 4);

    //--
    struct QualityOption
    {
        uint m_samples;
        bool m_useNormal;
        bool m_useBluring;
        bool m_enabled;
    };

    //-- should be in sync with graphic setting LIGHTING_QUALITY
    const QualityOption options[] =
    {
        {4, true,  true,  true},    //-- MAX
        {3, true,  true,  true},    //-- HIGH
        {2, true,  true,  true},    //-- MEDIUM
        {1, true,  true,  true},    //-- LOW
        {1, false, false, false}    //-- OFF
    };

    //--
    const QualityOption& option = options[optionIndex];

    m_enabled   = option.m_enabled;
    g_sampleNum = option.m_samples;
    g_useNormal = option.m_useNormal;
    g_bluring   = option.m_useBluring;
}

bool SSAOSupport::showBuffer() const
{
    BW_GUARD;

    return g_showSSAOBuffer;
}

void SSAOSupport::showBuffer( bool show )
{
    g_showSSAOBuffer = show;
}

void SSAOSupport::enable(bool flag)
{
    BW_GUARD;
    m_enabled = flag;
}

void SSAOSupport::resolve()
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("SSAO");
    BW_SCOPED_RENDER_PERF_MARKER("SSAO")

    if (!enable())
        return;

    IRendererPipeline* rp = Renderer::instance().pipeline();

    //-- downsample depth

    if(g_depthBufferDownsampleFactor > 0 && m_rtDownsampledDepth->push())
    {
        Moo::rc().fsQuad().draw(*m_matDepth.get());
        m_rtDownsampledDepth->pop();
    }

    //-- resolve

    if(m_rt->push())
    {
        //-- clear buffer only in editor or when show buffer is on in client for debug purposes.
        bool clearBuffer = g_showSSAOBuffer;
#ifdef EDITOR_ENABLED
        clearBuffer = true;
#endif
        if (clearBuffer)
        {
            Moo::rc().device()->Clear(0, NULL, D3DCLEAR_TARGET, Moo::Colour(1,1,1,1), 1, 0);
        }

        ID3DXEffect* effect = m_mat->pEffect()->pEffect();
        IRendererPipeline* rp = Renderer::instance().pipeline();
        if(effect)
        {
            if(g_depthBufferDownsampleFactor > 0)
            {
                effect->SetTexture("g_depth", m_rtDownsampledDepth->pTexture());
            }
            effect->SetBool("g_useDownsampledDepth", g_depthBufferDownsampleFactor > 0 ? 1 : 0);

            effect->SetTexture("g_noiseMapSSAO", m_noiseMap->pTexture());
            effect->SetInt("g_sampleNum", g_sampleNum);
            effect->SetInt("g_useNormal", g_useNormal);
            effect->SetBool("g_useStencilOptimization", g_useStencilOptimization);
            effect->SetFloat("g_radius", g_settings.m_radius);
            effect->SetFloat("g_amplify", g_settings.m_amplify);
            effect->SetFloat("g_fade", g_settings.m_fade);
            effect->CommitChanges();
        }
        Moo::rc().fsQuad().draw(*m_mat.get());

        m_rt->pop();
    }

    //-- blure

    if(g_bluring)
    {
        if(m_rtBlure->push())
        {
            //-- clear buffer only in editor for debug purposes.
#ifdef EDITOR_ENABLED
            Moo::rc().device()->Clear(0, NULL, D3DCLEAR_TARGET, Moo::Colour(1,1,1,1), 1, 0);
#endif

            ID3DXEffect* effect = m_matBlure->pEffect()->pEffect();

            m_matBlure->hTechnique("VERTICAL_BLURE");
            effect->SetTexture("g_srcMap", m_rt->pTexture());
            effect->SetBool("g_useStencilOptimization", g_useStencilOptimization);
            effect->CommitChanges();

            Moo::rc().fsQuad().draw(*m_matBlure.get());
            m_rtBlure->pop();
        }

        if(m_rt->push())
        {
            //-- clear buffer only in editor for debug purposes.
#ifdef EDITOR_ENABLED
            Moo::rc().device()->Clear(0, NULL, D3DCLEAR_TARGET, Moo::Colour(1,1,1,1), 1, 0);
#endif

            ID3DXEffect* effect = m_matBlure->pEffect()->pEffect();

            m_matBlure->hTechnique("HORIZONTAL_BLURE");
            effect->SetTexture("g_srcMap", m_rtBlure->pTexture());
            effect->SetBool("g_useStencilOptimization", g_useStencilOptimization);
            effect->CommitChanges();

            Moo::rc().fsQuad().draw(*m_matBlure.get());
            m_rt->pop();
        }
    }
}

DX::BaseTexture* SSAOSupport::screenSpaceAmbienOcclusionMap()
{
    BW_GUARD;

    return m_rt->pTexture();
}

//--------------------------------------------------------------------------------------------------

bool SSAOSupport::recreateForD3DExDevice() const
{
    return true;
}

//----------------------------------------------------------------------------------------------

void SSAOSupport::deleteUnmanagedObjects()
{
    BW_GUARD;

    m_rt = NULL;
    m_rtBlure = NULL;
    m_rtDownsampledDepth = NULL;
}

//----------------------------------------------------------------------------------------------

void SSAOSupport::createUnmanagedObjects()
{
    BW_GUARD;

    bool success = true;

    // TODO: do not create texture if not needed

    uint s = -g_ssaoBufferDownsampleFactor;
    uint d = -g_depthBufferDownsampleFactor;

    success &= createRenderTarget(m_rt, s, s, D3DFMT_R32F, "texture/SSAO map", true, true);

    //-- TODO: ...
    //if(g_bluring)
    //{
        success &= createRenderTarget(m_rtBlure, s, s, D3DFMT_R32F, "texture/SSAO blure", true, true);
    //}

    if(g_depthBufferDownsampleFactor > 0)
    {
        success &= createRenderTarget(m_rtDownsampledDepth, d, d, D3DFMT_R32F, "texture/SSAO depth", true, true);
    }

    m_noiseMap = Moo::TextureManager::instance()->get("system/maps/noise4x4.dds");

    success &= (m_noiseMap != NULL);

    MF_ASSERT(success && "Not all system resources have been loaded correctly.");
}

//----------------------------------------------------------------------------------------------
void SSAOSupport::createManagedObjects()
{
    BW_GUARD;

    //-- register shared auto-constant.
    Moo::rc().effectVisualContext().registerAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "SSAOParams",
        new SSAOParamsConstant(*this)
        );

    bool success = true;
    success &= createEffect(m_mat, "shaders/ssao/ssao.fx");
    success &= createEffect(m_matBlure, "shaders/ssao/blure4x4.fx");
    success &= createEffect(m_matDepth, "shaders/ssao/depth.fx");
    MF_ASSERT(success && "Not all system resources have been loaded correctly.");
}

//----------------------------------------------------------------------------------------------
void SSAOSupport::deleteManagedObjects()
{
    BW_GUARD;

    //-- unregister shared auto-constant.
    Moo::rc().effectVisualContext().unregisterAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "SSAOParams"
        );

    m_noiseMap = NULL;
    m_mat = NULL;
    m_matBlure = NULL;
    m_matDepth = NULL;
}

//----------------------------------------------------------------------------------------------

void SSAOSupport::setSsaoBufferDownsampleFactor(int value)
{
    g_ssaoBufferDownsampleFactor=value;

    deleteUnmanagedObjects();
    createUnmanagedObjects();
}

int SSAOSupport::getSsaoBufferDownsampleFactor() const 
{
    return g_ssaoBufferDownsampleFactor;
}

//----------------------------------------------------------------------------------------------

void SSAOSupport::setDepthBufferDownsampleFactor(int value)
{
    g_depthBufferDownsampleFactor=value;

    deleteUnmanagedObjects();
    createUnmanagedObjects();
}

int SSAOSupport::getDepthBufferDownsampleFactor() const
{
    return g_depthBufferDownsampleFactor;
}

//----------------------------------------------------------------------------------------------

BW_END_NAMESPACE
