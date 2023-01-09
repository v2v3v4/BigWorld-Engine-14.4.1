#include "pch.hpp"
#include "ppaa_support.hpp"
#include "fullscreen_quad.hpp"
#include "moo_utilities.hpp"
#include "render_event.hpp"
#include "effect_material.hpp"
#include "render_target.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
    //----------------------------------------------------------------------------------------------
    PPAASupport::PPAASupport() : m_enabled(false), m_inited(false), m_mode(0)
    {
    }
    
    //----------------------------------------------------------------------------------------------
    PPAASupport::~PPAASupport()
    {

    }

    //----------------------------------------------------------------------------------------------
    void PPAASupport::mode(uint index)
    {
        if (index >= 3)
        {
            ERROR_MSG("Invalid PSAA mode index.");
            return;
        }
        
        m_mode = index;
    }

    //----------------------------------------------------------------------------------------------
    void PPAASupport::enable(bool flag)
    {
        if (flag && !m_enabled && !m_inited)
        {
            // switching enable on, when uninitialised
            m_enabled = flag;   // checked during resource creation.
            createUnmanagedObjects();
        }
        else if (!flag && m_enabled && m_inited)
        {
            // switching enable off, release unneeded resources
            deleteUnmanagedObjects();
        }

        m_enabled = flag;
    }

    //----------------------------------------------------------------------------------------------
    void PPAASupport::apply()
    {
        BW_GUARD;
        BW_SCOPED_RENDER_PERF_MARKER("FXAA")

        if (!m_enabled || !m_inited) return;
        
        //-- make back-buffer copy.
        copyBackBuffer(m_rt.get());
        
        //-- setup constants.
        m_materials[m_mode]->pEffect()->pEffect()->SetTexture("g_bbCopyMap", m_rt->pTexture());

        //-- draw full screen quad with fxaa filter.
        rc().fsQuad().draw(*m_materials[m_mode].get());

        //-- set all textures to NULL, just in case the render target was set on 
        //-- any of the stages before.
        uint32 nTextures = Moo::rc().deviceInfo(Moo::rc().deviceIndex()).caps_.MaxSimultaneousTextures;
        for (uint32 i = 0; i < nTextures; ++i)
        {
            Moo::rc().setTexture(i, NULL);
        }
    }

    //----------------------------------------------------------------------------------------------
    void PPAASupport::createManagedObjects()
    {
        BW_GUARD;
        
        bool success = true;
        success &= createEffect(m_materials[0], "shaders/anti_aliasing/fxaa_LQ.fx");
        success &= createEffect(m_materials[1], "shaders/anti_aliasing/fxaa_MQ.fx");
        success &= createEffect(m_materials[2], "shaders/anti_aliasing/fxaa_HQ.fx");
        MF_ASSERT(success && "Not all system resources were loaded correctly.");
    }

    //----------------------------------------------------------------------------------------------
    void PPAASupport::deleteManagedObjects()
    {
        for (uint i = 0; i < 3; ++i)
            m_materials[i] = NULL;
    }

    //----------------------------------------------------------------------------------------------
    void PPAASupport::createUnmanagedObjects()
    {
        if (!m_enabled) return;

        //-- retrieve the current back buffer description.
        const D3DSURFACE_DESC& bbDesc = Moo::rc().backBufferDesc();

        //-- release old pointer.
        m_rt = NULL;

        //-- try to share render target with post-processing framework.
        Moo::BaseTexturePtr pTex = Moo::TextureManager::instance()->get("backBufferCopy", false, true, false);
        Moo::RenderTarget*  pRT  = dynamic_cast<Moo::RenderTarget*>(pTex.getObject());
        if (!Moo::rc().isMultisamplingEnabled() && pRT)
        {
            m_inited = true;
            m_rt = pRT;
            INFO_MSG("PPAASupport shares render target with post-processing framework.\n");
        }
        else
        {
            m_inited = createRenderTarget(m_rt, bbDesc.Width, bbDesc.Height, bbDesc.Format, "PPAA bbCopy", true);
            INFO_MSG("PPAASupport created its own render target with the back-buffer's format.\n");
        }
    }

    //----------------------------------------------------------------------------------------------
    void PPAASupport::deleteUnmanagedObjects()
    {
        m_rt = NULL;
        m_inited = false;
    }

    //----------------------------------------------------------------------------------------------
    bool PPAASupport::recreateForD3DExDevice() const
    {
        return true;
    }

} //-- Moo

BW_END_NAMESPACE
