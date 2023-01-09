#include "pch.hpp"

#include "deferred_decals_manager.hpp"
#include "moo/effect_material.hpp"
#include "moo/visual_manager.hpp"
#include "moo/moo_utilities.hpp"
#include "moo/texture_manager.hpp"
#include "cstdmf/dogwatch.hpp"
#include "cstdmf/watcher.hpp"
#include "renderer.hpp"
#include "render_event.hpp"
#include "texture_atlas.hpp"
#include "chunk/umbra_config.hpp"
#include "chunk/chunk_umbra.hpp"

BW_BEGIN_NAMESPACE


//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
    //-- maximum number of simultaneously visible decals.
    uint    g_maxVisibleDecals  = 1024;
    uint    g_curVisibleDecals  = 0;
    uint32  g_staticAtlasSize   = 4096;
    bool    g_showStaticAtlas   = false;
    uint32  g_curTotalDecals    = 0;

    //-- decompose matrix into scale vector, translation vector and rotation matrix.
    void decomposeMatrix(Vector3& pos, Vector3& dir, Vector3& up, Vector3& scale, const Matrix& iMat)
    {
        BW_GUARD;

        scale.x = iMat[0].length();
        scale.y = iMat[1].length();
        scale.z = iMat[2].length();

        dir = iMat.applyToUnitAxisVector(2);
        up  = iMat.applyToUnitAxisVector(1);
        pos = iMat.applyToOrigin();

        dir.normalise();
        up.normalise();
    }
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.


BW_SINGLETON_STORAGE(DecalsManager)


//--------------------------------------------------------------------------------------------------
DecalsManager::StaticDecals::StaticDecals()
{
    m_cache.reserve(500);
    m_freeIndices.reserve(100);
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::StaticDecals::insert(int32 uid, const DecalsManager::Decal& value)
{
    if (uint32(uid) >= m_cache.size())  m_cache.push_back(value);
    else                                m_cache[uid] = value;
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::StaticDecals::remove(int32 uid)
{
    m_cache[uid] = Decal();
    m_freeIndices.push_back(uid);
}

//--------------------------------------------------------------------------------------------------
int32 DecalsManager::StaticDecals::nextGUID() const
{
    int32 next = (int32)m_cache.size();
    if (m_freeIndices.size() != 0)
    {
        next = m_freeIndices.back();
        m_freeIndices.pop_back();
    }
    return next;
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::StaticDecals::sort(BW::vector<int32>& indices) const
{
    //-- sorting functor.
    struct Sorter
    {
        Sorter(const StaticDecals& decals) : m_container(decals) { }

        bool operator() (uint lft, uint rht) const 
        {
            const Decal& left  = m_container.get(lft);
            const Decal& right = m_container.get(rht);
            
            if (left.m_materialType == right.m_materialType)
            {
                return left.m_priority < right.m_priority;
            }
            else
            {
                return left.m_materialType < right.m_materialType;
            }
        }

        const StaticDecals& m_container;
    };

    std::sort(indices.begin(), indices.end(), Sorter(*this));
}

//--------------------------------------------------------------------------------------------------
DecalsManager::DynamicDecals::DynamicDecals() : m_realSize(0)
{
    m_cache.reserve(500);
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::DynamicDecals::add(const DecalsManager::DynamicDecal& value)
{
    if (m_realSize < m_cache.size())    m_cache[m_realSize] = value;
    else                                m_cache.push_back(value);

    ++m_realSize;
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::DynamicDecals::remove(int32 uid)        
{ 
    if (m_realSize > 1)     m_cache[uid] = m_cache[m_realSize-1];
    else                    m_cache[uid] = DecalsManager::DynamicDecal();

    --m_realSize;
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::DynamicDecals::sort(BW::vector<int32>& indices) const
{
    //-- sorting functor.
    struct Sorter
    {
        Sorter(const DynamicDecals& decals) : m_container(decals) { }

        bool operator() (uint lft, uint rht) const 
        {
            const DynamicDecal& left  = m_container.get(lft);
            const DynamicDecal& right = m_container.get(rht);
            
            if (left.m_materialType == right.m_materialType)
            {
                return left.m_priority < right.m_priority;
            }
            else
            {
                return left.m_materialType < right.m_materialType;
            }
        }

        const DynamicDecals& m_container;
    };

    std::sort(indices.begin(), indices.end(), Sorter(*this));
}

//--------------------------------------------------------------------------------------------------
DecalsManager::DecalsManager()
    :   m_time(0),
        m_staticAtlasSize(1,1,1,1),
        m_fadeRange(100, 150),
        m_internalFadeRange(0,0,0),
        m_debugDrawEnabled(false),
        m_activeQualityOption(0)
{
    MF_WATCH("Render/DS/Decals/show static decals atlas",
        g_showStaticAtlas, Watcher::WT_READ_WRITE, "Show static decals atlas."
        );
    MF_WATCH("Render/DS/Decals/max visible decals",
        g_maxVisibleDecals, Watcher::WT_READ_ONLY,  "Maximum number of visible decals."
        );
    MF_WATCH("Render/DS/Decals/current visible decals",
        g_curVisibleDecals, Watcher::WT_READ_ONLY,  "Number currently visible decals."
        );
    MF_WATCH("Render/DS/Decals/total decals count" ,
        g_curTotalDecals, Watcher::WT_READ_ONLY,    "Total number of decals."
        );
    MF_WATCH("Render/DS/Decals/static decals atlas size",
        g_staticAtlasSize, Watcher::WT_READ_ONLY,   "Static decals atlas size."
        );

    //-- setup quality options.
    m_qualityOptions.push_back(QualityOption(1.0f)); //-- MAX
    m_qualityOptions.push_back(QualityOption(0.8f)); //-- HIGH
    m_qualityOptions.push_back(QualityOption(0.6f)); //-- MEDIUM
    m_qualityOptions.push_back(QualityOption(0.4f)); //-- LOW
    m_qualityOptions.push_back(QualityOption(0.0f)); //-- OFF

    //-- update internal fade range.
    setFadeRange(m_fadeRange.x, m_fadeRange.y);
}

//--------------------------------------------------------------------------------------------------
DecalsManager::~DecalsManager()
{

}

//--------------------------------------------------------------------------------------------------
bool DecalsManager::init()
{
    BW_GUARD;

    m_dynamicDecalsManager.reset(new DynamicDecalManager());

    return true;
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::tick(float dt)
{
    BW_GUARD;

    //-- prepare to next frame
    prepareToNextFrame();

    //-- increment timing.
    m_time += dt; 

    //-- ToDo: implement some kind of hierarchical culling.
    //-- resolve visibility for dynamic decals.
    for (uint i = 0; i < m_dynamicDecals.size(); ++i)
    {
        DynamicDecal& decal = m_dynamicDecals.get(i);

        if (decal.m_creationTime + decal.m_lifeTime <= m_time)
        {
            m_dynamicDecals.remove(i--);
            continue;
        }

#ifdef EDITOR_ENABLED
        Matrix m = Moo::rc().view();
        m.preTranslateBy(viewSpaceOffset);
        m.postMultiply(Moo::rc().projection());
        decal.m_worldBounds.calculateOutcode(m);
#else
        decal.m_worldBounds.calculateOutcode(Moo::rc().viewProjection());
#endif
        if (!decal.m_worldBounds.combinedOutcode())
        {
            m_visibleDynamicDecals.push_back(i);
        }
    }

    g_curTotalDecals = uint32(m_staticDecals.size() + m_dynamicDecals.size());
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::setFadeRange(float start, float end)
{
    BW_GUARD;

    if (start > 0.0f && end > 0.0f && end > start)
    {
        //-- save requested fade range.
        m_fadeRange = Vector2(start, end);

        //-- apply quality option lod bias.
        Vector2 biasedFadeRange = m_fadeRange * m_qualityOptions[m_activeQualityOption].m_lodBias;

        //-- update internal state.
        m_internalFadeRange.x = biasedFadeRange.x * biasedFadeRange.x;
        m_internalFadeRange.y = biasedFadeRange.y * biasedFadeRange.y;
        m_internalFadeRange.z = 1.0f / (m_internalFadeRange.y - m_internalFadeRange.x);
    }
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::markVisible(int32 idx)
{
    BW_GUARD;

    //-- skip decals outside the visible area.
    {
        const Vector3& decalPos     = m_staticDecals.get(idx).m_pos;
        const Vector3& camPos       = Moo::rc().invView().applyToOrigin();
        float          distSquared  = ((decalPos - camPos) * Moo::rc().lodZoomFactor()).lengthSquared();

        if (distSquared > m_internalFadeRange.y)
            return;
    }

    m_visibleStaticDecals.push_back(idx);
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::prepareToNextFrame()
{
    BW_GUARD;

    //-- clear intermediate data structures.
    for (uint i = 0; i < Decal::MATERIAL_COUNT + 1; ++i)
    {
        m_drawData[i].m_offset = 0;
        m_drawData[i].m_size   = 0;
    }

    for (uint i = 0; i < Decal::MATERIAL_COUNT; ++i)
    {
        m_staticGPUDecals[i].clear();
        m_dynamicGPUDecals[i].clear();
    }

    m_visibleStaticDecals.clear();
    m_visibleDynamicDecals.clear();
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::prepareVisibleDecals()
{
    BW_GUARD;

    //-- sort static and dynamic decals by material and drawing priority.
    m_staticDecals.sort(m_visibleStaticDecals);
    m_dynamicDecals.sort(m_visibleDynamicDecals);

    //-- now convert already sorted visible decals to the GPU acceptable form.
    for (uint i = 0; i < m_visibleStaticDecals.size(); ++i)
    {
        const Decal& d = m_staticDecals.get(m_visibleStaticDecals[i]);
        float fadeVal = calcFadeValue(d);
        if (fadeVal != 0 && d.m_map1 != NULL)
        {
            m_staticGPUDecals[d.m_materialType].push_back(convert2GPU(d, fadeVal));
        }
    }

    //-- 
    for (uint i = 0; i < m_visibleDynamicDecals.size(); ++i)
    {
        const DynamicDecal& d     = m_dynamicDecals.get(m_visibleDynamicDecals[i]);
        float               blend = 1.0f - (m_time - d.m_creationTime) / d.m_lifeTime;
        float fadeVal = calcFadeValue(d) * d.m_startAlpha;
        if (fadeVal != 0 && d.m_map1 != NULL)
        {
#ifdef EDITOR_ENABLED
            DynamicDecal decal = d;
            decal.m_pos += viewSpaceOffset;
            m_dynamicGPUDecals[d.m_materialType].push_back(convert2GPU(decal, fadeVal * blend));
#else
            m_dynamicGPUDecals[d.m_materialType].push_back(convert2GPU(d, fadeVal * blend));
#endif
        }
    }

    //-- and finally prepare draw data structures.
    for (uint i = 0, offset = 0; i < Decal::MATERIAL_COUNT; ++i)
    {
        //-- clamp size to maximum allowed count.
        uint size = uint( m_staticGPUDecals[i].size() + m_dynamicGPUDecals[i].size() );
        size      = Math::clamp<uint>(0, size, g_maxVisibleDecals - offset);

        m_drawData[i + 1].m_offset = offset;
        m_drawData[i + 1].m_size   = size;

        m_drawData[0].m_size += size;

        offset += size;
    }

    //-- update watcher.
    g_curVisibleDecals = m_drawData[0].m_size;
}

//--------------------------------------------------------------------------------------------------
float DecalsManager::calcFadeValue(const Decal& decal) const
{
    //-- calculate fade distance.
    const Vector3& camPos      = Moo::rc().invView().applyToOrigin();
    float          distSquared = ((decal.m_pos - camPos) * Moo::rc().lodZoomFactor()).lengthSquared();
    
    return Math::clamp(0.0f, 1.0f - (distSquared - m_internalFadeRange.x) * m_internalFadeRange.z, 1.0f);
}

//--------------------------------------------------------------------------------------------------
DecalsManager::GPUDecal DecalsManager::convert2GPU(const Decal& decal, float blend) const
{
    BW_GUARD;

    const Rect2D& r1 = decal.m_map1->m_rect;
    const Rect2D& r2 = (decal.m_map2) ? decal.m_map2->m_rect : Rect2D();

    Vector2 map1TC(
        (float)(r1.m_right  - r1.m_left) + r1.m_left * m_staticAtlasSize.z,
        (float)(r1.m_bottom - r1.m_top)  + r1.m_top  * m_staticAtlasSize.w
        );

    Vector2 map2TC(
        (float)(r2.m_right  - r2.m_left) + r2.m_left * m_staticAtlasSize.z,
        (float)(r2.m_bottom - r2.m_top)  + r2.m_top  * m_staticAtlasSize.w
        );

    //-- prepare gpu decal.
    GPUDecal gpuDecal;
    gpuDecal.m_dir   = Vector4(decal.m_dir * (blend + 0.5f),                map1TC.x);
    gpuDecal.m_pos   = Vector4(decal.m_pos,                                 map1TC.y);
    gpuDecal.m_up    = Vector4(decal.m_up * (decal.m_influenceType + 0.5f), map2TC.x);
    gpuDecal.m_scale = Vector4(decal.m_scale,                               map2TC.y);

    return gpuDecal;
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::uploadInstancingBuffer()
{
    BW_GUARD;
    MF_ASSERT(m_drawData[0].m_size <= g_maxVisibleDecals);

    if (m_drawData[0].m_size)
    {
        Moo::VertexLock<GPUDecal> vl(
            m_instancedVB, 0, m_drawData[0].m_size * sizeof(GPUDecal), D3DLOCK_DISCARD
            );

        if (vl)
        {
            for (uint i = 0, count = 0; i < Decal::MATERIAL_COUNT; ++i)
            {
                for (uint j = 0; j < m_staticGPUDecals[i].size() && count < g_maxVisibleDecals; ++j)
                {
                    vl[count++] = m_staticGPUDecals[i][j];
                }

                for (uint j = 0; j < m_dynamicGPUDecals[i].size() && count < g_maxVisibleDecals; ++j)
                {
                    vl[count++] = m_dynamicGPUDecals[i][j];
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::draw()
{
    BW_GUARD;

    BW_SCOPED_DOG_WATCHER("deferred decal draw");
    BW_SCOPED_RENDER_PERF_MARKER("Deferred Decals");

    //-- 1. prepare visible decals.
    prepareVisibleDecals();

    //-- earlier exit in case if we don't have decals to draw.
    if (!m_drawData[0].m_size)
        return;

    //-- 2. upload them to GPU.
    uploadInstancingBuffer();

    //-- 3. setup common properties of the material.
    ID3DXEffect* effect = m_materials[0]->pEffect()->pEffect();
    {
        effect->SetTexture("g_atlasMap",    m_staticAtlasMap->pTexture());
        effect->SetVector ("g_atlasSize",   &m_staticAtlasSize);
        effect->CommitChanges();
    }

    //-- 4. set render targets.
    if (Moo::rc().pushRenderTarget())
    {
        // COLORWRITEENABLE can be changed by the materials that the decals
        // use. Some appear to write only to stencil for example.
        Moo::rc().pushRenderState(D3DRS_COLORWRITEENABLE);

        //-- Notice that we bind on the output 2 render target but in the shader based on the desired
        //-- material we can turn off writing into either of them.
        Moo::rc().setRenderTarget(0, Renderer::instance().pipeline()->gbufferSurface(2));
        Moo::rc().setRenderTarget(1, Renderer::instance().pipeline()->gbufferSurface(1));

        m_cube->instancingStream(&m_instancedVB);

        //-- 5. render decals with different materials.
        for (uint i = 0; i < Decal::MATERIAL_COUNT + 1; ++i)
        {
            const Moo::EffectMaterial* pMat = NULL;
            if(m_debugDrawEnabled && i > 0)
                pMat = m_debug_materials[m_debugRenderType].get();
            else
                pMat = m_materials[i].get();

            const Moo::EffectMaterial& material = *pMat;

            const DrawData&       drawData = m_drawData[i];

            if (drawData.m_size && material.begin())
            {
                for (uint32 j = 0; j < material.numPasses(); ++j)
                {
                    material.beginPass(j);
                    m_cube->justDrawPrimitivesInstanced(drawData.m_offset, drawData.m_size);
                    material.endPass();
                }
                material.end();
            }
        }

        //-- restore state.
        Moo::rc().popRenderState();
        Moo::rc().popRenderTarget();
    }
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::setQualityOption(int option)
{
    BW_GUARD;
    MF_ASSERT(option >= 0 && option < (int)m_qualityOptions.size());

    m_activeQualityOption = option;

    //--
    setFadeRange(m_fadeRange.x, m_fadeRange.y);
}

//--------------------------------------------------------------------------------------------------
int32 DecalsManager::createStaticDecal(const Decal::Desc& desc)
{
    BW_GUARD;

    //--
    bool requireMap2 = (desc.m_materialType != Decal::MATERIAL_DIFFUSE);

    //-- try to add textures to the texture atlas.
    TextureAtlas::SubTexturePtr map1 = (desc.m_map1 != "") ?
        m_staticAtlasMap->subTexture( desc.m_map1, desc.m_map1Src ) : 0;
    TextureAtlas::SubTexturePtr map2 = (requireMap2 && desc.m_map2 != "") ?
        m_staticAtlasMap->subTexture( desc.m_map2, desc.m_map2Src ) : 0;

#ifndef EDITOR_ENABLED
    if (!map1 || (requireMap2 && !map2))
    {
        return -1;
    }
#endif

    //--
    Decal decal;

    //-- retrieve decomposed decal's transformation.
    decomposeMatrix(decal.m_pos, decal.m_dir, decal.m_up, decal.m_scale, desc.m_transform);

    //--
    decal.m_map1            = map1;
    decal.m_map2            = map2;
    decal.m_priority        = desc.m_priority;
    decal.m_materialType    = desc.m_materialType;
    decal.m_influenceType   = desc.m_influenceType;

    //--
    int32 guid = m_staticDecals.nextGUID();
    m_staticDecals.insert(guid, decal);

#ifdef EDITOR_ENABLED
    if (!map1 || requireMap2 && !map2)
    {
        return -1;
    }
#endif

    return guid;
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::updateStaticDecal(int32 idx, const Decal::Desc& desc)
{
    BW_GUARD;
    MF_ASSERT(idx != -1);
    MF_ASSERT(m_staticDecals.size() > uint32(idx));

    //--
    Decal& decal = m_staticDecals.get(idx);

    //--
    bool requireMap2 = desc.m_materialType != Decal::MATERIAL_DIFFUSE;

    //-- try to add textures to the texture atlas.
#ifdef EDITOR_ENABLED
    TextureAtlas::SubTexturePtr map1 = (desc.m_map1 != "") ?
        m_staticAtlasMap->subTexture( desc.m_map1, desc.m_map1Src ) : 0;
    TextureAtlas::SubTexturePtr map2 = (requireMap2 && desc.m_map2 != "") ?
        m_staticAtlasMap->subTexture( desc.m_map2, desc.m_map2Src ) : 0;
#else
    TextureAtlas::SubTexturePtr map1 =
        m_staticAtlasMap->subTexture( desc.m_map1, desc.m_map1Src );
    TextureAtlas::SubTexturePtr map2 = requireMap2 ?
        m_staticAtlasMap->subTexture( desc.m_map2, desc.m_map2Src ) : 0;
#endif

    //-- update texture cache and retrieve sub-texture indices.
    decal.m_map1 = map1;
    decal.m_map2 = map2;        

    //-- retrieve decomposed decal's transformation.
    decomposeMatrix(decal.m_pos, decal.m_dir, decal.m_up, decal.m_scale, desc.m_transform);

    //--
    decal.m_priority        = desc.m_priority;
    decal.m_materialType    = desc.m_materialType;
    decal.m_influenceType   = desc.m_influenceType;
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::updateStaticDecal(int32 idx, const Matrix& transform)
{
    BW_GUARD;
    MF_ASSERT(idx != -1);
    MF_ASSERT(m_staticDecals.size() > uint32(idx)); 

    Decal& decal = m_staticDecals.get(idx);

    decomposeMatrix(decal.m_pos, decal.m_dir, decal.m_up, decal.m_scale, transform);
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::removeStaticDecal(int32 idx)
{
    BW_GUARD;
    MF_ASSERT(idx != -1);
    MF_ASSERT(m_staticDecals.size() > uint32(idx));

    m_staticDecals.remove(idx);
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::createDynamicDecal(const Decal::Desc& desc, float lifeTime, float startAlpha)
{
    BW_GUARD;

    DynamicDecal decal;

    //-- update texture cache and retrieve sub-texture indices.
    decal.m_map1 = m_staticAtlasMap->subTexture(desc.m_map1, desc.m_map1Src, true);
    decal.m_map2 = (desc.m_materialType != Decal::MATERIAL_DIFFUSE) ? m_staticAtlasMap->subTexture(desc.m_map2, desc.m_map2Src, true) : NULL;

    //-- retrieve decomposed decal's transformation.
    decomposeMatrix(decal.m_pos, decal.m_dir, decal.m_up, decal.m_scale, desc.m_transform);

    decal.m_priority        = desc.m_priority;
    decal.m_materialType    = desc.m_materialType;
    decal.m_influenceType   = desc.m_influenceType;
    decal.m_lifeTime        = lifeTime;
    decal.m_creationTime    = m_time;
    decal.m_worldBounds.setBounds(Vector3(-0.5f, -0.5f, -0.5f), Vector3(+0.5f, +0.5f, +0.5f));
    decal.m_worldBounds.transformBy(desc.m_transform);
    decal.m_startAlpha = startAlpha;

    m_dynamicDecals.add(decal);
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::clearDynamicDecals()
{
    m_dynamicDecals.clear();
}

//--------------------------------------------------------------------------------------------------
bool DecalsManager::recreateForD3DExDevice() const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::createUnmanagedObjects()
{
    BW_GUARD;

    bool success = true;

    //-- create buffer for the instancing drawing.
    //-- ToDo: reconsider memory usage. Currently is float4x4 * 512 -> 64 * 512 -> 32kb
    success &= SUCCEEDED(m_instancedVB.create(
        sizeof(GPUDecal) * g_maxVisibleDecals, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT,
        "vertex buffer/decal instancing VB"
        ));

    //--
    m_staticAtlasMap->createUnmanagedObjects();

    MF_ASSERT(success && "Can't create instancing buffer for decal pass.");
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::deleteUnmanagedObjects()
{
    BW_GUARD;

    //--
    m_staticAtlasMap->deleteUnmanagedObjects();

    m_instancedVB.release();
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::createManagedObjects()
{
    BW_GUARD;

    bool success = true;

    //-- create materials.
    const char* effectName   = "shaders/std_effects/deferred_decal.fx";

    //-- Warning: should be in sync with EMaterialType enumeration.
    const char* techniques[] = {
        "STENCIL",
        "DIFFUSE",
        "BUMP",
        "PARALLAX",
        "ENV_MAPPING"
    };

    for (uint i = 0; i < Decal::MATERIAL_COUNT + 1; ++i)
    {
        m_materials[i] = new Moo::EffectMaterial();
        success &= m_materials[i]->initFromEffect(effectName);
        m_materials[i]->hTechnique(techniques[i]);
    }

    // initialise debug materials
    const char* debug_techniques[] = {
        "DEBUG_OVERDRAW"
    };

    for (uint i = 0; i < DEBUG_RENDER_COUNT; ++i)
    {
        m_debug_materials[i] = new Moo::EffectMaterial();
        success &= m_debug_materials[i]->initFromEffect(effectName);
        m_debug_materials[i]->hTechnique(debug_techniques[i]);
    }

    //-- create cube mesh.
    m_cube   = Moo::VisualManager::instance()->get("system/models/fx_unite_cube.visual");
    success &= m_cube.exists();

    //-- create texture atlases.
    m_staticAtlasMap = new TextureAtlas(g_staticAtlasSize, g_staticAtlasSize, "static_decal_atlas_map");

    m_staticAtlasSize.set(
        (float)m_staticAtlasMap->width(), (float)m_staticAtlasMap->height(),
        1.0f / (float)m_staticAtlasMap->width(), 1.0f / (float)m_staticAtlasMap->height()
        );

    MF_ASSERT_DEV(success && "Not all system resources have been loaded correctly.");
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::deleteManagedObjects()
{
    BW_GUARD;

    //--
    m_staticAtlasMap = NULL;

    //--
    for (uint i = 0; i < Decal::MATERIAL_COUNT + 1; ++i)
    {
        m_materials[i] = NULL;
    }

    //--
    for (uint i = 0; i < DEBUG_RENDER_COUNT; ++i)
    {
        m_debug_materials[i] = NULL;
    }

    //--
    m_cube = NULL;
}

//--------------------------------------------------------------------------------------------------
bool DecalsManager::showStaticDecalsAtlas() const
{
    BW_GUARD;

    return g_showStaticAtlas;
}

//--------------------------------------------------------------------------------------------------
DX::BaseTexture* DecalsManager::staticDecalAtlas() const
{
    BW_GUARD;
    MF_ASSERT(m_staticAtlasMap.get());

    return m_staticAtlasMap->pTexture();
}

#ifdef EDITOR_ENABLED

//--------------------------------------------------------------------------------------------------
void DecalsManager::showStaticDecalsAtlas(bool flag)
{
    BW_GUARD;

    g_showStaticAtlas = flag;
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::getTexturesUsed( uint idx, BW::map<void*, int>& textures )
{
    BW_GUARD;
    MF_ASSERT(idx != -1);

    const Decal& decal = m_staticDecals.get(idx);

    //--
    const TextureAtlas::SubTexturePtr& pTex1 = decal.m_map1;
#ifdef EDITOR_ENABLED
    if (!pTex1)
    {
        return;
    }
#endif

    textures.insert(std::make_pair(pTex1.get(), 
        (pTex1->m_rect.m_right - pTex1->m_rect.m_left + 1) * 
        (pTex1->m_rect.m_bottom - pTex1->m_rect.m_top + 1))
        );

    //-- Note: some decal's materials may not require map2 texture to be available. For example
    //--       MATERIAL_DIFFUSE uses only map1 texture.
    if (decal.m_materialType != Decal::MATERIAL_DIFFUSE)
    {
        const TextureAtlas::SubTexturePtr& pTex2 = decal.m_map2;

#ifdef EDITOR_ENABLED
        if (!pTex2)
        {
            return;
        }
#endif

        textures.insert(std::make_pair(pTex2.get(), 
            (pTex2->m_rect.m_right - pTex2->m_rect.m_left + 1) * 
            (pTex2->m_rect.m_bottom - pTex2->m_rect.m_top + 1))
            );
    }
}

//--------------------------------------------------------------------------------------------------
void DecalsManager::getVerticesUsed( BW::map<void*, int>& vertices )
{
    BW_GUARD;

    vertices.insert(std::make_pair(&m_instancedVB, m_instancedVB.size()));
}

//--------------------------------------------------------------------------------------------------
float DecalsManager::getMap1AspectRatio( uint idx )
{
    BW_GUARD;
    MF_ASSERT(idx != -1);

    TextureAtlas::SubTexture* pTex1 = m_staticDecals.get(idx).m_map1.get();

#ifdef EDITOR_ENABLED
    if (!pTex1)
    {
        return -1.0f;
    }
#endif

    return  pTex1->m_loaded ? ((float)(pTex1->m_rect.m_right - pTex1->m_rect.m_left + 1) / 
        (float)(pTex1->m_rect.m_bottom - pTex1->m_rect.m_top + 1)) : -1.0f ;
}

//--------------------------------------------------------------------------------------------------
size_t DecalsManager::numVisibleDecals()
{
    BW_GUARD;

    return g_curVisibleDecals;
}

//--------------------------------------------------------------------------------------------------
size_t DecalsManager::numTotalDecals()
{
    return g_curTotalDecals;
}

void DecalsManager::setVirtualOffset(const Vector3 &offset)
{
    viewSpaceOffset = offset;
}

bool DecalsManager::getTexturesValid( uint idx )
{
    BW_GUARD;
    MF_ASSERT( idx != -1 );
    const Decal& decal = m_staticDecals.get( idx );

    return decal.m_map1 &&
        (decal.m_materialType == Decal::MATERIAL_DIFFUSE || decal.m_map2);
}
#endif

BW_END_NAMESPACE
