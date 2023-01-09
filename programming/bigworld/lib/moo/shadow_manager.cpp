#include "pch.hpp"

#include "shadow_manager.hpp"

#include "cstdmf/dogwatch.hpp"
#include "cstdmf/watcher.hpp"
#include "resmgr/auto_config.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_umbra.hpp"
#include <string>

#include "pyscript/script.hpp"
#include "cstdmf/singleton.hpp"
#include "resmgr/datasection.hpp"
#include "romp/time_of_day.hpp"

#include <string>
#include "pyscript/script.hpp"
#include "cstdmf/singleton.hpp"
#include "resmgr/datasection.hpp"

#include "dynamic_shadow.hpp"
#include "semi_dynamic_shadow.hpp"
#include "moo/moo_utilities.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/nvapi_wrapper.hpp"
#include "moo/render_event.hpp"
#include "render_target.hpp"

#include "space/client_space.hpp"
#include "space/deprecated_space_helpers.hpp"

BW_BEGIN_NAMESPACE

//--------------------------------------------------------------------------------------------------
// Singleton storage
//--------------------------------------------------------------------------------------------------

BW_SINGLETON_STORAGE(Moo::ShadowManager)

//--------------------------------------------------------------------------------------------------
// Flora shader names
//--------------------------------------------------------------------------------------------------


using Moo::ShadowManager;

static AutoConfigString s_floraMFM( "environment/FloraWithShadowMaterial" );

//-- Shadow resolve buffer auto-constant.
//--------------------------------------------------------------------------------------------------
class ShadowScreenSpaceMap : public Moo::EffectConstantValue
{
public:
    ShadowScreenSpaceMap( const ShadowManager& sm ) : m_sm( sm )
    {
    }

    bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
    {
        BW_GUARD;
        pEffect->SetTexture( constantHandle, m_sm.getResolveBuffer() );
        return true;
    }

private:
    const Moo::ShadowManager& m_sm;
};

//-- Shadow blend-distance buffer auto-constant.
//--------------------------------------------------------------------------------------------------
class ShadowBlendParams : public Moo::EffectConstantValue
{
public:
    ShadowBlendParams( const ShadowManager& sm ) : m_sm( sm )
    {
    }

    bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
    {
        BW_GUARD;
        
        // x -- distance from the camera, which is visible from the
        // Semi-dynamic shadow.
        // y -- distance from x, for which, Semi-dynamic shadow will be mixed
        // with the dynamic.
        // z -- Coefficient of mixing, how to take into account the dynamic
        // shadows semi-dynamic.
        Vector4 blend( 
            m_sm.get_semiViewDistanceFrom(), 
            m_sm.get_dynamicViewDistanceTo() - m_sm.get_semiViewDistanceFrom(), 
            m_sm.get_semiBlendFactor(), 
            0 
        );

        pEffect->SetVector( constantHandle, &blend );
        return true;
    }

private:
    const ShadowManager& m_sm;
};

//-- Shadow blend-distance buffer auto-constant.
//--------------------------------------------------------------------------------------------------
class IsShadowsEnabled : public Moo::EffectConstantValue
{
public:
    IsShadowsEnabled( const ShadowManager& sm ) : m_sm( sm )
    {
    }

    bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
    {
        BW_GUARD;
        pEffect->SetBool( constantHandle, m_sm.isShadowsEnabled() );
        return true;
    }

private:
    const ShadowManager& m_sm;
};

namespace Moo {
    //--------------------------------------------------------------------------------------------------
    // ShadowManager
    //--------------------------------------------------------------------------------------------------

    ShadowManager::ShadowManager()

        // default constructor values should match values of the "off" SHADOWS_QUALITY option
        : m_pShadowsWrapper(NULL)
        , m_semiDynamicShadows(NULL)
        , m_debugMode(false)
        , m_isShadowQualityChanged(false)

        , m_dynamicEnabled(false)
        , m_dynamicTerrainShadowMapSize(1024)
        , m_dynamicShadowMapSize(1024)
        , m_dynamicViewDistanceTo(0.f)

        , m_semiEnabled(false)
        , m_semiViewDistanceFrom(0.f)
        , m_semiCoveredAreaSize(16)
        , m_semiShadowMapSize(4096)
        , m_semiBlendFactor(0.f)
        , m_shadowQualityIndex(0)
        , m_enabledManager(true)
    {
    }

    //--------------------------------------------------------------------------------------------------
    ShadowManager::~ShadowManager()
    {
    }

    //--------------------------------------------------------------------------------------------------
    void ShadowManager::createManagedObjects()
    {
        BW_GUARD;

        //-- register shared auto-constant.
        Moo::rc().effectVisualContext().registerAutoConstant(
            Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "ShadowScreenSpaceMap",
            new ShadowScreenSpaceMap(*this)
        );

        //-- register shared auto-constant.
        Moo::rc().effectVisualContext().registerAutoConstant(
            Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "ShadowBlendParams",
            new ShadowBlendParams(*this)
        );

        //-- register shared auto-constant.
        Moo::rc().effectVisualContext().registerAutoConstant(
            Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "IsShadowsEnabled",
            new IsShadowsEnabled(*this)
        );
    }

    //--------------------------------------------------------------------------------------------------
    void ShadowManager::deleteManagedObjects()
    {
        BW_GUARD;

        //-- unregister shared auto-constant.
        Moo::rc().effectVisualContext().unregisterAutoConstant(
            Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "ShadowScreenSpaceMap"
        );

        //-- unregister shared auto-constant.
        Moo::rc().effectVisualContext().unregisterAutoConstant(
            Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "ShadowBlendParams"
        );

        //-- unregister shared auto-constant.
        Moo::rc().effectVisualContext().unregisterAutoConstant(
            Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "IsShadowsEnabled"
        );
    }

    //--------------------------------------------------------------------------------------------------
    void ShadowManager::createUnmanagedObjects()
    {
        m_enabledManager = true;

        m_enabledManager &= createRenderTarget(
            m_screenSpaceShadowMap,
            static_cast<uint>(Moo::rc().screenWidth()),
            static_cast<uint>(Moo::rc().screenHeight()),
            D3DFMT_A8R8G8B8,
            "SSSM", true, false
        );

        // Force the screen space shadow map to be created now so that we
        // have the correct screen related depth and stencil set.
        m_screenSpaceShadowMap->pTexture();

#if !defined( _RELEASE )
        // Because this is the screen space shadow map, make sure the depthStencil
        // that is set is the same size as the screen space shadow map. 
        // If shadows aren't rendering at all it could be that the wrong 
        // depthStencil is set at this point.
        IDirect3DSurface9* pDepthSurface;
        HRESULT hr = Moo::rc().device()->GetDepthStencilSurface(&pDepthSurface);
        MF_ASSERT(hr == S_OK);

        D3DSURFACE_DESC depthDesc;
        pDepthSurface->GetDesc(&depthDesc);
        pDepthSurface->Release();

        MF_ASSERT(depthDesc.Width == m_screenSpaceShadowMap->width());
        MF_ASSERT(depthDesc.Height == m_screenSpaceShadowMap->height());
#endif
    }

    //--------------------------------------------------------------------------------------------------
    bool ShadowManager::recreateForD3DExDevice() const
    {
        return true;
    }

    //--------------------------------------------------------------------------------------------------
    void ShadowManager::deleteUnmanagedObjects()
    {
        m_screenSpaceShadowMap = NULL;
    }

    //--------------------------------------------------------------------------------------------------
    bool ShadowManager::init(DataSectionPtr section)
    {
        BW_GUARD;

#define DECLARE_WATCHER(var_name)               \
    MF_WATCH("Render/Shadows/"#var_name, *this, \
        &ShadowManager::get_##var_name, \
        &ShadowManager::set_##var_name)

        DECLARE_WATCHER(debugMode);

        DECLARE_WATCHER(dynamicEnabled);
        DECLARE_WATCHER(dynamicTerrainShadowMapSize);
        DECLARE_WATCHER(dynamicShadowMapSize);
        DECLARE_WATCHER(dynamicViewDistanceTo);

        DECLARE_WATCHER(semiEnabled);
        DECLARE_WATCHER(semiViewDistanceFrom);
        DECLARE_WATCHER(semiCoveredAreaSize);
        DECLARE_WATCHER(semiShadowMapSize);
        DECLARE_WATCHER(semiBlendFactor);

#undef DECLARE_WATCHER

        MF_WATCH("Render/Shadows/blendDistance", *this, &ShadowManager::getBlendDistance);

        return true;
    }

    //--------------------------------------------------------------------------------------------------
    void ShadowManager::fini()
    {
        BW_GUARD;

        enableDynamic(false);
        enableSemi(false);
    }

    //--------------------------------------------------------------------------------------------------
    bool ShadowManager::isDynamicShadowsEnabled() const
    {
        return m_pShadowsWrapper != NULL;
    }

    //--------------------------------------------------------------------------------------------------
    bool ShadowManager::isSemiDynamicShadowsEnabled() const
    {
        return m_semiDynamicShadows.get() != NULL;
    }

    //--------------------------------------------------------------------------------------------------
    bool ShadowManager::isShadowsEnabled() const
    {
        return (isSemiDynamicShadowsEnabled() || isDynamicShadowsEnabled()) && m_enabledManager;
    }

    //--------------------------------------------------------------------------------------------------
    float ShadowManager::dynamicShadowsDistance() const
    {
        return m_dynamicViewDistanceTo;
    }

    //--------------------------------------------------------------------------------------------------
    void ShadowManager::tick(float dTime)
    {
        BW_GUARD;
        // enable / disable

        bool isShadowPosible = false;

        ClientSpacePtr cs = DeprecatedSpaceHelpers::cameraSpace();
        if (cs)
        {
            Vector3 lightDir = cs->enviro().timeOfDay()->lighting().mainLightDir();
            lightDir.normalise();
            isShadowPosible = lightDir.length() > 0.01f;
        }

        if (m_pShadowsWrapper)
        {
            if (!m_dynamicEnabled || !isShadowPosible || m_isShadowQualityChanged)
            {
                enableDynamic(false);
            }
        }
        else
        {
            if (m_dynamicEnabled && isShadowPosible)
            {
                enableDynamic(true);
            }
        }

        if (m_semiDynamicShadows.get())
        {
            if (!m_semiEnabled || !isShadowPosible || m_isShadowQualityChanged)
            {
                enableSemi(false);
            }
        }
        else
        {
            if (m_semiEnabled && isShadowPosible)
            {
                enableSemi(true);
            }
        }

        m_isShadowQualityChanged = false;

        // tick

        if (m_semiDynamicShadows.get())
        {
            m_semiDynamicShadows->tick(dTime);
        }
    }

    //--------------------------------------------------------------------------------------------------
    void ShadowManager::cast(Moo::DrawContext& shadowDrawContext)
    {
        BW_GUARD;
        BW_SCOPED_DOG_WATCHER("Shadows");
        BW_SCOPED_RENDER_PERF_MARKER("Cast Shadows");

        if (m_pShadowsWrapper)
        {
            m_pShadowsWrapper->castShadows(shadowDrawContext);
        }

        if (m_semiDynamicShadows.get())
        {
            m_semiDynamicShadows->cast(shadowDrawContext);
        }

        RenderItemsList::iterator itemIt = m_renderItems.begin();
        for (; itemIt != m_renderItems.end(); ++itemIt)
        {
            (*itemIt)->release();
        }

        m_renderItems.clear();
    }

    //--------------------------------------------------------------------------------------------------

    void ShadowManager::receive()
    {
        BW_GUARD;
        BW_SCOPED_RENDER_PERF_MARKER("Receive Shadows");

        if (!m_enabledManager)
            return;

        if (m_screenSpaceShadowMap.get())
        {
            if (m_screenSpaceShadowMap->push())
            {
                Moo::rc().device()->Clear(0, NULL, D3DCLEAR_TARGET, Moo::Colour(0, 0, 0, 0), 1, 0);

                if (m_pShadowsWrapper)
                {
                    m_pShadowsWrapper->receiveShadows();
                }

                if (m_semiDynamicShadows.get())
                {
                    m_semiDynamicShadows->receive();
                }

                m_screenSpaceShadowMap->pop();
            }
        }
    }

    //--------------------------------------------------------------------------------------------------
    DX::BaseTexture* ShadowManager::getResolveBuffer() const
    {
        return m_screenSpaceShadowMap->pTexture();
    }

    //--------------------------------------------------------------------------------------------------
    void ShadowManager::drawDebugInfo()
    {
        BW_GUARD;
        if (m_pShadowsWrapper)
        {
            m_pShadowsWrapper->drawDebugInfo();
        }

        if (m_semiDynamicShadows.get())
        {
            m_semiDynamicShadows->drawDebugInfo();
        }

        if (m_debugMode)
        {
            float w = Moo::rc().screenWidth();
            float h = Moo::rc().screenHeight();
            Vector2 widthHeight(300.f, 300.f);
            Vector2 topLeft = Vector2(w, widthHeight.y) - widthHeight;

            Moo::drawDebugTexturedRect(topLeft, topLeft + widthHeight, m_screenSpaceShadowMap->pTexture());
        }
    }

    //--------------------------------------------------------------------------------------------------

#define DECLARE_SHADOWS_MODULE_SELECTOR_ACESSORS(var_type, name)            \
                                                                            \
void                                                                        \
ShadowManager::set_##name(var_type new_val)                                 \
{                                                                           \
    var_type old_val = m_##name;                                            \
    m_##name = new_val;                                                     \
    if(m_pShadowsWrapper && m_##name != old_val)                            \
    {                                                                       \
        bool semiEnable = m_semiEnabled;                                    \
        enableDynamic(false);                                               \
        enableDynamic(true);                                                \
        enableSemi(false);                                                  \
        m_semiEnabled = semiEnable;                                         \
    }                                                                       \
}                                                                           \
                                                                            \
var_type                                                                    \
ShadowManager::get_##name() const                                           \
{                                                                           \
    return m_##name;                                                        \
}                                                                           \

    DECLARE_SHADOWS_MODULE_SELECTOR_ACESSORS(bool, debugMode);

    DECLARE_SHADOWS_MODULE_SELECTOR_ACESSORS(bool, dynamicEnabled);
    DECLARE_SHADOWS_MODULE_SELECTOR_ACESSORS(uint, dynamicTerrainShadowMapSize);
    DECLARE_SHADOWS_MODULE_SELECTOR_ACESSORS(uint, dynamicShadowMapSize);
    DECLARE_SHADOWS_MODULE_SELECTOR_ACESSORS(float, dynamicViewDistanceTo);

    DECLARE_SHADOWS_MODULE_SELECTOR_ACESSORS(bool, semiEnabled);
    DECLARE_SHADOWS_MODULE_SELECTOR_ACESSORS(float, semiViewDistanceFrom);
    DECLARE_SHADOWS_MODULE_SELECTOR_ACESSORS(uint, semiCoveredAreaSize);
    DECLARE_SHADOWS_MODULE_SELECTOR_ACESSORS(uint, semiShadowMapSize);
    DECLARE_SHADOWS_MODULE_SELECTOR_ACESSORS(float, semiBlendFactor);

#undef DECLARE_SHADOWS_MODULE_SELECTOR_ACESSORS

    //--------------------------------------------------------------------------------------------------
    void ShadowManager::setQualityOption(int optionIndex)
    {
        BW_GUARD;
        MF_ASSERT(optionIndex >= 0 && optionIndex <= QUALITY_TOTAL);

        //--
        struct QualityOption
        {
            bool    m_dynamicEnabled;
            bool    m_semiEnabled;
            float   m_semiViewDistanceFrom;
            float   m_dynamicViewDistanceTo;
            float   m_semiBlendFactor;
        };

        //-- should be in sync with graphic setting SHADOW_QUALITY
        const QualityOption options[QUALITY_TOTAL] =
        {
            { true,  true,  100.0f, 120.0f, 0.f }, //-- MAX
            { true,  true,   80.0f, 100.0f, 0.f }, //-- HIGH
            { true,  true,   50.0f,  70.0f, 0.f }, //-- MEDIUM
            { true,  true,   50.0f,  70.0f, 1.f }, //-- LOW
            { false, false,   0.0f,   0.0f, 0.f }, //-- OFF
            { true,  false, 100.0f, 120.0f, 0.f }  //-- EDITORS
        };

        //--
        const QualityOption& option = options[optionIndex];

        m_dynamicEnabled = option.m_dynamicEnabled;
        m_semiEnabled = option.m_semiEnabled;
        m_semiViewDistanceFrom = option.m_semiViewDistanceFrom;
        m_dynamicViewDistanceTo = option.m_dynamicViewDistanceTo;
        m_semiBlendFactor = option.m_semiBlendFactor;
        m_isShadowQualityChanged = true;
        m_shadowQualityIndex = optionIndex;
    }

#ifdef EDITOR_ENABLED
    void ShadowManager::setGenerationFinishedCallback(
        GenerationFinishedCallback callback)
    {
        if (m_semiDynamicShadows.get())
        {
            m_semiDynamicShadows->setGenerationFinishedCallback(callback);
        }
    }

    void ShadowManager::resetAll()
    {
        if (m_dynamicEnabled)
        {
            enableDynamic(false);
            enableDynamic(true);
        }

        if (m_semiEnabled)
        {
            enableSemi(false);
            enableSemi(true);
        }
    }
#endif

    //--------------------------------------------------------------------------------------------------
    void ShadowManager::enableDynamic(bool enabled)
    {
        BW_GUARD;

        if (enabled && !m_pShadowsWrapper)
        {
            m_pShadowsWrapper = new DynamicShadow(
                m_debugMode,
                m_dynamicTerrainShadowMapSize,
                m_dynamicShadowMapSize,
                m_dynamicViewDistanceTo,
                m_shadowQualityIndex
            );

#if UMBRA_ENABLE
            ChunkManager::instance().umbra()->shadowDistanceVisible(m_dynamicViewDistanceTo);
#endif
            m_pShadowsWrapper->enable(true);
        }

        if (!enabled && m_pShadowsWrapper) {
            delete m_pShadowsWrapper;
            m_pShadowsWrapper = NULL;
        }
    }

    //--------------------------------------------------------------------------------------------------
    void ShadowManager::enableSemi(bool enabled)
    {
        BW_GUARD;

        if (enabled && !m_semiDynamicShadows.get())
        {
            m_semiDynamicShadows.reset(
                new Moo::SemiDynamicShadow(
                    m_semiCoveredAreaSize,
                    m_semiShadowMapSize,
                    m_debugMode
                ));
        }

        if (!enabled && m_semiDynamicShadows.get())
        {
            m_semiDynamicShadows.reset(NULL);
        }
    }

    //--------------------------------------------------------------------------------------------------
    float ShadowManager::getBlendDistance() const
    {
        return m_dynamicViewDistanceTo - m_semiViewDistanceFrom;
    }

    //--------------------------------------------------------------------------------------------------
    void ShadowManager::addRenderItem(IShadowRenderItem* item)
    {
        m_renderItems.push_back(item);
    }

}

BW_END_NAMESPACE

//--------------------------------------------------------------------------------------------------
// End
//--------------------------------------------------------------------------------------------------