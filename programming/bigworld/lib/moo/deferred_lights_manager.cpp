#include "pch.hpp"

#include "deferred_lights_manager.hpp"
#include "renderer.hpp"
#include "render_event.hpp"
#include "moo/moo_utilities.hpp"
#include "moo/fullscreen_quad.hpp"
#include "romp/ssao_support.hpp"
#include "moo/effect_material.hpp"
#include "moo/visual_manager.hpp"
#include "moo/effect_visual_context.hpp"
#include "cstdmf/dogwatch.hpp"

//-- ToDo: reconsider intercommunication between Shadows and Deferred Shading Pipeline.
#include "shadow_manager.hpp"
#include "dynamic_shadow.hpp"

BW_BEGIN_NAMESPACE

using namespace Moo;

//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
    //----------------------------------------------------------------------------------------------
    const uint32 g_instanceCount    = 1000;
    const float  g_LODRadiusError   = 0.5f;

    //-- ToDo: improve. Do sorting of indices or pointer on objects instead of the real objects.
    //----------------------------------------------------------------------------------------------
    class OmniLightSorter
    {
    public:
        OmniLightSorter(const Vector3& camPos) : m_camPos(camPos) { }
        bool operator()(const Moo::OmniLight::GPU& lft, const Moo::OmniLight::GPU& rht)
        {
            return lft.m_pos.w > rht.m_pos.w;
        }

    private:
        Vector3 m_camPos;
    };
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.


//-- define singleton.
BW_SINGLETON_STORAGE(LightsManager)


//--------------------------------------------------------------------------------------------------
LightsManager::LightsManager()
{

}

//--------------------------------------------------------------------------------------------------
LightsManager::~LightsManager()
{

}

//--------------------------------------------------------------------------------------------------
bool LightsManager::init()
{
    BW_GUARD;
    return true;
}

//--------------------------------------------------------------------------------------------------
void LightsManager::tick(float dt)
{
    BW_GUARD;

    m_lights.clear();
    m_gpuOmnisFallbacks.clear();
    m_gpuOmnisInstanced.clear();
    m_gpuSpotsFallbacks.clear();
    m_gpuSpotsInstanced.clear();
}

//--------------------------------------------------------------------------------------------------
void LightsManager::draw()
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("lighting")
    BW_SCOPED_RENDER_PERF_MARKER("Deferred Lighting");

    IRendererPipeline* rp = Renderer::instance().pipeline();

    //-- 1. prepare lights.
    prepareLights();
    
    //-- 2. draw sun light with ambient and shadow term.
    drawSunLight();

    //-- 3. omni lights.
    drawOmnis();

    //-- 4. spot lights.
    drawSpots();
}

//--------------------------------------------------------------------------------------------------
void LightsManager::drawSunLight()
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("sun");
    BW_SCOPED_RENDER_PERF_MARKER("Sun");

    //-- directional lights and ambient term.
    //-- Note: here we make assumptions that we always will have one directional light, so
    //--       we can apply some basic optimization and do ambient and directional light in
    //--       one pass. This behavior may be re-writed if needed.
    m_material->hTechnique("SUN");

    SSAOSupport* ssao = rp().ssaoSupport();
    ShadowManager* ds = rp().dynamicShadow();
    
    ID3DXEffect* effect = m_material->pEffect()->pEffect();
    {
        //-- shadows
        // effect->SetBool("g_enableShadows", ds->isShadowsEnabled() );
        // effect->SetFloat("g_dynamicShadowsDist", ds->dynamicShadowsDistance());

        //-- ssao

        effect->SetBool   ("g_enableSSAO", ssao->enable());
        effect->SetTexture("g_texSSAO", ssao->screenSpaceAmbienOcclusionMap());

        //-- commit

        effect->CommitChanges();
    }

    Moo::rc().fsQuad().draw(*m_material.get());
}


//--------------------------------------------------------------------------------------------------
void LightsManager::drawSpots()
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("spots");
    BW_SCOPED_RENDER_PERF_MARKER("Spots");

    //-- ToDo: implement.
    
    m_material->hTechnique("SPOT_FALLBACK");
    for (uint i = 0; i < m_gpuSpotsFallbacks.size(); ++i)
    {
        ID3DXEffect* effect = m_material->pEffect()->pEffect();
        {
            effect->SetValue("g_spotLight", &m_gpuSpotsFallbacks[i], sizeof(Moo::SpotLight::GPU));
            effect->CommitChanges();
        }
        Moo::rc().fsQuad().draw(*m_material.get());
    }
}

//--------------------------------------------------------------------------------------------------
void LightsManager::drawOmnis()
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("omnis");
    BW_SCOPED_RENDER_PERF_MARKER("Omnis");

    const UINT numInstanced = static_cast<UINT>(m_gpuOmnisInstanced.size());
    const size_t numFallbacks = m_gpuOmnisFallbacks.size();

    //-- set common constants.
    ID3DXEffect* effect = m_material->pEffect()->pEffect();
    {
        effect->SetFloat("g_omniLightLODRadiusError", g_LODRadiusError);
        effect->CommitChanges();
    }

    //-- draw instanced omni lights.
    if (numInstanced)
    {
        BW_SCOPED_DOG_WATCHER("instanced")

        //-- fill instance data vertex buffer.
        {
            BW_SCOPED_DOG_WATCHER("fill instancing buffer")
            
            Moo::VertexLock<Moo::OmniLight::GPU> vl(m_instancedVB, 0,
                numInstanced * 64, D3DLOCK_DISCARD);
            if (vl)
            {
                for (uint i = 0; i < numInstanced && i < g_instanceCount; ++i)
                {
                    vl[i] = m_gpuOmnisInstanced[i];
                }
            }
        }

        m_sphere->instancingStream(&m_instancedVB);

        //-- 1. exclude from processing pixels behind the light geometry.
        //--    Clear stencil buffer with zero values and then draw back-faced vertices 
        //--    with z-test GREAT_EQUAL and mark passed pixels in the stencil buffer. 
        //-- 2. exclude from processing pixels in front of the light geometry.
        //--    Draw front-faced vertices with z-test LESS_EQUAL and stencil test.
        m_material->hTechnique("OMNI_COMMON");
        if (m_material->begin())
        {
            for (uint32 i = 0; i < m_material->numPasses(); ++i)
            {
                m_material->beginPass(i);
                m_sphere->justDrawPrimitivesInstanced(0, numInstanced);
                m_material->endPass();
            }
            m_material->end();
        }
    }

    //-- draw omni lights which intersect camera near plane. So for this kind of omni lights
    //-- two passes optimization doesn't work, so we disable it.
    if (numFallbacks)
    {
        BW_SCOPED_DOG_WATCHER("fallbacks")

        m_material->hTechnique("OMNI_FALLBACK");
        if (m_material->begin())
        {
            for (uint32 i = 0; i < m_material->numPasses(); ++i)
            {
                m_material->beginPass(i);

                for (uint i = 0; i < numFallbacks; ++i)
                {
                    ID3DXEffect* effect = m_material->pEffect()->pEffect();
                    {
                        effect->SetValue("g_omniLight", &m_gpuOmnisFallbacks[i], sizeof(Moo::OmniLight::GPU));
                        effect->CommitChanges();
                    }

                    m_sphere->justDrawPrimitives();
                }

                m_material->endPass();
            }

            m_material->end();
        }
    }
}

//--------------------------------------------------------------------------------------------------
void LightsManager::gatherVisibleLights(const Moo::LightContainerPtr& lights)
{
    BW_GUARD;

    m_lights.addToSelfOnlyVisibleLights(lights, Moo::rc().viewProjection());
}

//--------------------------------------------------------------------------------------------------
void LightsManager::prepareLights()
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("prepare lights")

    float   camNearDist = Moo::rc().camera().nearPlane() / cosf(Moo::rc().camera().fov() * 0.5f);
    Vector3 camWorldPos = Moo::rc().invView().applyToOrigin();

    //-- collect all the omni light for the current chunk. 
    const OmniLightVector& omnis = m_lights.omnis();
    for (uint i = 0; i < omnis.size(); ++i)
    {
        const Moo::OmniLightPtr& iOmni = omnis[i];
        Moo::OmniLight::GPU      oOmni = iOmni->gpu();

        //-- calculate square distance to the camera and decide which one algo will be used for
        //-- drawing this light.
        oOmni.m_pos.w = (iOmni->worldPosition() - camWorldPos).lengthSquared();

        //-- find distance at which we change rendering optimization algo.
        float fallbackDist        = (camNearDist + iOmni->outerRadius() + 0.25f) + g_LODRadiusError;
        float squaredFallbackDist = fallbackDist * fallbackDist;
    
        if (oOmni.m_pos.w > squaredFallbackDist)
        {
            m_gpuOmnisInstanced.push_back(oOmni);
        }
        else
        {
            m_gpuOmnisFallbacks.push_back(oOmni);
        }
    }

    //-- collect all the spot light for the current chunk.
    const SpotLightVector& spots = m_lights.spots();
    for (uint i = 0; i < spots.size(); ++i)
    {
        const Moo::SpotLightPtr& iSpot = spots[i];
        Moo::SpotLight::GPU     oSpot = iSpot->gpu();

        m_gpuSpotsFallbacks.push_back(oSpot);
    }
}

//--------------------------------------------------------------------------------------------------
void LightsManager::sortLights()
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("sort lights")

    //-- sort omni and spot lights from back to front relative to the camera.
    //-- That helps reduce overdraw.
    std::sort(
        m_gpuOmnisInstanced.begin(), m_gpuOmnisInstanced.end(),
        OmniLightSorter(Moo::rc().invView().applyToOrigin())
        );
}

//--------------------------------------------------------------------------------------------------
bool LightsManager::recreateForD3DExDevice() const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
void LightsManager::createUnmanagedObjects()
{
    BW_GUARD;

    bool success = true;

    //-- create buffer for the light instancing drawing.
    //-- ToDo: reconsider memory usage.
    //--       currently is float4x4 * 1000 -> 64 * 1000 -> 64.0kb
    success &= SUCCEEDED(m_instancedVB.create(
        64 * g_instanceCount, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT,
        "vertex buffer/light instancing VB"
        ));

    MF_ASSERT(success && "Can't create instancing buffer for lighting pass.");
}

//--------------------------------------------------------------------------------------------------
void LightsManager::deleteUnmanagedObjects()
{
    BW_GUARD;

    m_instancedVB.release();
}

//--------------------------------------------------------------------------------------------------
void LightsManager::createManagedObjects()
{
    BW_GUARD;

    bool success = true;
    success &= createEffect(m_material, "shaders/std_effects/resolve_lighting.fx");

    m_cone   = Moo::VisualManager::instance()->get("system/models/fx_unite_cone.visual");
    m_sphere = Moo::VisualManager::instance()->get("system/models/fx_unit_sphere.visual");

    success &= m_cone.exists() && m_sphere.exists();

    MF_ASSERT_DEV(success && "Not all system resources were loaded correctly.");
}

//--------------------------------------------------------------------------------------------------
void LightsManager::deleteManagedObjects()
{
    BW_GUARD;

    m_material = NULL;
    m_cone     = NULL;
    m_sphere   = NULL;
}

BW_END_NAMESPACE
