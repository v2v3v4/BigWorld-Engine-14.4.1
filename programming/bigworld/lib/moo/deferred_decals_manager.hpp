#pragma once

#include "cstdmf/singleton.hpp"
#include "cstdmf/smartpointer.hpp"
#include "forward_declarations.hpp"
#include "device_callback.hpp"
#include "moo_math.hpp"
#include "base_texture.hpp"
#include "vertex_buffer.hpp"
#include "dynamic_decal.hpp"
#include "texture_atlas.hpp"

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

//-- Manages all deferred decals on the scene in the current space.
//-- The real decals data is located here and the all external objects make deal only with an unique
//-- handle, which belongs to the every decal. This class doesn't perform any deleting and creating
//-- operations itself, instead of that it delegates their to the ChunkItem class. More over
//-- this class even doesn't do any culling operations because that is unnecessary, BigWorld's
//-- chunk system already has that ability. It just gets result about which decals are visible and
//-- draws only them.
//-- It also responsible for managing texture atlases for decals, to do this class has to know about
//-- all decals and their textures. It automatically updates these atlases in the background thread.
//--------------------------------------------------------------------------------------------------
class DecalsManager : public Moo::DeviceCallback, public Singleton<DecalsManager>
{
private:
    //-- make non-copyable.
    DecalsManager(const DecalsManager&);
    DecalsManager& operator = (const DecalsManager&);

public:

    //-- Represents decal structure.
    //----------------------------------------------------------------------------------------------
    struct Decal
    {
        //-- represents type of the decal i.e. to which surface type this decal can be applied. 
        //-- Flags may be combined to apply decal to the more than one type of surface.
        //--
        //-- Warning: Should be in sync with G_OBJECT_KIND_* constants in stdinclude.fxh
        enum EInfluenceType
        {
            APPLY_TO_TERRAIN = 1 << 1,
            APPLY_TO_FLORA   = 1 << 2,
            APPLY_TO_TREE    = 1 << 3,
            APPLY_TO_STATIC  = 1 << 4,
            APPLY_TO_DYNAMIC = 1 << 5,
            APPLY_TO_ALL     = APPLY_TO_FLORA | APPLY_TO_TERRAIN | APPLY_TO_TREE | APPLY_TO_STATIC | APPLY_TO_DYNAMIC
        };

        //-- represents available material types of the decal.
        enum EMaterialType
        {
            MATERIAL_DIFFUSE = 0,
            MATERIAL_BUMP,
            MATERIAL_PARALLAX,
            MATERIAL_ENV_MAPPING,
            MATERIAL_COUNT
        };

        //-- describes properties set for each individual decal.
        //-- Note: If map1Src and map2Src are available then used as a source for the texture atlas.
        //--       Means that m_map1 and m_map2 in this case are just names not the texture names.
        struct Desc
        {
            Desc() :    m_priority(0), m_materialType(MATERIAL_BUMP), m_influenceType(APPLY_TO_TERRAIN) { }

            Matrix              m_transform;
            BW::string          m_map1;
            BW::string          m_map2;
            Moo::BaseTexturePtr m_map1Src;
            Moo::BaseTexturePtr m_map2Src;
            uint32              m_priority;
            uint32              m_influenceType;
            EMaterialType       m_materialType;
        };

        Decal() :   m_dir(0,0,0), m_pos(0,0,0), m_up(0,0,0), m_scale(0,0,0), m_priority(0),
                    m_materialType(MATERIAL_BUMP),m_influenceType(APPLY_TO_TERRAIN) { }

        Vector3                     m_dir;
        Vector3                     m_pos;
        Vector3                     m_up;
        Vector3                     m_scale;

        TextureAtlas::SubTexturePtr m_map1;
        TextureAtlas::SubTexturePtr m_map2;
        uint                        m_priority;
        EMaterialType               m_materialType;
        uint                        m_influenceType;
    };

    //-- Represent dynamic nature of a decal. This type of decals
    struct DynamicDecal: public Decal
    {
        DynamicDecal() :
            m_creationTime(0.0f),
            m_lifeTime(1.0f),
            m_startAlpha()
        {}

        float       m_creationTime;
        float       m_lifeTime;
        BoundingBox m_worldBounds;
        float       m_startAlpha;
    };

    enum DebugRender
    {
        DEBUG_OVERDRAW,
        DEBUG_RENDER_COUNT
    };

public:
    DecalsManager();
    ~DecalsManager();

    bool                init();
    void                tick(float dt);
    void                draw();
    void                setQualityOption(int option);

    void                markVisible(int32 idx);

    //-- debugging.
    bool                showStaticDecalsAtlas() const;
    DX::BaseTexture*    staticDecalAtlas() const;
    
    //-- set fade range.
    void                setFadeRange(float start, float end);
    const Vector2&      getFadeRange() const { return m_fadeRange; }

    //-- function set for creating, deleting and updating each individual decal.
    int32               createStaticDecal(const Decal::Desc& desc);
    void                updateStaticDecal(int32 idx, const Decal::Desc& desc);
    void                updateStaticDecal(int32 idx, const Matrix& transform);
    void                removeStaticDecal(int32 idx);

    //--
    void                createDynamicDecal(const Decal::Desc& desc, float lifeTime, float startAlpha = 1.0f);
    void                clearDynamicDecals();

    //-- interface DeviceCallback.
    virtual bool        recreateForD3DExDevice() const;
    virtual void        createUnmanagedObjects();
    virtual void        deleteUnmanagedObjects();
    virtual void        createManagedObjects();
    virtual void        deleteManagedObjects();

#ifdef EDITOR_ENABLED
    void            enableDebugDraw(DebugRender dType) { m_debugRenderType = dType; m_debugDrawEnabled = true; }
    void            disableDebugDraw() { m_debugDrawEnabled = false; }
    void            showStaticDecalsAtlas(bool flag);   

    void            getTexturesUsed(uint idx, BW::map<void*, int>& textures);
    void            getVerticesUsed(BW::map<void*, int>& textures);
    float           getMap1AspectRatio(uint idx);
    static size_t   numVisibleDecals();
    static size_t   numTotalDecals();
    void            setVirtualOffset(const Vector3 &offset);
    bool            getTexturesValid( uint idx );
#endif

private:

    //-- Represent offset and size of instances for instancing rendering.
    //----------------------------------------------------------------------------------------------
    struct DrawData
    {
        DrawData() : m_offset(0), m_size(0) { }

        uint16 m_offset; //-- instances offset.
        uint16 m_size;   //-- instances count.
    };

    //-- represents decal data on the GPU side. It is tightly packed to achieve 16 alignment and to
    //-- save bandwidth.
    //----------------------------------------------------------------------------------------------
    struct GPUDecal
    {
        Vector4 m_dir;
        Vector4 m_pos;
        Vector4 m_up;
        Vector4 m_scale;
    };

    void        prepareToNextFrame();
    void        prepareVisibleDecals();
    void        uploadInstancingBuffer();
    float       calcFadeValue(const Decal& decal) const;
    GPUDecal    convert2GPU(const Decal& decal, float blend = 1.0f) const;

private:
    //-- list of the all static decals.
    //----------------------------------------------------------------------------------------------
    class StaticDecals
    {
    public:
        StaticDecals();

        Decal&          get         (int32 uid)         { return m_cache[uid]; }
        const Decal&    get         (int32 uid) const   { return m_cache[uid]; }
        size_t          size        () const            { return m_cache.size(); }
        void            insert      (int32 uid, const Decal& value);
        void            remove      (int32 uid);
        int32           nextGUID    () const;
        void            sort        (BW::vector<int32>& indices) const;

    private:
        BW::vector<Decal>           m_cache;
        mutable BW::vector<int32>   m_freeIndices;
    };

    //-- List of the all dynamic decals.
    //----------------------------------------------------------------------------------------------
    class DynamicDecals
    {
    public:
        DynamicDecals();

        DynamicDecal&       get     (int32 uid)         { return m_cache[uid]; }
        const DynamicDecal& get     (int32 uid) const   { return m_cache[uid]; }
        uint                size    () const            { return m_realSize; }
        void                clear   ()                  { m_realSize = 0; }
        void                add     (const DynamicDecal& value);
        void                remove  (int32 uid);
        void                sort    (BW::vector<int32>& indices) const;

    private:
        uint                        m_realSize;
        BW::vector<DynamicDecal>    m_cache;
    };

    Vector4                             m_staticAtlasSize;
    SmartPointer<TextureAtlas>          m_staticAtlasMap;

    //-- texture map atlases.
    Moo::EffectMaterialPtr              m_materials[Decal::MATERIAL_COUNT + 1];
    Moo::EffectMaterialPtr              m_debug_materials[DEBUG_RENDER_COUNT];
    Moo::VisualPtr                      m_cube;
    Moo::VertexBuffer                   m_instancedVB;
    DrawData                            m_drawData[Decal::MATERIAL_COUNT + 1];
    BW::vector<GPUDecal>                m_staticGPUDecals[Decal::MATERIAL_COUNT];
    BW::vector<GPUDecal>                m_dynamicGPUDecals[Decal::MATERIAL_COUNT];
    BW::vector<int32>                   m_visibleStaticDecals;
    BW::vector<int32>                   m_visibleDynamicDecals;

    StaticDecals                        m_staticDecals;
    DynamicDecals                       m_dynamicDecals;

    //--
    Vector2                             m_fadeRange;
    Vector3                             m_internalFadeRange;

    //-- for dynamic decals.
    float                               m_time;
    std::auto_ptr<DynamicDecalManager>  m_dynamicDecalsManager;

    DebugRender                         m_debugRenderType;
    bool                                m_debugDrawEnabled;

    //-- quality options.
    struct QualityOption
    {
        QualityOption(float lodBias) : m_lodBias(lodBias) { }

        float m_lodBias;
    };
    uint                        m_activeQualityOption;
    BW::vector<QualityOption>   m_qualityOptions;

#ifdef EDITOR_ENABLED
    Vector3 viewSpaceOffset;
#endif
};

BW_END_NAMESPACE
