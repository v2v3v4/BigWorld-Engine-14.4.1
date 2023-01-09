#include "pch.hpp"
#include "taa_support.hpp"
#include "fullscreen_quad.hpp"
#include "moo_utilities.hpp"
#include "render_target.hpp"
#include "effect_material.hpp"
#include "camera.hpp"

BW_BEGIN_NAMESPACE


//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
    //-- watchers.
    uint  g_prevSamplesPattern = 0;
    uint  g_samplesPattern = 0;
    float g_depthDelta = 0.01f;
    bool  g_enable = true;
    bool  g_debug = false;
    bool  g_enableJittering = true;

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

    //-- Copy content of the second MRT target which contains depth into the desired render target.
    //-- Note: Here we do a full screen pass to convert RGBA color to R16F.
    //----------------------------------------------------------------------------------------------
    void copyDepthWithResolve(Moo::RenderTarget* outRT, const Moo::EffectMaterial& resolveMaterial)
    {
        BW_GUARD;
        if (outRT->push())
        {
            //MRTSupport::instance().bind();
            Moo::rc().fsQuad().draw(resolveMaterial);
            //MRTSupport::instance().unbind();

            outRT->pop();
        }
        else
        {
            MF_ASSERT(!"Can't bind depth buffer copy RT.");
        }
    }
    
    //----------------------------------------------------------------------------------------------
    Matrix jitteredFrustum(
        float left, float right, float bottom, float top, float fNear, float fFar,
        float pixdx, float pixdy, float widht, float height
        )
    {
        BW_GUARD;
        float xwsize = right - left;
        float ywsize = top - bottom;

        //-- translate the screen space jitter distances into near clipping plane distances
        float dx = -(pixdx * xwsize / widht );
        float dy = -(pixdy * ywsize / height);

        Matrix outMat;
        D3DXMatrixPerspectiveOffCenterLH(
            &outMat, left + dx, right + dx, bottom + dy, top + dy, fNear, fFar
            );

        return outMat;
    }

    //----------------------------------------------------------------------------------------------
    Matrix jitteredPerspective(
        const Moo::Camera& camera, float pixdx, float pixdy, float width, float height
        )
    {
        BW_GUARD;
        float top    = camera.nearPlane() * tanf(camera.fov() * 0.5f);
        float bottom = -top;
        float right  = top * camera.aspectRatio();
        float left   = -right;

        return jitteredFrustum(
            left, right, bottom, top, camera.nearPlane(), camera.farPlane(),
            pixdx, pixdy, width, height
            );
    }
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.


namespace Moo
{
    //----------------------------------------------------------------------------------------------
    TemporalAASupport::TemporalAASupport()
        :   m_screenRes(1,1), m_sampleCounter(0), m_isEnabled(false), m_samplePatternChanged(true)
    {
        MF_WATCH("Render/taa/enable", g_enable, Watcher::WT_READ_WRITE,
            "enable/disable Temporal Anti Aliasing."
            );

        MF_WATCH("Render/taa/samples pattern", g_samplesPattern, Watcher::WT_READ_WRITE,
            "Use desired samples pattern 0 - MSAA 2x, 1 - FSAA 4x, 2 - MSAA 4x(RGAA)."
            );

        MF_WATCH("Render/taa/depth delta", g_depthDelta, Watcher::WT_READ_WRITE,
            "Depth delta."
            );

        MF_WATCH("Render/taa/enable debugging", g_debug, Watcher::WT_READ_WRITE,
            "Enable/Disable debugging."
            );

        MF_WATCH("Render/taa/enable jittering", g_enableJittering, Watcher::WT_READ_WRITE,
            "Enable/Disable camera jittering."
            );
    }

    //----------------------------------------------------------------------------------------------
    TemporalAASupport::~TemporalAASupport()
    {

    }

    //----------------------------------------------------------------------------------------------
    bool TemporalAASupport::enable() const
    {
        BW_GUARD;
        return g_enable;
    }

    //----------------------------------------------------------------------------------------------
    void TemporalAASupport::enable(bool flag)
    {
        BW_GUARD;
        g_enable = flag;
    }

    //----------------------------------------------------------------------------------------------
    void TemporalAASupport::nextFrame()
    {
        BW_GUARD;
        if (!g_enable || !g_enableJittering)
        {
            m_curSample = SampleDesc();
            return;
        }

        bool needReset = false;

        if (g_prevSamplesPattern != g_samplesPattern)
        {
            m_sampleCounter = 0;
            g_prevSamplesPattern = g_samplesPattern;
        }
    
        //-- save previous jitter offset.
        m_lastJitterOffset = m_curSample.m_offset;

        //-- MSAA 2x pattern
        if (g_samplesPattern == 0)
        {
            const Vector2 samples[] = 
            {
                Vector2(+0.25f, -0.25f),
                Vector2(-0.25f, +0.25f)
            };

            if (m_sampleCounter == 1)
                needReset = true;

            m_curSample.m_number = m_sampleCounter;
            m_curSample.m_offset = samples[m_sampleCounter];
            m_curSample.m_weight = 0.5f;
        }
        //-- FSAA 4x pattern it uses regular grid.
        else if (g_samplesPattern == 1)
        {
            const Vector2 samples[] = 
            {
                Vector2(+0.25f, -0.25f),
                Vector2(-0.25f, +0.25f),
                Vector2(-0.25f, -0.25f),
                Vector2(+0.25f, +0.25f)
            };

            if (m_sampleCounter == 3)
                needReset = true;

            m_curSample.m_number = m_sampleCounter;
            m_curSample.m_offset = samples[m_sampleCounter];
            m_curSample.m_weight = 0.25f;
        }
        //-- MSAA 4x or rotated grid pattern also called RGSS (Rotated Grid Super-sampling).
        else if (g_samplesPattern == 2)
        {
            const Vector2 samples[] = 
            {
                Vector2(-0.375f, +0.125f),
                Vector2(+0.375f, -0.125f),
                Vector2(+0.125f, +0.375f),
                Vector2(-0.125f, -0.375f)
            };

            if (m_sampleCounter == 3)
                needReset = true;

            m_curSample.m_number = m_sampleCounter;
            m_curSample.m_offset = samples[m_sampleCounter];
            m_curSample.m_weight = 0.25f;
        }

        //-- increment sampler counter.
        if (needReset)  m_sampleCounter = 0;
        else            ++m_sampleCounter;
    }

    //----------------------------------------------------------------------------------------------
    Matrix TemporalAASupport::jitteredProjMatrix(const Moo::Camera& cam)
    {
        return jitteredPerspective(
            cam, m_curSample.m_offset.x, m_curSample.m_offset.y, m_screenRes.x, m_screenRes.y
            );
    }

    //----------------------------------------------------------------------------------------------
    void TemporalAASupport::resolve()
    {
        BW_GUARD;
        if (!g_enable) return;
        
        //-- current sample's number.
        uint texID = m_curSample.m_number;

        //-- MSAA 2x
        if (g_samplesPattern == 0)
        {
            //-- 1. initial preparation in case if we switched from one samples pattern to another.
            if (m_samplePatternChanged)
            {
                copyBackBuffer(m_colorRTCopies[0].get());
                copyBackBuffer(m_colorRTCopies[1].get());
            }
            
            //-- 2. backup current back buffer data into appropriate temporal render target.
            copyBackBuffer(m_colorRTCopies[texID].get());

            //-- 3. draw full screen quad.
            {
                ID3DXEffect* effect = m_resolveTAAx2Material->pEffect()->pEffect();
                {
                    //-- set current and previous textures to blend.
                    for (uint i = 0; i < 2; ++i)
                    {
                        if (i == texID) effect->SetTexture("g_srcColorTex",  m_colorRTCopies[i]->pTexture());
                        else            effect->SetTexture("g_lastColorTex", m_colorRTCopies[i]->pTexture());
                    }
                    
                    //-- set the rest of parameters.
                    effect->SetTexture   ("g_lastDepthTex",   m_depthRTCopy->pTexture());
                    effect->SetFloatArray("g_jitterOffs",     m_curSample.m_offset, 2);
                    effect->SetFloatArray("g_lastJitterOffs", m_lastJitterOffset, 2);
                    effect->SetFloat     ("g_sampleWeight",   m_curSample.m_weight);

                    effect->CommitChanges();
                }
                Moo::rc().fsQuad().draw(*m_resolveTAAx2Material.get());
            }

            //-- 4. backup depth target.
            copyDepthWithResolve(m_depthRTCopy.get(), *m_resolveDepthMaterial.get());
        }
        //-- FSAA 4x or MSAA 4x(RGAA)
        else if (g_samplesPattern == 1 || g_samplesPattern == 2)
        {
            //-- 1. initial preparation in case if we switched from one samples pattern to another.
            if (m_samplePatternChanged)
            {
                copyBackBuffer(m_colorRTCopies[0].get());
                copyBackBuffer(m_colorRTCopies[1].get());
                copyBackBuffer(m_colorRTCopies[2].get());
                copyBackBuffer(m_colorRTCopies[3].get());
            }

            //-- 2. backup current back buffer data into appropriate temporal render target.
            copyBackBuffer(m_colorRTCopies[texID].get());

            //-- 3. draw full screen quad.
            {
                ID3DXEffect* effect = m_resolveTAAx4Material->pEffect()->pEffect();
                {
                    //-- set last frame textures based on the current sample index.
                    const char* textures[] = {"g_lastColorTex0", "g_lastColorTex1", "g_lastColorTex2"};
                    for (uint i = 0, offset = 0; i < 4; ++i)
                    {
                        if (i == m_curSample.m_number)
                        {
                            offset = -1;
                            continue;
                        }
                        effect->SetTexture(textures[i + offset], m_colorRTCopies[i]->pTexture());
                    }

                    //-- set the rest of parameters.
                    effect->SetTexture   ("g_lastDepthTex",   m_depthRTCopy->pTexture());
                    effect->SetFloatArray("g_jitterOffs",     m_curSample.m_offset, 2);
                    effect->SetFloatArray("g_lastJitterOffs", m_lastJitterOffset, 2);
                    effect->SetFloat     ("g_sampleWeight",   m_curSample.m_weight);

                    effect->CommitChanges();
                }
                Moo::rc().fsQuad().draw(*m_resolveTAAx4Material.get());
            }

            //-- 4. backup depth target.
            copyDepthWithResolve(m_depthRTCopy.get(), *m_resolveDepthMaterial.get());
        }
    }

    //----------------------------------------------------------------------------------------------
    void TemporalAASupport::createUnmanagedObjects()
    {
        BW_GUARD;

        bool                   success = true;
        const D3DSURFACE_DESC& bbDesc  = Moo::rc().backBufferDesc();

        m_screenRes.x = static_cast<float>(bbDesc.Width);
        m_screenRes.y = static_cast<float>(bbDesc.Height);

        for (uint i = 0; i < NUM_TEMPORAL_RTS; ++i)
        {
            BW::string rtName = makeStr("TAA temporal color texture #%d", i);
            success &= createRenderTarget(
                m_colorRTCopies[i], bbDesc.Width, bbDesc.Height, bbDesc.Format, rtName, true
                );
        }
        success &= createRenderTarget(
            m_depthRTCopy, bbDesc.Width, bbDesc.Height, D3DFMT_R32F, "TAA temporal depth copy", true
            );

        MF_ASSERT_DEV(success && "Not all system resources were loaded correctly.");
    }

    //----------------------------------------------------------------------------------------------
    void TemporalAASupport::deleteUnmanagedObjects()
    {
        BW_GUARD;
        
        for (uint i = 0; i < NUM_TEMPORAL_RTS; ++i)
            m_colorRTCopies[i] = NULL;

        m_depthRTCopy = NULL;
    }

    //----------------------------------------------------------------------------------------------
    void TemporalAASupport::createManagedObjects()
    {
        BW_GUARD;

        bool success = true;
        success &= createEffect(m_resolveDepthMaterial, "shaders/std_effects/depth_buf_resolve.fx");
        success &= createEffect(m_resolveTAAx2Material, "shaders/std_effects/temporal_aa_x2.fx");
        success &= createEffect(m_resolveTAAx4Material, "shaders/std_effects/temporal_aa_x4.fx");

        MF_ASSERT_DEV(success && "Not all system resources were loaded correctly.");
    }

    //----------------------------------------------------------------------------------------------
    void TemporalAASupport::deleteManagedObjects()
    {
        BW_GUARD;

        //-- reset smart-pointers.
        m_resolveDepthMaterial = NULL;
        m_resolveTAAx2Material = NULL;
        m_resolveTAAx4Material = NULL;
    }

} //-- Moo

BW_END_NAMESPACE
