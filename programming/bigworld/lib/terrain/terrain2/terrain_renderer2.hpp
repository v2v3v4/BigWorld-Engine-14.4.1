#ifndef TERRAIN_TERRAIN_RENDERER2_HPP
#define TERRAIN_TERRAIN_RENDERER2_HPP

#include "../base_terrain_renderer.hpp"

#include "horizon_shadow_map2.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
    class VertexDeclaration;
    class EffectConstantValue;
    typedef SmartPointer<EffectConstantValue> EffectConstantValuePtr;
}

namespace Terrain
{

class BaseTexture;
class EffectMaterial;
class VertexDeclaration;
class TerrainPhotographer;
class TerrainBlock2;
class TerrainSettings;
class TerrainTextureLayer;
struct CombinedLayer;
struct TerrainBlends;

typedef SmartPointer<BaseTexture> BaseTexturePtr;
typedef SmartPointer<TerrainBlock2> TerrainBlock2Ptr;
typedef SmartPointer<TerrainBlends> TerrainBlendsPtr;
typedef SmartPointer<TerrainSettings>   TerrainSettingsPtr;

/**
 *  This class is the terrain renderer class, it contains the
 *  methods for rendering the terrain2 blocks
 */
class TerrainRenderer2 : public BaseTerrainRenderer
{
private:
    class BaseMaterial
    {
    public:
        void                    getHandles();

        void                    SetParam( D3DXHANDLE param, bool b );
        void                    SetParam( D3DXHANDLE param, float f );
        void                    SetParam( D3DXHANDLE param, int i );
        void                    SetParam( D3DXHANDLE param, const Vector4* v );
        void                    SetParam( D3DXHANDLE param, const Matrix* m );
        void                    SetParam( D3DXHANDLE param, DX::BaseTexture* t );

        Moo::EffectMaterialPtr  pEffect_;

        D3DXHANDLE              world_;
        D3DXHANDLE              terrainScale_;

        D3DXHANDLE              aoMap_;
        D3DXHANDLE              aoMapSize_;

        D3DXHANDLE              normalMap_;
        D3DXHANDLE              normalMapSize_;

        D3DXHANDLE              horizonMap_;
        D3DXHANDLE              horizonMapSize_;

        D3DXHANDLE              holesMap_;
        D3DXHANDLE              holesMapSize_;
        D3DXHANDLE              holesSize_;

        D3DXHANDLE              bumpShaderMask_;
        D3DXHANDLE              bumpFading_;

        D3DXHANDLE              layer_[4];
        D3DXHANDLE              bumpLayer_[4];
        D3DXHANDLE              layerUProjection_[4];
        D3DXHANDLE              layerVProjection_[4];

        // TerrainOverlayController.py support
        D3DXHANDLE              layerOverlayColors_[4];
        D3DXHANDLE              texOverlay_;
        D3DXHANDLE              overlayMode_;
        D3DXHANDLE              overlayBounds_;
        D3DXHANDLE              overlayAlpha_;
        D3DXHANDLE              overlayColor_;

        D3DXHANDLE              blendMap_;
        D3DXHANDLE              blendMapSize_;
        D3DXHANDLE              layerMask_;

        D3DXHANDLE              lodTextureStart_;
        D3DXHANDLE              lodTextureDistance_;

        D3DXHANDLE              useMultipassBlending_;
        D3DXHANDLE              hasHoles_;
        D3DXHANDLE              hasAO_;

        D3DXHANDLE              horizonShadowsBlendDistance;
    };

    class TexturedMaterial : public BaseMaterial
    {
    public:
        void                    getHandles();

        D3DXHANDLE              specularPower_;
        D3DXHANDLE              specularMultiplier_;
        D3DXHANDLE              specularFresnelExp_;
        D3DXHANDLE              specularFresnelConstant_;
    };

    class LodTextureMaterial : public BaseMaterial
    {
    public:
        //void                  getHandles();
    };

    class ZPassMaterial : public BaseMaterial
    {
    public:
        //void                  getHandles();
    };

public:
    ~TerrainRenderer2();

    static TerrainRenderer2* instance();

    static bool init();

    // BaseTerrainRenderer interface implementation
    uint32 version() const;

    void addBlock(
        BaseRenderTerrainBlock* pBlock, 
        const Matrix& transform
        );

    void drawAll(
        Moo::ERenderingPassType passType,
        Moo::EffectMaterialPtr pOverride = NULL,
        bool clearList = true
        );

    void drawSingle(
        Moo::ERenderingPassType passType,
        BaseRenderTerrainBlock* pBlock, 
                    const Matrix&           transform, 
        Moo::EffectMaterialPtr altMat = NULL
        );

    void clearBlocks();

    bool canSeeTerrain() const { return isVisible_; }
    bool supportSmallBlend() const { return supportSmallBlends_; }
    bool zBufferIsClear() const { return zBufferIsClear_; }
    void zBufferIsClear( bool zBufferIsClear ) { zBufferIsClear_ = zBufferIsClear; }

    static uint8 getLoadFlag(
        float minDistance,
                                        float textureBlendsPreloadDistance,
        float normalPreloadDistance
        );

    static uint8 getDrawFlag(
        uint8 partialResult,
        float minDistance,
        float maxDistance,
        const TerrainSettingsPtr& pSettings
        );

    bool IsHorizonShadowMapSuppressed() const { return isHorizonMapSuppressed; }
    void IsHorizonShadowMapSuppressed(bool isSuppressed) { isHorizonMapSuppressed = isSuppressed; }

    void setHorizonShadowsBlendDistance(Vector2 distances) { horizonShadowsBlendDistance = distances; }

    bool isSpecialModeEnabled_; // if set to true, TerrainRenderer renders terrain kind as defined by material_kind_classify.xml
    bool forceRenderLowestLod_; // temporary hack! when turned ON, the lowest terrain LOD is rendered forcibly
    
    //-- Editor specific tasks.
#ifdef EDITOR_ENABLED

public:
    //--
    TerrainPhotographer& photographer() { return *pPhotographer_; }
    bool editorGenerateLodTexture(BaseRenderTerrainBlock* pBlock, const Matrix& transform);

private:
    //--
    BaseMaterial lodMapGeneratorMaterial_;
    BaseMaterial overlayMaterial_;

    // current index in textureOverlays_ during drawing
    uint currTextureOverlayLayer_;

    // obsolete TERRAIN_OVERLAY_MODE_COLORIZE_GROUND_STRENGTH mode with colors auto-calculated from materialKind of each tile
    typedef BW::map<BW::string, Vector4> TerrainOverlayColorMap;
    TerrainOverlayColorMap colorOverlays_; // only for TERRAIN_OVERLAY_MODE_COLORIZE_GROUND_STRENGTH

public:
    uint terrainOverlayMode(); // routes call to OptionsTerrain::terrainOverlayMode(), modes are listed in OptionsHelper.hpp
    void terrainOverlayMode(uint mode); // same as above

    struct TextureOverlayLayer : public SafeReferenceCount
    {
        TextureOverlayLayer(ComObjectWrap<DX::Texture> pTex)
            : texture(pTex), alpha(1.0f), color(1, 1, 1, 1), visible(true)
        { }

        ComObjectWrap<DX::Texture> texture;
        Vector4 color; // texture color will be used as monochrome and modulated with this color
        float alpha; // will be multiplified by overlayAlpha
        bool visible;
    };
    typedef SmartPointer<TextureOverlayLayer> TextureOverlayLayerPtr;

    BW::vector<TextureOverlayLayerPtr> textureOverlays; // textures ordered as they are drawn (last is topmost)

    // Overlay texture with ground-types layout.
    // Used only in TERRAIN_OVERLAY_MODE_GROUND_TYPES_MAP mode.
    ComObjectWrap<DX::Texture> groundTypesTexture; 
    
    float overlayAlpha; // global alpha multiplifier
    Vector4 overlayBounds; // (minX, minZ, sizeX, sizeZ) used for worldPos->textureCoords mapping
    
#endif //-- EDITOR_ENABLED

    /** 
     * This enum describes which textures should be rendered
     * and what resources are needed for a block.
     */
    enum RenderTextureMask
    {
        RTM_None            = 0,
        RTM_DrawLOD         = 1 << 0,
        RTM_DrawBlend       = 1 << 1,
        RTM_DrawBump        = 1 << 2,
        RTM_PreLoadBlend    = 1 << 3,
        RTM_DrawLODNormals  = 1 << 4,
        RTM_PreloadNormals  = 1 << 5
    };

    uint32 getLayerIdx(uint32 layerNo, const CombinedLayer& combinedLayer);
    
private:

    enum DrawMethod
    { 
        DM_Z_ONLY,          /** Draw to Z buffer only. */
        DM_SHADOW_CAST,     /** Draw shadow cast. */
        DM_SINGLE_LOD,      /** Draw single pass, using lod texture. */
        DM_SINGLE_OVERRIDE, /** Draw single pass, using override material. */
        DM_BLEND,           /** Draw all blend materials. */
        DM_BUMP,            /** Draw all blend materials with bump mapping. */
        DM_LOD_NORMALS      /** Draw with low resolution normal map and lod texture */
    };

    TerrainRenderer2();

    bool initInternal(const DataSectionPtr& pSettingsSection );

    void setTransformConstants( const TerrainBlock2* pBlock, 
                                BaseMaterial* pMaterial );
    void setAOMapConstants( const TerrainBlock2* pBlock, BaseMaterial* pMaterial );
    void setNormalMapConstants( const TerrainBlock2* pBlock, 
                                BaseMaterial* pMaterial,
                                bool lodNormals );
    void setHorizonMapConstants( const TerrainBlock2* pBlock, 
                                BaseMaterial* pMaterial );
    void setHolesMapConstants( const TerrainBlock2* pBlock, 
                                BaseMaterial* pMaterial );
    void setTextureLayerConstants(const TerrainBlendsPtr& pBlend, 
                                BaseMaterial* pMaterial, 
                                uint32 layer, bool blended = false, bool bumped = false );
    void setSingleLayerConstants( BaseMaterial* pMaterial, uint32 layerNo, 
                                CombinedLayer const &combinedLayer, uint8& bumpShaderMask, bool bumped );
    void setLodTextureConstants( const TerrainBlock2*   pBlock, 
                                BaseMaterial*           pMaterial,
                                bool                    doBlend,
                                bool                    lodNormals);

    void updateConstants(   const TerrainBlock2*    pBlock,
                            const TerrainBlendsPtr& pBlends,
                            BaseMaterial*           pMaterial,
                            DrawMethod              drawMethod  );

    void drawSingleInternal(
                            Moo::ERenderingPassType passType,
                            BaseRenderTerrainBlock* pBlock, 
                            const Matrix&           transform, 
                            bool                    depthOnly = false,
                            BaseMaterial*           altMat = NULL );

    bool drawTerrainMaterial(   TerrainBlock2*          pBlock,
                                BaseMaterial*           pMaterial, 
                                DrawMethod              drawMethod,
                                uint32&                 passCount );

    void drawPlaceHolderBlock(  TerrainBlock2*          pBlock,
                                uint32                  colour,
                                bool                    lowDetail = false );

    // Watch accessors
    float specularPower() const { return specularInfo_.power_; };
    void specularPower( float power );
    float specularMultiplier() const { return specularInfo_.multiplier_; };
    void specularMultiplier( float mult );
    float specularFresnelExp() const { return specularInfo_.fresnelExp_; }
    void specularFresnelExp( float exp );
    float specularFresnelConstant() const { return specularInfo_.fresnelConstant_; };
    void specularFresnelConstant( float c );

    void ensureEffectMaterialsUpToDate();

    // types
    struct Block
    {
        Matrix              transform;
        TerrainBlock2Ptr    block;
        bool                depthOnly;
    };
    typedef AVectorNoDestructor<Block> BlockVector;
    struct Terrain2SpecularInfo
    {
        float power_;
        float multiplier_;
        float fresnelExp_;
        float fresnelConstant_;
    };

    BlockVector             blocks_;
    Terrain2SpecularInfo    specularInfo_;
    Moo::VertexDeclaration* pDecl_;
    TexturedMaterial        texturedMaterial_;
    TexturedMaterial        texturedReflectionMaterial_;
    LodTextureMaterial      lodTextureMaterial_;
    LodTextureMaterial      lodTextureReflectionMaterial_;
    ZPassMaterial           zPassMaterial_;
    BaseMaterial            shadowCastMaterial_;
    BaseMaterial            overrideMaterial_;
    uint32                  frameTimestamp_;
    uint32                  nBlocksRendered_;
    TerrainPhotographer*    pPhotographer_;
    bool                    supportSmallBlends_;
    bool                    zBufferIsClear_;
    bool                    useStencilMasking_;

    //-- ToDo: reconsider.
    // The smooth flow of dynamic shadows into horizonShadows
    // x - begin to appear behind of dynamic shadows
    // y - visible only horizonShadows
    Vector2 horizonShadowsBlendDistance;

    HorizonShadowMap2       dummyHorizonShadowMap_;
    bool                    isHorizonMapSuppressed;

    static TerrainRenderer2*    s_instance_;
};

inline uint8 TerrainRenderer2::getLoadFlag(float minDistance,
                                        float textureBlendsPreloadDistance,
                                        float normalPreloadDistance)
{   
    // If we're in the preload distance, flag a preload
    if ( minDistance <= textureBlendsPreloadDistance )
        return RTM_PreLoadBlend | RTM_PreloadNormals;

    if ( minDistance <= normalPreloadDistance )
        return RTM_PreloadNormals;

    // We only need to draw a lod texture
    return RTM_DrawLODNormals;
}


} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_TERRAIN_RENDERER2_HPP
