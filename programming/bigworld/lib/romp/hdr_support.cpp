#include "pch.hpp"
#include "hdr_support.hpp"
#include "hdr_settings.hpp"
#include "moo/fullscreen_quad.hpp"
#include "moo/moo_utilities.hpp"
#include "moo/render_target.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/render_event.hpp"

BW_BEGIN_NAMESPACE

//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
    //-- watchers.
    HDRSettings g_cfg;
    float       g_avgLum = 0.0f;

    //-- the minimal size of the bloom calculation chain. Affects overall blooming quality. Lower
    //-- values make bloom more smooth.
    const float g_minBloomTargetSize = 25.0f;

    //-- Warning: this feature used only for debugging because it kills asynchronous behavior between
    //--          CPU and GPU.
    bool        g_readbackAvgLumToCPU = false;

    //-- ToDo (b_sviglo): Ugly solution for 8.0 version reconsider later.
    bool        g_forceDisableBloom = false;

    //-- Makes string from desired format and parameters.
    //----------------------------------------------------------------------------------------------
    BW::string makeStr(const char* format, ...)
    {
        BW_GUARD;

        va_list args;
        char buffer[256];
        va_start(args, format);
        _vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
        va_end(args);

        return buffer;
    }

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

    //-- HDR params shared auto-constant.
    //--------------------------------------------------------------------------------------------------
    class HDRParamsConstant : public Moo::EffectConstantValue
    {
    public:
        HDRParamsConstant(const HDRSupport& hdr) : m_hdr(hdr)
        {

        }

        bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
        {
            BW_GUARD;
            
            Vector4 params(0.0f, 1.0f, 1.0f, 1.0f);

            if (m_hdr.enable())
            {
                const HDRSettings::Environment& cfg = m_hdr.settings().m_environment;

                params.set(
                    cfg.m_skyLumMultiplier,
                    cfg.m_sunLumMultiplier,
                    cfg.m_ambientLumMultiplier,
                    cfg.m_fogLumMultiplier
                    );
            }
            pEffect->SetVector(constantHandle, &params);
            return true;
        }

    private:
        const HDRSupport& m_hdr;
    };


    //-- Gamma correction.
    //--------------------------------------------------------------------------------------------------
    class GammaCorrectionConstant : public Moo::EffectConstantValue
    {
    public:
        GammaCorrectionConstant(const HDRSupport& hdr) : m_hdr(hdr)
        {

        }

        bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
        {
            BW_GUARD;

            Vector4 params(1.0f, 1.0f, 0.0f, 0.0);

            if (m_hdr.enable() && m_hdr.settings().m_gammaCorrection.m_enabled)
            {
                const HDRSettings::GammaCorrection& cfg = m_hdr.settings().m_gammaCorrection;

                params.set(cfg.m_gamma, 1.0f / cfg.m_gamma, 0.0f, 0.0f);
            }

            pEffect->SetVector(constantHandle, &params);
            return true;
        }

    private:
        const HDRSupport& m_hdr;
    };
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.


//--------------------------------------------------------------------------------------------------
HDRSupport::HDRSupport()
    :   m_successed(false),
        m_valid(false),
        m_curTime(0.0f),
        m_update(true)
{
    MF_WATCH("Render/HDR/average luminance",
        g_avgLum, Watcher::WT_READ_ONLY, "Average luminance value."
        );

    MF_WATCH("Render/HDR/enable",
        g_cfg.m_enabled, Watcher::WT_READ_WRITE, "enable/disable HDR rendering."
        );

    MF_WATCH("Render/HDR/readback avg lum",
        g_readbackAvgLumToCPU, Watcher::WT_READ_WRITE, "enable/disable readback of avg luminance from GPU (slow)."
        );

    MF_WATCH("Render/HDR/average luminance update interval",
        g_cfg.m_avgLumUpdateInterval, Watcher::WT_READ_WRITE, "How often average luminance will be calculated."
        );

    MF_WATCH("Render/HDR/use the log of luminance",
        g_cfg.m_useLogAvgLuminance, Watcher::WT_READ_WRITE, "Use log for average luminance."
        );

    MF_WATCH("Render/HDR/accommodation speed",
        g_cfg.m_adaptationSpeed, Watcher::WT_READ_WRITE, "Accommodation speed 10 - 1000."
        );

    MF_WATCH("Render/HDR/tone mapping/middle gray",
        g_cfg.m_tonemapping.m_middleGray, Watcher::WT_READ_WRITE, "Middle gray value to which overall HDR luminance aims."
        );

    MF_WATCH("Render/HDR/tone mapping/white point",
        g_cfg.m_tonemapping.m_whitePoint, Watcher::WT_READ_WRITE, "White point luminance value."
        );

    MF_WATCH("Render/HDR/tone mapping/eye dark limit",
        g_cfg.m_tonemapping.m_eyeDarkLimit, Watcher::WT_READ_WRITE, "Minimal pixel luminance value before it will be clamped."
        );

    MF_WATCH("Render/HDR/tone mapping/eye light limit",
        g_cfg.m_tonemapping.m_eyeLightLimit, Watcher::WT_READ_WRITE, "Maximal pixel luminance value after it will be clamped."
        );

    MF_WATCH("Render/HDR/environment/ambient luminance multiplier",
        g_cfg.m_environment.m_ambientLumMultiplier, Watcher::WT_READ_WRITE, "0.0 - 20.0f"
        );

    MF_WATCH("Render/HDR/environment/sky luminance multiplier",
        g_cfg.m_environment.m_skyLumMultiplier, Watcher::WT_READ_WRITE, "0.0 - 20.0f"
        );

    MF_WATCH("Render/HDR/environment/sun luminance multiplier",
        g_cfg.m_environment.m_sunLumMultiplier, Watcher::WT_READ_WRITE, "0.0 - 20.0f"
        );

    MF_WATCH("Render/HDR/environment/fog luminance multiplier",
        g_cfg.m_environment.m_fogLumMultiplier, Watcher::WT_READ_WRITE, "0.0 - 20.0f"
        );

    MF_WATCH("Render/HDR/bloom/enabled",
        g_cfg.m_bloom.m_enabled, Watcher::WT_READ_WRITE, "Enable/disable bloom."
        );

    MF_WATCH("Render/HDR/bloom/factor",
        g_cfg.m_bloom.m_factor, Watcher::WT_READ_WRITE, "Bloom factor 0.0 - 5.0."
        );

    MF_WATCH("Render/HDR/bloom/bright threshold",
        g_cfg.m_bloom.m_brightThreshold, Watcher::WT_READ_WRITE, "0.0 - 5.0."
        );

    MF_WATCH("Render/HDR/bloom/bright offset",
        g_cfg.m_bloom.m_brightOffset, Watcher::WT_READ_WRITE, "0.0 - 5.0."
        );

    MF_WATCH("Render/HDR/linear space lighting/enable",
        g_cfg.m_gammaCorrection.m_enabled, Watcher::WT_READ_WRITE, "Use linear space lighting calculation."
        );

    MF_WATCH("Render/HDR/linear space lighting/gamma",
        g_cfg.m_gammaCorrection.m_gamma, Watcher::WT_READ_WRITE, "Gamma."
        );
}

//--------------------------------------------------------------------------------------------------
HDRSupport::~HDRSupport()
{
#if ENABLE_WATCHERS
    if (Watcher::hasRootWatcher())
    {
        Watcher::rootWatcher().removeChild("Render/HDR");
    }
#endif //-- ENABLE_WATCHERS
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::settings(const HDRSettings* cfg)
{
    BW_GUARD;

    //-- set current settings.
    g_cfg = cfg ? *cfg : HDRSettings();
}

//--------------------------------------------------------------------------------------------------
const HDRSettings& HDRSupport::settings() const
{
    return g_cfg;
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::setQualityOption(int /*option*/)
{
    BW_GUARD;

    //-- ToDo (b_sviglo): reconsider.
}

//--------------------------------------------------------------------------------------------------
bool HDRSupport::enable() const
{
    BW_GUARD;
    return g_cfg.m_enabled;
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::enable(bool flag)
{
    BW_GUARD;
    g_cfg.m_enabled = flag;
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::tick(float dt)
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("HDR tick")

    //-- increment time.
    m_curTime += dt;

    //-- adjust current time.
    if (m_curTime >= g_cfg.m_avgLumUpdateInterval)
    {
        m_update = true;
        m_curTime -= g_cfg.m_avgLumUpdateInterval;
    }

    //-- ToDo (b_sviglo): Ugly solution for 8.0 version reconsider later.
    {
        const Moo::GraphicsSetting::GraphicsSettingPtr& ppCfg = Moo::GraphicsSetting::getFromLabel("POST_PROCESSING_QUALITY");
        g_forceDisableBloom = (ppCfg && (ppCfg->activeOption() > 3));
    }
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::intercept()
{
    BW_GUARD;

    if (!enable() || !m_valid) return;

    //-- 1. set all textures to NULL, just in case the render target was set on any of the stages before.
    uint32 nTextures = Moo::rc().currentDeviceInfo().caps_.MaxSimultaneousTextures;
    for (uint32 i = 0; i < nTextures; ++i)
    {
        Moo::rc().setTexture(i, NULL);
    }
    
    //-- 2. try to make HDR render target as current.
    m_successed = m_hdrRT->push();

    //-- 3. clear color buffer.
    Moo::rc().device()->Clear(0, NULL, D3DCLEAR_TARGET, Moo::Colour(0,0,0,0), 1, 0);
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::resolve()
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("HDR");
    BW_SCOPED_RENDER_PERF_MARKER("HDR");

    if (!enable() || !m_valid) return;

    //-- copy HDR render target into the main back buffer.
    if (m_successed)
    {
        //-- set main back buffer render target as current.
        m_hdrRT->pop();

        //-- update average luminance.
        if (m_update)
        {
            calcAvgLum();
            m_update = false;
        }

        //-- calculate current average luminance on gpu.
        calcAvgLumOnGPU();

        //-- Warning: for debug purposes we can read back average luminance value calculated on the
        //--          GPU but it significantly decreases frame rate due CPU<->GPU synchronization.
        if (g_readbackAvgLumToCPU)
        {
            g_avgLum = readAvgLum();
        }

        //-- now we know average luminance for current frame use it to decide which pixels are
        //-- suited to be added to the bloom map.
        calcBloom();

        //-- and finally do tone-mapping.
        doTonemapping();
    }
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::calcBloom()
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("HDR Bloom");
    BW_SCOPED_RENDER_PERF_MARKER("HDR Bloom");

    if (!m_valid)
        return;

    if (!g_cfg.m_bloom.m_enabled || g_forceDisableBloom)
        return;

    //-- 1. make down-sampled copy of HDR map and apply to it bright pass.
    m_bloomMat->hTechnique("BRIGHT_PASS");
    if (m_bloomMaps[0].first->push())
    {
        const DX::Viewport& vp0 = m_viewPorts[0];
        Vector4 screen(
            static_cast<float>(vp0.Width), static_cast<float>(vp0.Height), 1.0f / vp0.Width, 1.0f / vp0.Height
            );

        DX::Viewport& vp = m_viewPorts[1];
        Moo::rc().setViewport(&vp);

        ID3DXEffect* effect = m_bloomMat->pEffect()->pEffect();
        {
            const Vector4 params(g_cfg.m_bloom.m_brightThreshold, g_cfg.m_bloom.m_brightOffset, g_cfg.m_tonemapping.m_middleGray, 0);
            effect->SetTexture("g_srcMap",      m_hdrRT->pTexture());
            effect->SetTexture("g_avgLumMap",   m_finalAvgLumRT->pTexture());
            effect->SetVector ("g_params",      &params);
            effect->SetVector ("g_srcSize",     &screen);
            effect->CommitChanges();

            Moo::rc().fsQuad().draw(*m_bloomMat.get(), vp.Width, vp.Height);            
        }

        m_bloomMaps[0].first->pop();
    }

    //-- 2. now generate mip-map chain of bloom map.
    m_bloomMat->hTechnique("DOWN_SAMPLE_BLOOM_MAP");
    for (uint i = 1; i < m_bloomMaps.size(); ++i)
    {
        if (m_bloomMaps[i].first->push())
        {
            DX::Viewport& vp = m_viewPorts[i + 1];

            Moo::rc().setViewport(&vp);
            
            ID3DXEffect* effect = m_bloomMat->pEffect()->pEffect();
            {
                effect->SetTexture("g_srcMap", m_bloomMaps[i - 1].first->pTexture());
                m_bloomMat->commitChanges();

                Moo::rc().fsQuad().draw(*m_bloomMat.get(), vp.Width, vp.Height);
            }

            m_bloomMaps[i].first->pop();
        }
    }

    //-- 3. now blur bloom maps start from the end of mip-map chain.
    
    //-- 3.1. process the largest (last) mip level.
    {
        DX::Viewport& vp      = m_viewPorts[m_bloomMaps.size()];
        BloomMapPair& mapPair = m_bloomMaps.back();

        //-- blur mip.
        doGaussianBlur(mapPair.first.get(), mapPair.second.get(), *m_gaussianBlurMat.get(), vp);
    }
    
    //-- 3.2. now process other mips.
    for (int i = static_cast<int>(m_bloomMaps.size()) - 2; i >= 0; --i)
    {
        DX::Viewport& vp = m_viewPorts[i + 1];

        //-- prepare render targets shortcuts.
        Moo::RenderTarget* rt1    = m_bloomMaps[i].first.get();
        Moo::RenderTarget* rt2    = m_bloomMaps[i].second.get();
        Moo::RenderTarget* prevMip = m_bloomMaps[i + 1].second.get();

        //-- correct previous mip texture in boundary case.
        if (i == m_bloomMaps.size() - 2)
        {
            prevMip = m_bloomMaps[i + 1].first.get();
        }

        //-- accumulate color with previous mip.
        m_bloomMat->hTechnique("ACCUMULATE_BLOOM_MAP");
        if (rt2->push())
        {
            Moo::rc().setViewport(&vp);

            ID3DXEffect* effect = m_bloomMat->pEffect()->pEffect();
            {
                effect->SetTexture("g_srcMap", rt1->pTexture());
                effect->SetTexture("g_prevMipMap", prevMip->pTexture());
                m_bloomMat->commitChanges();
                    
                Moo::rc().fsQuad().draw(*m_bloomMat.get(), vp.Width, vp.Height);
            }

            rt2->pop();
        }
        
        //-- blur current mip.
        doGaussianBlur(rt2, rt1, *m_gaussianBlurMat.get(), vp);
    }

    //-- Now the blurred bloom map located m_bloomMaps[0].second.
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::calcAvgLum()
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("calc avg lum")
    MF_ASSERT( m_bloomMaps.size() && m_avgLumRTs.size());

    Moo::RenderTarget* downSampledHDRRT = m_bloomMaps[0].first.get();

    //-- 1. make copy down sampled copy of HDR map.
    if (downSampledHDRRT->push())
    {
        //-- set view port.
        Moo::rc().setViewport(&m_viewPorts[1]);

        //-- setup constants.
        m_averageLuminanceMat->hTechnique("DOWN_SAMPLE_COLOR");
        ID3DXEffect* effect = m_averageLuminanceMat->pEffect()->pEffect();
        {
            effect->SetTexture("g_previousMipMap", m_hdrRT->pTexture());
            m_averageLuminanceMat->commitChanges();

            Moo::rc().fsQuad().draw(
                *m_averageLuminanceMat.get(), m_viewPorts[1].Width, m_viewPorts[1].Height
                );          
        }

        downSampledHDRRT->pop();
    }

    //-- 2. make down sampled average luminance map based on down sampled copy of HDR map.
    m_averageLuminanceMat->hTechnique(g_cfg.m_useLogAvgLuminance ? "LOG_AVERAGE_LUM" : "AVERAGE_LUM");
    if (m_avgLumRTs[0]->push())
    {
        const DX::Viewport& vp = m_viewPorts[1];
        Vector4 screen(
            static_cast<float>(vp.Width), static_cast<float>(vp.Height), 1.0f / vp.Width, 1.0f / vp.Height
            );

        //-- set view port.
        Moo::rc().setViewport(&m_viewPorts[2]);

        //-- setup constants.
        ID3DXEffect* effect = m_averageLuminanceMat->pEffect()->pEffect();
        {
            effect->SetTexture("g_previousMipMap", downSampledHDRRT->pTexture());
            effect->SetVector ("g_previousMipMapSize", &screen);
            m_averageLuminanceMat->commitChanges();

            Moo::rc().fsQuad().draw(
                *m_averageLuminanceMat.get(), m_viewPorts[2].Width, m_viewPorts[2].Height
                );          
        }

        m_avgLumRTs[0]->pop();
    }

    //-- 3. now calculate the average luminance on the screen up to 1x1 mip.
    m_averageLuminanceMat->hTechnique("DOWN_SAMPLE_LUM");
    for (uint i = 1; i < m_avgLumRTs.size(); ++i)
    {
        if (m_avgLumRTs[i]->push())
        {
            //-- set view port.
            Moo::rc().setViewport(&m_viewPorts[i + 2]);

            //-- setup constants.
            ID3DXEffect* effect = m_averageLuminanceMat->pEffect()->pEffect();
            {
                effect->SetTexture("g_previousMipMap", m_avgLumRTs[i - 1]->pTexture());
                m_averageLuminanceMat->commitChanges();

                Moo::rc().fsQuad().draw(
                    *m_averageLuminanceMat.get(), m_viewPorts[i + 2].Width, m_viewPorts[i + 2].Height
                    );          
            }

            m_avgLumRTs[i]->pop();
        }
    }

    //-- swap 1x1 mip with another 1x1 mip for accessing this texture from shader.
    std::swap(m_avgLumRTs.back(), m_avgLumRT);
    std::swap(m_avgLumRTs.back(), m_finalAvgLumRT);
}

//--------------------------------------------------------------------------------------------------
float HDRSupport::readAvgLum()
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("read avg lum from GPU")

    ComObjectWrap<DX::Surface> inmemSurface;
    ComObjectWrap<DX::Surface> hdrSurface;
    D3DLOCKED_RECT  lockRect;
    float           avgLum = 0.0f;

    if (
        SUCCEEDED(m_finalAvgLumRT->pSurface(hdrSurface)) &&
        SUCCEEDED(m_inmemTexture->GetSurfaceLevel(0, &inmemSurface)) &&
        SUCCEEDED(Moo::rc().device()->GetRenderTargetData(hdrSurface.pComObject(), inmemSurface.pComObject())) &&
        SUCCEEDED(inmemSurface->LockRect(&lockRect, NULL, 0))
        )
    {
        avgLum = *reinterpret_cast<const float*>(lockRect.pBits);
        inmemSurface->UnlockRect();
    }

    return avgLum;
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::calcAvgLumOnGPU()
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("calc avg lum on GPU");
    MF_ASSERT( m_avgLumRTs.size() );

    if (m_finalAvgLumRT->push())
    {
        //-- set view port.
        Moo::rc().setViewport(&m_viewPorts.back());

        //-- setup constants.
        m_averageLuminanceMat->hTechnique(g_cfg.m_useLogAvgLuminance ? "FINAL_AVERAGE_LOG_LUM" : "FINAL_AVERAGE_LUM");
        ID3DXEffect* effect = m_averageLuminanceMat->pEffect()->pEffect();
        {
            const Vector4 params(
                g_cfg.m_tonemapping.m_eyeDarkLimit,
                g_cfg.m_tonemapping.m_eyeLightLimit,
                m_curTime, g_cfg.m_adaptationSpeed
                );
            effect->SetTexture("g_avgLumMap1", m_avgLumRTs.back()->pTexture());
            effect->SetTexture("g_avgLumMap2", m_avgLumRT->pTexture());
            effect->SetVector ("g_params", &params);
            effect->CommitChanges();
        }

        Moo::rc().fsQuad().draw(*m_averageLuminanceMat.get(), 1, 1);            

        m_finalAvgLumRT->pop();
    }
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::doTonemapping()
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("tone mapping");
    MF_ASSERT( m_bloomMaps.size() );

    //-- setup constants.
    ID3DXEffect* effect = m_toneMappingMat->pEffect()->pEffect();
    {
        const Vector4 bloomParams(g_cfg.m_bloom.m_enabled && !g_forceDisableBloom, g_cfg.m_bloom.m_factor, 0.0f, 0.0f);
        const Vector4 params(g_cfg.m_tonemapping.m_middleGray, g_cfg.m_tonemapping.m_whitePoint, 0, 0);
        effect->SetTexture("g_avgLumMap",       m_finalAvgLumRT->pTexture());
        effect->SetTexture("g_HDRMap",          m_hdrRT->pTexture());
        effect->SetTexture("g_bloomMap",        m_bloomMaps[0].second->pTexture());
        effect->SetVector ("g_bloomParams",     &bloomParams);
        effect->SetVector ("g_params",          &params);
        effect->CommitChanges();

        //-- draw full screen quad into the main back-buffer and resolve HDR buffer.
        Moo::rc().fsQuad().draw(*m_toneMappingMat.get());
    }
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::createUnmanagedObjects()
{
    BW_GUARD;

    //-- 2. retrieve the current back buffer description.
    const D3DSURFACE_DESC& bbDesc = Moo::rc().backBufferDesc();

    //-- 3. retrieve new screen resolution.
    uint width  = static_cast<uint>(bbDesc.Width);
    uint height = static_cast<uint>(bbDesc.Height);

    // Smallest resolution that we are allowing to be used
    const uint32 smallestRes = 16;
    if (width < smallestRes || height < smallestRes)
        return;
    
    //-- 4. recreate HDR render target.
    bool success = createRenderTarget(
        m_hdrRT, width, height, D3DFMT_A16B16G16R16F, "texture/HDR map", true, true
        );

    //-- calculate mip-map chain.
    typedef std::pair<uint, uint> MipSize;
    typedef BW::vector<MipSize>  MipArray;

    //-- add two fist viewports.
    DX::Viewport viewPort;
    Moo::rc().getViewport(&viewPort);
    m_viewPorts.push_back(viewPort);

    MipArray mipArray;

    uint mipWidth  = width / 2;
    uint mipHeight = height / 2;

    while (mipWidth || mipHeight)
    {
        //-- make sure that width and height great or equal 1.
        mipWidth  = max(mipWidth,  uint(1));
        mipHeight = max(mipHeight, uint(1));
        
        //-- add mip.
        mipArray.push_back(MipSize(mipWidth, mipHeight));

        //-- add view port.
        viewPort.Width  = mipWidth;
        viewPort.Height = mipHeight;
        m_viewPorts.push_back(viewPort);

        //-- calculate size of the next mip-map level.
        mipWidth  >>= 1;
        mipHeight >>= 1;
    }

    //-- find count of blur maps.
    uint bloomMapsCount = 0;
    for (uint i = 0; i < mipArray.size(); ++i, ++bloomMapsCount)
    {
        if (mipArray[i].first < g_minBloomTargetSize && mipArray[i].first < g_minBloomTargetSize)
            break;
    }

    // possible that the buffer is too small to create any bloom maps.
    if (!bloomMapsCount) return;

    //-- resize blur maps.
    m_bloomMaps.resize(bloomMapsCount);
    for (uint i = 0; i < bloomMapsCount; ++i)
    {
        const MipSize& size    = mipArray[i];
        BW::string    rtName1 = makeStr("texture/HDR bloom map #%d_1", i);
        BW::string    rtName2 = makeStr("texture/HDR bloom map #%d_2", i);

        success &= createRenderTarget(
            m_bloomMaps[i].first, size.first, size.second, D3DFMT_A16B16G16R16F, rtName1.c_str(), true, true
            );

        success &= createRenderTarget(
            m_bloomMaps[i].second, size.first, size.second, D3DFMT_A16B16G16R16F, rtName2.c_str(), true, true
            );
    }

    MF_ASSERT( mipArray.size() > 1 );

    //-- create render targets mip-map chain for finding average luminance.
    m_avgLumRTs.resize(mipArray.size() - 1);
    for (uint i = 1; i < mipArray.size(); ++i)
    {
        const MipSize& size   = mipArray[i];
        BW::string    rtName = makeStr("texture/HDR average luminance #%d", i);

        success &= createRenderTarget(
            m_avgLumRTs[i - 1], size.first, size.second, D3DFMT_R32F,   rtName.c_str(), true, true
            );
    }

    //-- create second average luminance rt.
    success &= createRenderTarget(
        m_avgLumRT, 1, 1, D3DFMT_R32F, "texture/HDR average luminance RT", true, true
        );

    success &= createRenderTarget(
        m_finalAvgLumRT, 1, 1, D3DFMT_R32F, "texture/HDR final average luminance RT", true, true
        );

    m_inmemTexture = Moo::rc().createTexture(
        1, 1, 1, 0, D3DFMT_R32F, D3DPOOL_SYSTEMMEM, "texture/HDR average value"
        );
    success &= m_inmemTexture.hasComObject();

    MF_ASSERT(success && "Not all system resources have been loaded correctly.");
    m_valid = success;
}

//--------------------------------------------------------------------------------------------------
bool HDRSupport::recreateForD3DExDevice() const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::deleteUnmanagedObjects()
{
    BW_GUARD;

    //-- reset smart-pointer.
    m_hdrRT = NULL;
    m_avgLumRT = NULL;
    m_finalAvgLumRT = NULL;
    m_bloomMaps.clear();
    m_avgLumRTs.clear();
    m_viewPorts.clear();

    //-- reset parameters.
    // Unmanaged objects like render targets and textures are
    // based on the size of the render window. This render window 
    // might not be a valid size so need to handle this case.
    m_valid = false;
    m_update  = true;
    m_curTime = 0.0f;
    m_avgLumValues.set(0,0);
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::createManagedObjects()
{
    BW_GUARD;

    //-- register shared auto-constant.
    Moo::rc().effectVisualContext().registerAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "HDRParams",
        new HDRParamsConstant(*this)
        );

    //-- register shared auto-constant.
    Moo::rc().effectVisualContext().registerAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "GammaCorrection",
        new GammaCorrectionConstant(*this)
        );

    bool success = true;

    success &= createEffect(m_toneMappingMat, "shaders/hdr/hdr_tonemapping.fx");
    success &= createEffect(m_averageLuminanceMat, "shaders/hdr/average_luminance.fx");
    success &= createEffect(m_bloomMat, "shaders/hdr/hdr_bloom.fx");
    success &= createEffect(m_gaussianBlurMat, "shaders/hdr/fast_gaussian_blur.fx");

    MF_ASSERT(success && "Not all system resources have been loaded correctly.");
}

//--------------------------------------------------------------------------------------------------
void HDRSupport::deleteManagedObjects()
{
    BW_GUARD;

    //-- unregister shared auto-constant.
    Moo::rc().effectVisualContext().unregisterAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "HDRParams"
        );

    //-- unregister shared auto-constant.
    Moo::rc().effectVisualContext().unregisterAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "GammaCorrection"
        );
    
    m_toneMappingMat      = NULL;
    m_averageLuminanceMat = NULL;
    m_bloomMat            = NULL;
    m_gaussianBlurMat     = NULL;
}


BW_END_NAMESPACE
