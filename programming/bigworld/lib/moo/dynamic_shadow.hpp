#pragma once

#include "cstdmf/singleton.hpp"
#include "cstdmf/smartpointer.hpp"
#include "moo/forward_declarations.hpp"
#include "moo/device_callback.hpp"
#include "moo/camera.hpp"
#include "math/forward_declarations.hpp"
#include "math/matrix.hpp"
#include "math/vector2.hpp"
#include "math/boundbox.hpp"
#include "moo/vertex_buffer.hpp"
#include "chunk/chunk.hpp"
#include "shadows_lib/shadows.hpp"

#include "moo/custom_mesh.hpp"
#include "moo/vertex_formats.hpp"


//--------------------------------------------------------------------------------------------------
// Configuration
//--------------------------------------------------------------------------------------------------

#define BW_SHADOWS_MAX_SPLIT_COUNT 4

//--------------------------------------------------------------------------------------------------
// Forward declarations.
//--------------------------------------------------------------------------------------------------

namespace DX
{
    typedef IDirect3DBaseTexture9 BaseTexture;
};

BW_BEGIN_NAMESPACE

class Chunk;

class ChunkItem;
typedef SmartPointer<ChunkItem> ChunkItemPtr;

namespace Moo
{
    class RenderTarget;
    class EffectMaterial;
    class BaseTexture;
    class DrawContext;

    typedef SmartPointer<RenderTarget> RenderTargetPtr;
};

typedef BW::vector<ChunkItemPtr> ShadowItems;

namespace Moo {
    //--------------------------------------------------------------------------------------------------
    // ShadowsWrapper
    //--------------------------------------------------------------------------------------------------

    class DynamicShadow : public Moo::DeviceCallback
    {
    public:

        enum QualityMask
        {
            QM_DINAMYC_CAST_ALL = 0,
            QM_DINAMYC_CAST_ATTACHMENT_ONLY = 1,
        };

        struct AtlasRect
        {
            AtlasRect() :
                x(0), y(0), w(0), h(0)
            {}

            AtlasRect(float x, float y, float w, float h) :
                x(x), y(y), w(w), h(h)
            { }

            float x; //-- top corner
            float y; //-- left corner
            float w; //-- width
            float h; //-- height
        };

        struct ShadowQuality
        {
            uint m_numSplit;
            int  m_blurQuality;
            int  m_castMask;
        };

        DynamicShadow(
            // rendering mode of the used area of Shadow Map.
            bool  debugMode,
            // map resolution for soft terrain shadows (in pixels)
            uint  terrainShadowMapSize,
            // shadow map resolution of one split (in pixels)
            uint  dynamicShadowMapSize,
            // visibility of shadows
            float viewDist,
            // quality of shadows.
            int   shadowQuality
        );

        ~DynamicShadow();

        bool enable(bool isEnabled);

        // interface
        void castShadows(Moo::DrawContext& shadowDrawContext);
        void receiveShadows();
        void drawDebugInfo();

        DX::BaseTexture* screenSpaceShadowMap();

        // DeviceCallback implementations
        virtual void deleteUnmanagedObjects();
        virtual void createUnmanagedObjects();
        virtual bool recreateForD3DExDevice() const;

        void setNumSplit(uint numSplit);
        uint getNumSplit() const;

    private:
        bool createNoiseMap();

        //-- init flags
        //-- TODO: reconsider initialization mechanism
        bool                        m_inited;  // init() has been called
        bool                        m_enabled; // the object enabled from options
        bool                        m_unmanagedObjectsCreated;

        //-- construction settings
        const bool m_debugMode;
        const uint m_terrainShadowMapSize;
        const uint m_dynamicShadowMapSize;
        const float m_viewDist;
        ShadowQuality m_shadowQuality;

        //-- data (transfer from cast to receive pass)
        BW::vector<float>           m_splitPlanes;
        BW::vector<Matrix>          m_textureMatrices;

        //-- resources (main)
        Moo::RenderTargetPtr        m_dynamicShadowMapAtlas;
        ComObjectWrap<DX::Texture>  m_pNoiseMap;

        //-- resources (cast)
        Moo::EffectMaterialPtr      m_terrainCast;
        Moo::EffectMaterialPtr      m_fillScreenMap; // fill screen map using z-buffer

        //-- resources (debug mode)
        Moo::EffectMaterialPtr              m_debugEffectMaterial;
        AtlasRect                           m_atlasLayout[BW_SHADOWS_MAX_SPLIT_COUNT];
    };

}

BW_END_NAMESPACE

//--------------------------------------------------------------------------------------------------
// End
//--------------------------------------------------------------------------------------------------