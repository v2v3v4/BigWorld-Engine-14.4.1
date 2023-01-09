#include "pch.hpp"
#include "fullscreen_quad.hpp"
#include "effect_material.hpp"
#include "vertex_declaration.hpp"
#include "moo_utilities.hpp"

BW_BEGIN_NAMESPACE

//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
    //-- build full-screen quad mesh with respect to current screen dimension.
    //----------------------------------------------------------------------------------------------
    template <typename MeshType>
    void buildFSQuadMesh(MeshType & mesh, uint width, uint height)
    {
        BW_GUARD;

        //-- calculate adjust factor.
        Vector3 adjust(-1.0f / width, +1.0f / height, 0.f);

        //-- left-top
        mesh[0].pos_.set(-1.f, 1.f, 0.0f);
        mesh[0].uv_.set (0.f, 0.f);

        //-- left-bottom        
        mesh[1].pos_.set(-1.f, -1.f, 0.0f);
        mesh[1].uv_.set (0.f, 1.f);

        //-- right-top
        mesh[2].pos_.set(1.f, 1.f, 0.0f);
        mesh[2].uv_.set (1.f, 0.f);

        //-- right-bottom
        mesh[3].pos_.set(1.f, -1.f, 0.0f);
        mesh[3].uv_.set (1.f, 1.f);

        //-- unadjusted vertex values
        for (uint32 i = 0; i < 4; ++i)
        {
            mesh[i].pos_ = mesh[i].pos_ + adjust;
        }
    }
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.


namespace Moo
{
    //----------------------------------------------------------------------------------------------
    FullscreenQuad::FullscreenQuad() :
        m_vecDclr(NULL)
    {

    }

    //----------------------------------------------------------------------------------------------
    FullscreenQuad::~FullscreenQuad()
    {

    }

    //----------------------------------------------------------------------------------------------
    void FullscreenQuad::draw(const EffectMaterial& material)
    {
        BW_GUARD;

        if (material.begin())
        {
            for (uint32 i = 0; i < material.numPasses(); ++i)
            {
                material.beginPass(i);
                m_FSQuadMesh.drawEffect(m_vecDclr);
                material.endPass();
            }
            material.end();
        }
    }

    //----------------------------------------------------------------------------------------------
    void FullscreenQuad::draw(const EffectMaterial& material, uint width, uint height)
    {
        BW_GUARD;

        CustomMesh<Moo::VertexXYZUV> quadMesh(D3DPT_TRIANGLESTRIP);
        quadMesh.resize(4);
        buildFSQuadMesh(quadMesh, width, height);

        if (material.begin())
        {
            for (uint32 i = 0; i < material.numPasses(); ++i)
            {
                material.beginPass(i);
                quadMesh.drawEffect(m_vecDclr);
                material.endPass();
            }
            material.end();
        }
    }
    
    //----------------------------------------------------------------------------------------------
    void FullscreenQuad::clearChannel( const Vector4 & color, uint32 colorWriteEnable, uint width, uint height )
    {
        ID3DXEffect* effect = m_clearFM->pEffect()->pEffect();

        effect->SetVector( "g_color", &color );
        effect->CommitChanges();

        Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );
        Moo::rc().setWriteMask( 0, colorWriteEnable );
        
        draw( *( m_clearFM ), width, height );
        
        Moo::rc().popRenderState();
    }

    //----------------------------------------------------------------------------------------------
    void FullscreenQuad::clearChannel( const Vector4 & color, uint32 colorWriteEnable )
    {
        ID3DXEffect* effect = m_clearFM->pEffect()->pEffect();

        effect->SetVector( "g_color", &color );
        effect->CommitChanges();

        Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );
        Moo::rc().setWriteMask( 0, colorWriteEnable );

        draw( *( m_clearFM ) );

        Moo::rc().popRenderState();
    }

    //----------------------------------------------------------------------------------------------
    void FullscreenQuad::createManagedObjects()
    {
        BW_GUARD;
        bool isOk = true;

        m_vecDclr = Moo::VertexDeclaration::get("xyzuv");
        isOk = Moo::createEffect( m_clearFM, "shaders/std_effects/quad_clear.fx" );

        MF_ASSERT( m_vecDclr && isOk && "Not all system resources were loaded correctly." );
    }

    //----------------------------------------------------------------------------------------------
    void FullscreenQuad::deleteManagedObjects()
    {
        m_vecDclr = NULL;
        m_clearFM = NULL;
    }

    //----------------------------------------------------------------------------------------------
    void FullscreenQuad::createUnmanagedObjects()
    {
        BW_GUARD;

        //-- retrieve the current back buffer description.
        const D3DSURFACE_DESC& bbDesc = Moo::rc().backBufferDesc();

        //-- rebuild full-screen quad.
        Moo::VertexXYZUV * mesh = m_FSQuadMesh.lock< Moo::VertexXYZUV >(
            D3DPT_TRIANGLESTRIP, 4 );
        buildFSQuadMesh( mesh, bbDesc.Width, bbDesc.Height );
        m_FSQuadMesh.unlock();
    }

    //----------------------------------------------------------------------------------------------
    void FullscreenQuad::deleteUnmanagedObjects()
    {
        m_FSQuadMesh.release();
    }

    //----------------------------------------------------------------------------------------------
    bool FullscreenQuad::recreateForD3DExDevice() const
    {
        return true;
    }

} //-- Moo

BW_END_NAMESPACE
