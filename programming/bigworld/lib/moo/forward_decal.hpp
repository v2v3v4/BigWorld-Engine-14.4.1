
#pragma once

#include "utils.hpp"
#include "geometry.hpp"

#include "cstdmf\smartpointer.hpp"
#include "model\nodeless_model.hpp"
#include "chunk\chunk_space.hpp"
#include "terrain\terrain2\terrain_height_map2.hpp"
#include "duplo\py_attachment.hpp"
#include "moo\render_target.hpp"
#include "texture_atlas.hpp"

BW_BEGIN_NAMESPACE

class ChunkBSPObstacle;
class PyModel;


    enum EDecalType 
    {
        STICKER,
        INSCRIPTION,
        NUM_DECAL_TYPES
    };

    struct stickerGPU
    {
        Moo::VertexBuffer* vb;
        int verticesCount;
        Moo::BaseTexturePtr pTexSrc;
        TextureAtlas::SubTexturePtr texture;
        Vector4 uv;

        stickerGPU() : vb(NULL), verticesCount(0) {}
        ~stickerGPU();
    };

    class stickerCPU
    {
    public:
        typedef CPUSpotVertex VertexType;
    protected:
        BW::vector< VertexType >                m_vertices;
        int                                     m_id;
        BW::string                              m_textureID;
        float                                   m_alpha;
        Moo::BaseTexturePtr                     m_pTexSrc;
        stickerGPU                              m_stickerGPU;
        bool                                    m_isInscription;
    private:
        void eliminateGaps(const Vector3 &displacement);
    public:
        stickerCPU(int id, const BW::string& textureID, Moo::BaseTexture* pTexSrc, float alpha, bool isInscription);
        ~stickerCPU() { }
        void initialize( const Vector3 &start_, const Vector3 &end_, 
                         const Vector2 &size_, const PyModel* model, const Vector3 &up_ );
        bool refillGPUSticker();

        inline const BW::vector< VertexType >& vertices() const { return m_vertices; }
        inline int id() const { return m_id; }
        inline const BW::string& textureID() const { return m_textureID; }
        inline const float alpha() const { return m_alpha; }
        inline const Moo::BaseTexture* srcTexture() const { return m_pTexSrc.get(); }
        inline stickerGPU& gpuSticker() { return m_stickerGPU; } 
        inline bool isInscription() { return m_isInscription; } 
    };

    class ForwardStickerModel
    {
    private:
        int newDecalIndex;

        int id;

        float lodDistances[2];
        float alphas[2];
        BW::list<stickerCPU> decals;
        const PyModel* owner;

        bool refillStickerVB(stickerGPU& vb, const BW::string& texName);

    public:
        ForwardStickerModel();
        virtual ~ForwardStickerModel();
        virtual void tick( float dTime ) { }
        virtual void draw( const Matrix & worldTransform, float lod );
        void prepareDraw( float distance );
        virtual void tossed( bool outside );
        int addSticker( BW::string texture, const SmartPointer<PyModel> model_, 
            Vector3 segmentStart, Vector3 segmentEnd, Vector2 size_, Vector3 up_, Moo::BaseTexture* pSrcTex = NULL, bool isInscription = false);

        void delSticker(int stickerID);
        void clear();
        void setLODDistances(float texture0distance,float texture1distance);
        void setAlphas(float texture0alpha,float texture1alpha);
        void deleteUnmanagedObjects();
        void createUnmanagedObjects();
    };

    class TextureAtlas;

    class ForwardDecalsManager : public Moo::DeviceCallback
    {
        struct ForwardStickerDrawInfo
        {
            stickerGPU* pSticker;
            const Matrix worldTransform;
            float distance;
            float lod;
            float alpha;
            ForwardStickerDrawInfo(stickerGPU* _pSticker, const Matrix & _worldTransform, float _distance, float _lod, float _alpha ):
                pSticker(_pSticker), worldTransform(_worldTransform), distance(_distance), lod(_lod), alpha(_alpha) {}
        };
        BW::list<ForwardStickerModel*>      m_stickerModels;
        BW::list<ForwardStickerDrawInfo>    m_inscriptionsToDraw;
        BW::list<ForwardStickerDrawInfo>    m_stickersToDraw;
        //0 atlas containing all stickers except text-stickers
        //1 atlas containing text-stickers
        SmartPointer< TextureAtlas >        m_decalAtlasMap[NUM_DECAL_TYPES];
        Vector4                             m_decalAtlasSize[NUM_DECAL_TYPES];
        Moo::EffectMaterialPtr              m_material;
        // vertex declaration for drawing stickers
        Moo::VertexDeclaration *        m_stickerVertexDeclaration; 
    public:
        static ForwardDecalsManager* getInstance();
        ForwardDecalsManager();
        virtual ~ForwardDecalsManager();
        void removeSticker( ForwardStickerModel *pModel );
        void addSticker( ForwardStickerModel *pModel );

        virtual bool recreateForD3DExDevice() const;
        virtual void deleteUnmanagedObjects( );
        virtual void createUnmanagedObjects( );
        virtual void createManagedObjects();
        virtual void deleteManagedObjects();

        void addForwardStickerToDraw( stickerGPU* pSticker, const Matrix & worldTransform, float lod, float distance, float alpha, bool isInscription );
        void draw();

        TextureAtlas*   decalAtlas( EDecalType type ) const;
        const Vector4&  decalAtlasSize( EDecalType type ) const { return m_decalAtlasSize[type]; };
        Moo::EffectMaterial* pMaterial() const;

        static void init();
        static void fini();
    private:

        void drawPriv();
        void drawStickers(BW::list<ForwardStickerDrawInfo>& stickers, TextureAtlas* pAtlas, const Vector4& atlasSize);
        void drawSingleSticker( const ForwardStickerDrawInfo& drawInfo );

        static ForwardDecalsManager *s_pInstance;
    };

BW_END_NAMESPACE
