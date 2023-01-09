#pragma once

#include "cstdmf/singleton.hpp"
#include "cstdmf/smartpointer.hpp"
#include "moo/moo_dx.hpp"
#include "moo/device_callback.hpp"
#include "moo/forward_declarations.hpp"

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

//--------------------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------------------

class DataSection;
class Chunk;

typedef SmartPointer<DataSection> DataSectionPtr;

namespace Moo {
    
    class DrawContext;
    class DynamicShadow;
    class SemiDynamicShadow;

    //--------------------------------------------------------------------------------------------------
    // ShadowManager
    //--------------------------------------------------------------------------------------------------

    class IShadowRenderItem
    {
    public:
        virtual ~IShadowRenderItem() {};
        virtual void render(Moo::DrawContext& drawContext) = 0;
        virtual void release() = 0;
    };

    class ShadowManager : public Moo::DeviceCallback
    {
    public:

        enum Quality
        {
            QUALITY_MAX = 0,
            QUALITY_HIGH = 1,
            QUALITY_MEDIUM = 2,
            QUALITY_LOW = 3,
            QUALITY_OFF = 4,
            QUALITY_EDITORS = 5,
            QUALITY_TOTAL,
        };

        typedef BW::vector<IShadowRenderItem*> RenderItemsList;

        ShadowManager();
        ~ShadowManager();

        //-- Moo::DeviceCallback
        virtual bool recreateForD3DExDevice() const;

        virtual void deleteUnmanagedObjects();
        virtual void createUnmanagedObjects();

        virtual void createManagedObjects();
        virtual void deleteManagedObjects();

        //-- TODO: reconsider "section" parametr
        bool init(DataSectionPtr section);
        void fini();

        //-- ToDo (b_sviglo):
        void setQualityOption(int optionIndex);

        bool isDynamicShadowsEnabled() const;
        bool isSemiDynamicShadowsEnabled() const;
        bool isShadowsEnabled() const;
        float dynamicShadowsDistance() const;

        void tick(float dTime);

        void cast(Moo::DrawContext& shadowDrawContext);
        void receive();

        DX::BaseTexture* getResolveBuffer() const;

        void drawDebugInfo();

        void addRenderItem(IShadowRenderItem* item);
        RenderItemsList& getRenderItems() { return m_renderItems; }

#ifdef EDITOR_ENABLED
        typedef void (*GenerationFinishedCallback)();
        void setGenerationFinishedCallback(
            GenerationFinishedCallback callback);
        void resetAll();
#endif

    private:
        int  m_shadowQualityIndex;
        bool m_enabledManager;

        void enableDynamic(bool enabled);
        void enableSemi(bool enabled);

        float getBlendDistance() const; // between dyanmic and static

        DynamicShadow* m_pShadowsWrapper;
        std::auto_ptr<SemiDynamicShadow> m_semiDynamicShadows;

        bool m_isShadowQualityChanged;

        RenderItemsList      m_renderItems;
        Moo::RenderTargetPtr m_screenSpaceShadowMap;

#define DECLARE_MEMBER(var_type, var_name)  \
public:                                 \
    void set_##var_name(var_type val);  \
    var_type get_##var_name() const;    \
private:                                \
    var_type m_##var_name

        DECLARE_MEMBER(bool, debugMode);

        DECLARE_MEMBER(bool, dynamicEnabled);
        DECLARE_MEMBER(uint, dynamicTerrainShadowMapSize);
        DECLARE_MEMBER(uint, dynamicShadowMapSize);
        DECLARE_MEMBER(float, dynamicViewDistanceTo);

        DECLARE_MEMBER(bool, semiEnabled);
        DECLARE_MEMBER(float, semiViewDistanceFrom);
        DECLARE_MEMBER(uint, semiCoveredAreaSize);
        DECLARE_MEMBER(uint, semiShadowMapSize);
        DECLARE_MEMBER(float, semiBlendFactor);
    };

    //--------------------------------------------------------------------------------------------------
    // end
    //--------------------------------------------------------------------------------------------------
}

BW_END_NAMESPACE
