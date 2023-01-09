#include "pch.hpp"
#include "moo_utilities.hpp"
#include "effect_material.hpp"
#include "render_target.hpp"
#include "moo/geometrics.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
    //----------------------------------------------------------------------------------------------
    bool createEffect(Moo::EffectMaterialPtr& mat, const BW::string& name)
    {
        BW_GUARD;
        if (!mat) mat = new Moo::EffectMaterial;

        if (mat->initFromEffect(name))
        {
            return true;
        }

        mat = NULL;
        return false;
    }

    //----------------------------------------------------------------------------------------------
    bool createRenderTarget(
        Moo::RenderTargetPtr& rt, uint width, uint height, D3DFORMAT pixelFormat,
        const BW::string& name, bool useMainZBuffer, bool discardZBuffer)
    {
        BW_GUARD;
        if (Moo::rc().supportsTextureFormat(pixelFormat))
        {
            if (!rt)
            {
                rt = new Moo::RenderTarget(name);
            }

            if (rt->create(width, height, useMainZBuffer, pixelFormat, NULL, D3DFMT_UNKNOWN, discardZBuffer))
            {
                return true;
            }
        }

        rt = NULL;
        return false;
    }

    //----------------------------------------------------------------------------------------------
    bool copyBackBuffer(RenderTarget* outRT)
    {
        return copyRT(outRT, 0);
    }

    //----------------------------------------------------------------------------------------------
    bool copyRT(RenderTarget* outRT, uint rtSlot)
    {
        BW_GUARD;

        ComObjectWrap<DX::Surface> inRT = Moo::rc().getRenderTarget(rtSlot);

        return copySurface(outRT, inRT);
    }
    
    //----------------------------------------------------------------------------------------------
    bool copySurface(RenderTarget* outRT, const ComObjectWrap<DX::Surface>& surface)
    {
        BW_GUARD;

        MF_ASSERT(surface != NULL);
        MF_ASSERT(outRT != NULL);

        if (!surface || !outRT)
            return false;

        ComObjectWrap<DX::Surface> pDest;
        if (SUCCEEDED(outRT->pSurface(pDest)))
        {
            HRESULT hr = Moo::rc().device()->StretchRect(
                surface.pComObject(), NULL, pDest.pComObject(), NULL, D3DTEXF_LINEAR
                );
            MF_ASSERT(SUCCEEDED(hr) && "Can't stretch back-buffer.");

            if (FAILED(hr))
                return false;
        }
        else
        {
            MF_ASSERT(!"Can't extract surface from the incoming render target.");
            return false;
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool copyRT(
        RenderTarget* destRT, 
        const RECT* destRect, 
        RenderTarget* srcRT,
        const RECT*  srcRect, 
        D3DTEXTUREFILTERTYPE filter
    )
    {
        ComObjectWrap<DX::Surface> destS, srcS;

        if ( SUCCEEDED( destRT->pSurface( destS ) ) && SUCCEEDED( srcRT->pSurface( srcS ) ) )
        {
            HRESULT hr = Moo::rc().device()->StretchRect( srcS.pComObject(), srcRect, destS.pComObject(), destRect, filter );
            
            MF_ASSERT( SUCCEEDED(hr) && "Can't stretch back-buffer." );
            if ( FAILED( hr ) )
                return false;
        }
        else
        {
            MF_ASSERT( !"Can't extract surface from the ( src/dest ) render target." );
            return false;
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    void drawDebugTexturedRect(const Vector2& topLeft, const Vector2& bottomRight, 
        DX::BaseTexture* texture, Moo::Colour color)
    {
        BW_GUARD;

#ifndef BW_EXPORTER
        Moo::Material::setVertexColour();
        Moo::rc().setRenderState(D3DRS_ZENABLE, FALSE);
        Moo::rc().setRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
        Moo::rc().setTexture(0, texture);
        Geometrics::texturedRect(topLeft, bottomRight, color, false);
        Moo::rc().setTexture(0, NULL);
#endif
    }

#if ENABLE_RESOURCE_COUNTERS
    BW::string  getAllocator(const char* module, const BW::string& resourceID)
    {
        BW::string allocStr = module;
        allocStr += resourceID;
        std::replace( allocStr.begin() + strlen(module), allocStr.end(), '/', '|' );
        return allocStr;
    }
#endif

} //-- Moo

BW_END_NAMESPACE
