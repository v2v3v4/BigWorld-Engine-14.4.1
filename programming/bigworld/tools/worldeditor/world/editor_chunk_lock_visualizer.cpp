#include "pch.hpp"
#include "editor_chunk_lock_visualizer.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "moo/visual_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "moo/effect_material.hpp"
#include "moo/renderer.hpp"

BW_BEGIN_NAMESPACE

void EditorChunkLockVisualizer::createManagedObjects()
{
    BW_GUARD;

    bool success = true;

    //-- create materials.
    BW::string effectName = "resources/shaders/lock_marker.fx";

    const char* techniques[] = {
        "STENCIL",
        "DIFFUSE",
    };

    for (uint i = 0; i < 2; ++i)
    {
        m_materials[i] = new Moo::EffectMaterial();
        success &= m_materials[i]->initFromEffect(effectName);
        m_materials[i]->hTechnique(techniques[i]);
    }

    //-- create cube mesh.
    m_cube   = Moo::VisualManager::instance()->get("system/models/fx_unite_cube.visual");
    success &= m_cube.exists();

    MF_ASSERT_DEV(success && "Not all system resources have been loaded correctly.");
}

void EditorChunkLockVisualizer::deleteManagedObjects()
{
    BW_GUARD;
    //--
    for (uint i = 0; i < 2; ++i)
    {
        m_materials[i] = NULL;
    }

    //--
    m_cube = NULL;
}

void EditorChunkLockVisualizer::createUnmanagedObjects()
{
    BW_GUARD;

    bool success = true;

    //-- create buffer for the instancing drawing.
    //-- ToDo: reconsider memory usage. Currently is float4x4 * 512 -> 64 * 512 -> 32kb
    success &= SUCCEEDED(m_instancedVB.create(
        sizeof(GPUBox) * 512, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT,
        "vertex buffer/EditorChunkLockVisualizer instancing VB"
        ));

    MF_ASSERT(success && "Can't create instancing buffer for chunk lock pass.");
}

void EditorChunkLockVisualizer::deleteUnmanagedObjects()
{
    BW_GUARD;
    m_instancedVB.release();
}

Vector4 colorToFloat(DWORD color)
{
    return Vector4( (float)GetRValue(color) / 255.0f,
                    (float)GetGValue(color) / 255.0f,
                    (float)GetBValue(color) / 255.0f,
                    (float)(LOBYTE((color)>>24)) / 255.0f);
}

EditorChunkLockVisualizer::GPUBox EditorChunkLockVisualizer::fromBoundBox2GPUBox(
    const BoundingBox & bbox, const Matrix & transform, DWORD color )
{
    GPUBox outBox;

    Matrix bboxTransform = transform;
    bboxTransform.preTranslateBy( bbox.centre() );
    Vector3 max_ = bbox.maxBounds();
    Vector3 min_ = bbox.minBounds();
    Vector3 d = max_ - min_;
    Vector4 clr = colorToFloat(color);

    Vector3 direction = bboxTransform.applyToUnitAxisVector( 2 );
    Vector3 up = bboxTransform.applyToUnitAxisVector( 1 );
    d = d * bboxTransform.nonUniformScale();

    //Shader seems to refer to dir as up and vice-versa
    outBox.m_dir = Vector4( up, clr.x );
    outBox.m_pos = Vector4( bboxTransform[ 3 ], clr.y );
    outBox.m_up = Vector4( direction, clr.z );
    outBox.m_scale = Vector4( d, clr.w );

    return outBox;
}

void EditorChunkLockVisualizer::addBox(
    const BoundingBox& bbox, const Matrix & transform, DWORD color )
{
    m_Vertices.push_back(fromBoundBox2GPUBox(bbox, transform, color));
}

void EditorChunkLockVisualizer::fillVB()
{
    size_t vbSize = m_instancedVB.size();
    size_t requiredSize =  m_Vertices.size() * sizeof( GPUBox );
    if( vbSize < requiredSize )
    {
        // need to expand vb
        m_instancedVB.release();
        int requestSize = static_cast< int >(requiredSize);

        // expand to next step
        static const int s_stepSize = 64 * sizeof( GPUBox );
        requestSize += s_stepSize - requestSize % s_stepSize;

        bool success = true;
        success &= SUCCEEDED(m_instancedVB.create(
            requestSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT,
            "vertex buffer/EditorChunkLockVisualizer instancing VB"
        ));
        MF_ASSERT(success && "Can't create instancing buffer for chunk lock pass.");
    }

    Moo::VertexLock<GPUBox> vl( m_instancedVB, 0, ( int ) m_Vertices.size() * sizeof(GPUBox), D3DLOCK_DISCARD );

    if (vl)
    {
        for (uint j = 0; j < m_Vertices.size(); ++j)
        {
            vl[j] = m_Vertices[j];
        }
    }
}

void EditorChunkLockVisualizer::draw()
{
    BW_GUARD;

    if(m_Vertices.empty())
        return;
    
    Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );
    Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
        D3DCOLORWRITEENABLE_RED |
        D3DCOLORWRITEENABLE_GREEN |
        D3DCOLORWRITEENABLE_BLUE );
    
    //-- 1. Fill instanced vertex buffer
    fillVB();

    //-- 2. set render targets.
    if (Moo::rc().pushRenderTarget())
    {
        //-- Notice that we bind on the output 2 render target but in the shader based on the desired
        //-- material we can turn off writing into either of them.
        Moo::rc().setRenderTarget(0, Renderer::instance().pipeline()->gbufferSurface(2));
        Moo::rc().setRenderTarget(1, Renderer::instance().pipeline()->gbufferSurface(1));

        m_cube->instancingStream(&m_instancedVB);

        //-- 5. render with different materials.
        for (uint i = 0; i < 2; ++i)
        {
            const Moo::EffectMaterial& material = *m_materials[i];

            if (m_Vertices.size() && material.begin())
            {
                for (uint32 j = 0; j < material.numPasses(); ++j)
                {
                    material.beginPass(j);
                    m_cube->justDrawPrimitivesInstanced(0, ( uint ) m_Vertices.size());
                    material.endPass();
                }
                material.end();
            }
        }

        //-- restore state.
        Moo::rc().popRenderTarget();
    }
    m_Vertices.clear();
    Moo::rc().popRenderState();
}

BW_END_NAMESPACE
